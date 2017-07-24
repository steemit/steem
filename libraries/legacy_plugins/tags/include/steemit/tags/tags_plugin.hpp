#pragma once

#include <steemit/app/plugin.hpp>
#include <steemit/chain/database.hpp>
#include <steemit/chain/comment_object.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <fc/thread/future.hpp>
#include <fc/api.hpp>

namespace steemit { namespace tags {
using namespace steemit::chain;
using namespace boost::multi_index;

using steemit::app::application;

using chainbase::object;
using chainbase::oid;
using chainbase::allocator;

//
// Plugins should #define their SPACE_ID's so plugins with
// conflicting SPACE_ID assignments can be compiled into the
// same binary (by simply re-assigning some of the conflicting #defined
// SPACE_ID's in a build script).
//
// Assignment of SPACE_ID's cannot be done at run-time because
// various template automagic depends on them being known at compile
// time.
//
#ifndef TAG_SPACE_ID
#define TAG_SPACE_ID 5
#endif

#define TAGS_PLUGIN_NAME "tags"

typedef protocol::fixed_string_32 tag_name_type;

// Plugins need to define object type IDs such that they do not conflict
// globally. If each plugin uses the upper 8 bits as a space identifier,
// with 0 being for chain, then the lower 8 bits are free for each plugin
// to define as they see fit.
enum
{
   tag_object_type              = ( TAG_SPACE_ID << 8 ),
   tag_stats_object_type        = ( TAG_SPACE_ID << 8 ) + 1,
   peer_stats_object_type       = ( TAG_SPACE_ID << 8 ) + 2,
   author_tag_stats_object_type = ( TAG_SPACE_ID << 8 ) + 3
};

namespace detail { class tags_plugin_impl; }


/**
 *  The purpose of the tag object is to allow the generation and listing of
 *  all top level posts by a string tag.  The desired sort orders include:
 *
 *  1. created - time of creation
 *  2. maturing - about to receive a payout
 *  3. active - last reply the post or any child of the post
 *  4. netvotes - individual accounts voting for post minus accounts voting against it
 *
 *  When ever a comment is modified, all tag_objects for that comment are updated to match.
 */
class tag_object : public object< tag_object_type, tag_object >
{
   public:
      template< typename Constructor, typename Allocator >
      tag_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      tag_object() {}

      id_type           id;

      tag_name_type     tag;
      time_point_sec    created;
      time_point_sec    active;
      time_point_sec    cashout;
      int64_t           net_rshares = 0;
      int32_t           net_votes   = 0;
      int32_t           children    = 0;
      double            hot         = 0;
      double            trending    = 0;
      share_type        promoted_balance = 0;

      account_id_type   author;
      comment_id_type   parent;
      comment_id_type   comment;

      bool is_post()const { return parent == comment_id_type(); }
};

typedef oid< tag_object > tag_id_type;


struct by_cashout; /// all posts regardless of depth
struct by_net_rshares; /// all comments regardless of depth
struct by_parent_created;
struct by_parent_active;
struct by_parent_promoted;
struct by_parent_net_rshares; /// all top level posts by direct pending payout
struct by_parent_net_votes; /// all top level posts by direct votes
struct by_parent_trending;
struct by_parent_children; /// all top level posts with the most discussion (replies at all levels)
struct by_parent_hot;
struct by_author_parent_created;  /// all blog posts by author with tag
struct by_author_comment;
struct by_reward_fund_net_rshares;
struct by_comment;
struct by_tag;


typedef multi_index_container<
   tag_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< tag_object, tag_id_type, &tag_object::id > >,
      ordered_unique< tag< by_comment >,
         composite_key< tag_object,
            member< tag_object, comment_id_type, &tag_object::comment >,
            member< tag_object, tag_id_type, &tag_object::id >
         >,
         composite_key_compare< std::less< comment_id_type >, std::less< tag_id_type > >
      >,
      ordered_unique< tag< by_author_comment >,
            composite_key< tag_object,
               member< tag_object, account_id_type, &tag_object::author >,
               member< tag_object, comment_id_type, &tag_object::comment >,
               member< tag_object, tag_id_type, &tag_object::id >
            >,
            composite_key_compare< std::less< account_id_type >, std::less< comment_id_type >, std::less< tag_id_type > >
      >,
      ordered_unique< tag< by_parent_created >,
            composite_key< tag_object,
               member< tag_object, tag_name_type, &tag_object::tag >,
               member< tag_object, comment_id_type, &tag_object::parent >,
               member< tag_object, time_point_sec, &tag_object::created >,
               member<tag_object, tag_id_type, &tag_object::id >
            >,
            composite_key_compare< std::less< tag_name_type >, std::less<comment_id_type>, std::greater< time_point_sec >, std::less< tag_id_type > >
      >,
      ordered_unique< tag< by_parent_active >,
            composite_key< tag_object,
               member< tag_object, tag_name_type, &tag_object::tag >,
               member< tag_object, comment_id_type, &tag_object::parent >,
               member< tag_object, time_point_sec, &tag_object::active >,
               member< tag_object, tag_id_type, &tag_object::id >
            >,
            composite_key_compare< std::less<tag_name_type>, std::less<comment_id_type>, std::greater< time_point_sec >, std::less< tag_id_type > >
      >,
      ordered_unique< tag< by_parent_promoted >,
            composite_key< tag_object,
               member< tag_object, tag_name_type, &tag_object::tag >,
               member< tag_object, comment_id_type, &tag_object::parent >,
               member< tag_object, share_type, &tag_object::promoted_balance >,
               member< tag_object, tag_id_type, &tag_object::id >
            >,
            composite_key_compare< std::less<tag_name_type>, std::less<comment_id_type>, std::greater< share_type >, std::less< tag_id_type > >
      >,
      ordered_unique< tag< by_parent_net_rshares >,
            composite_key< tag_object,
               member< tag_object, tag_name_type, &tag_object::tag >,
               member< tag_object, comment_id_type, &tag_object::parent >,
               member< tag_object, int64_t, &tag_object::net_rshares >,
               member< tag_object, tag_id_type, &tag_object::id >
            >,
            composite_key_compare< std::less<tag_name_type>, std::less<comment_id_type>, std::greater< int64_t >, std::less< tag_id_type > >
      >,
      ordered_unique< tag< by_parent_net_votes >,
            composite_key< tag_object,
               member< tag_object, tag_name_type, &tag_object::tag >,
               member< tag_object, comment_id_type, &tag_object::parent >,
               member< tag_object, int32_t, &tag_object::net_votes >,
               member< tag_object, tag_id_type, &tag_object::id >
            >,
            composite_key_compare< std::less<tag_name_type>, std::less<comment_id_type>, std::greater< int32_t >, std::less< tag_id_type > >
      >,
      ordered_unique< tag< by_parent_children >,
            composite_key< tag_object,
               member< tag_object, tag_name_type, &tag_object::tag >,
               member< tag_object, comment_id_type, &tag_object::parent >,
               member< tag_object, int32_t, &tag_object::children >,
               member< tag_object, tag_id_type, &tag_object::id >
            >,
            composite_key_compare< std::less<tag_name_type>, std::less<comment_id_type>, std::greater< int32_t >, std::less< tag_id_type > >
      >,
      ordered_unique< tag< by_parent_hot >,
            composite_key< tag_object,
               member< tag_object, tag_name_type, &tag_object::tag >,
               member< tag_object, comment_id_type, &tag_object::parent >,
               member< tag_object, double, &tag_object::hot >,
               member< tag_object, tag_id_type, &tag_object::id >
            >,
            composite_key_compare< std::less<tag_name_type>, std::less<comment_id_type>, std::greater< double >, std::less< tag_id_type > >
      >,
      ordered_unique< tag< by_parent_trending >,
            composite_key< tag_object,
               member< tag_object, tag_name_type, &tag_object::tag >,
               member< tag_object, comment_id_type, &tag_object::parent >,
               member< tag_object, double, &tag_object::trending >,
               member< tag_object, tag_id_type, &tag_object::id >
            >,
            composite_key_compare< std::less<tag_name_type>, std::less<comment_id_type>, std::greater< double >, std::less< tag_id_type > >
      >,
      ordered_unique< tag< by_cashout >,
            composite_key< tag_object,
               member< tag_object, tag_name_type, &tag_object::tag >,
               member< tag_object, time_point_sec, &tag_object::cashout >,
               member< tag_object, tag_id_type, &tag_object::id >
            >,
            composite_key_compare< std::less<tag_name_type>, std::less< time_point_sec >, std::less< tag_id_type > >
      >,
      ordered_unique< tag< by_net_rshares >,
            composite_key< tag_object,
               member< tag_object, tag_name_type, &tag_object::tag >,
               member< tag_object, int64_t, &tag_object::net_rshares >,
               member< tag_object, tag_id_type, &tag_object::id >
            >,
            composite_key_compare< std::less<tag_name_type>, std::greater< int64_t >, std::less< tag_id_type > >
      >,
      ordered_unique< tag< by_author_parent_created >,
            composite_key< tag_object,
               member< tag_object, tag_name_type, &tag_object::tag >,
               member< tag_object, account_id_type, &tag_object::author >,
               member< tag_object, time_point_sec, &tag_object::created >,
               member< tag_object, tag_id_type, &tag_object::id >
            >,
            composite_key_compare< std::less<tag_name_type>, std::less<account_id_type>, std::greater< time_point_sec >, std::less< tag_id_type > >
      >,
      ordered_unique< tag< by_reward_fund_net_rshares >,
            composite_key< tag_object,
               member< tag_object, tag_name_type, &tag_object::tag >,
               const_mem_fun< tag_object, bool, &tag_object::is_post >,
               member< tag_object, int64_t, &tag_object::net_rshares >,
               member< tag_object, tag_id_type, &tag_object::id >
            >,
            composite_key_compare< std::less<tag_name_type>, std::less< bool >,std::greater< int64_t >, std::less< tag_id_type > >
      >
   >,
   allocator< tag_object >
> tag_index;

/**
 *  The purpose of this index is to quickly identify how popular various tags by maintaining variou sums over
 *  all posts under a particular tag
 */
class tag_stats_object : public object< tag_stats_object_type, tag_stats_object >
{
   public:
      template< typename Constructor, typename Allocator >
      tag_stats_object( Constructor&& c, allocator< Allocator > )
      {
         c( *this );
      }

      tag_stats_object() {}

      id_type           id;

      tag_name_type     tag;
      asset             total_payout = asset( 0, SBD_SYMBOL );
      int32_t           net_votes = 0;
      uint32_t          top_posts = 0;
      uint32_t          comments  = 0;
      fc::uint128       total_trending = 0;
};

typedef oid< tag_stats_object > tag_stats_id_type;

struct by_comments;
struct by_top_posts;
struct by_trending;

typedef multi_index_container<
   tag_stats_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< tag_stats_object, tag_stats_id_type, &tag_stats_object::id > >,
      ordered_unique< tag< by_tag >, member< tag_stats_object, tag_name_type, &tag_stats_object::tag > >,
      /*
      ordered_non_unique< tag< by_comments >,
         composite_key< tag_stats_object,
            member< tag_stats_object, uint32_t, &tag_stats_object::comments >,
            member< tag_stats_object, tag_name_type, &tag_stats_object::tag >
         >,
         composite_key_compare< std::less< tag_name_type >, std::greater< uint32_t > >
      >,
      ordered_non_unique< tag< by_top_posts >,
         composite_key< tag_stats_object,
            member< tag_stats_object, uint32_t, &tag_stats_object::top_posts >,
            member< tag_stats_object, tag_name_type, &tag_stats_object::tag >
         >,
         composite_key_compare< std::less< tag_name_type >, std::greater< uint32_t > >
      >,
      */
      ordered_non_unique< tag< by_trending >,
         composite_key< tag_stats_object,
            member< tag_stats_object, fc::uint128 , &tag_stats_object::total_trending >,
            member< tag_stats_object, tag_name_type, &tag_stats_object::tag >
         >,
         composite_key_compare<  std::greater< fc::uint128  >, std::less< tag_name_type > >
      >
  >,
  allocator< tag_stats_object >
> tag_stats_index;


/**
 *  The purpose of this object is to track the relationship between accounts based upon how a user votes. Every time
 *  a user votes on a post, the relationship between voter and author increases direct rshares.
 */
class peer_stats_object : public object< peer_stats_object_type, peer_stats_object >
{
   public:
      template< typename Constructor, typename Allocator >
      peer_stats_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      peer_stats_object() {}

      id_type           id;

      account_id_type   voter;
      account_id_type   peer;
      int32_t           direct_positive_votes = 0;
      int32_t           direct_votes = 1;

      int32_t           indirect_positive_votes = 0;
      int32_t           indirect_votes = 1;

      float             rank = 0;

      void update_rank()
      {
          auto direct         = float( direct_positive_votes ) / direct_votes;
          auto indirect       = float( indirect_positive_votes ) / indirect_votes;
          auto direct_order   = log( direct_votes );
          auto indirect_order = log( indirect_votes );

          if( !(direct_positive_votes+indirect_positive_votes) ){
            direct_order *= -1;
            indirect_order *= -1;
          }

          direct *= direct;
          indirect *= indirect;

          direct *= direct_order * 10;
          indirect *= indirect_order;

          rank = direct + indirect;
      }
};

typedef oid< peer_stats_object > peer_stats_id_type;

struct by_rank;
struct by_voter_peer;
typedef multi_index_container<
   peer_stats_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< peer_stats_object, peer_stats_id_type, &peer_stats_object::id > >,
      ordered_unique< tag< by_rank >,
         composite_key< peer_stats_object,
            member< peer_stats_object, account_id_type, &peer_stats_object::voter >,
            member< peer_stats_object, float, &peer_stats_object::rank >,
            member< peer_stats_object, account_id_type, &peer_stats_object::peer >
         >,
         composite_key_compare< std::less< account_id_type >, std::greater< float >, std::less< account_id_type > >
      >,
      ordered_unique< tag< by_voter_peer >,
         composite_key< peer_stats_object,
            member< peer_stats_object, account_id_type, &peer_stats_object::voter >,
            member< peer_stats_object, account_id_type, &peer_stats_object::peer >
         >,
         composite_key_compare< std::less< account_id_type >,  std::less< account_id_type > >
      >
   >,
   allocator< peer_stats_object >
> peer_stats_index;

/**
 *  This purpose of this object is to maintain stats about which tags an author uses, how frequnetly, and
 *  how many total earnings of all posts by author in tag.  It also allows us to answer the question of which
 *  authors earn the most in each tag category.  This helps users to discover the best bloggers to follow for
 *  particular tags.
 */
class author_tag_stats_object : public object< author_tag_stats_object_type, author_tag_stats_object >
{
  public:
      template< typename Constructor, typename Allocator >
      author_tag_stats_object( Constructor&& c, allocator< Allocator > )
      {
         c( *this );
      }

      id_type         id;
      account_id_type author;
      tag_name_type   tag;
      asset           total_rewards = asset( 0, SBD_SYMBOL );
      uint32_t        total_posts = 0;
};
typedef oid< author_tag_stats_object > author_tag_stats_id_type;

struct by_author_tag_posts;
struct by_author_posts_tag;
struct by_author_tag_rewards;
struct by_tag_rewards_author;
using std::less;
using std::greater;

typedef chainbase::shared_multi_index_container<
  author_tag_stats_object,
  indexed_by<
      ordered_unique< tag< by_id >,
        member< author_tag_stats_object, author_tag_stats_id_type, &author_tag_stats_object::id >
      >,
      ordered_unique< tag< by_author_posts_tag >,
         composite_key< author_tag_stats_object,
            member< author_tag_stats_object, account_id_type, &author_tag_stats_object::author >,
            member< author_tag_stats_object, uint32_t, &author_tag_stats_object::total_posts >,
            member< author_tag_stats_object, tag_name_type, &author_tag_stats_object::tag >
         >,
         composite_key_compare< less< account_id_type >, greater< uint32_t >, less< tag_name_type > >
      >,
      ordered_unique< tag< by_author_tag_posts >,
         composite_key< author_tag_stats_object,
            member< author_tag_stats_object, account_id_type, &author_tag_stats_object::author >,
            member< author_tag_stats_object, tag_name_type, &author_tag_stats_object::tag >,
            member< author_tag_stats_object, uint32_t, &author_tag_stats_object::total_posts >
         >,
         composite_key_compare< less< account_id_type >, less< tag_name_type >, greater< uint32_t > >
      >,
      ordered_unique< tag< by_author_tag_rewards >,
         composite_key< author_tag_stats_object,
            member< author_tag_stats_object, account_id_type, &author_tag_stats_object::author >,
            member< author_tag_stats_object, tag_name_type, &author_tag_stats_object::tag >,
            member< author_tag_stats_object, asset, &author_tag_stats_object::total_rewards >
         >,
         composite_key_compare< less< account_id_type >, less< tag_name_type >, greater< asset > >
      >,
      ordered_unique< tag< by_tag_rewards_author >,
         composite_key< author_tag_stats_object,
            member< author_tag_stats_object, tag_name_type, &author_tag_stats_object::tag >,
            member< author_tag_stats_object, asset, &author_tag_stats_object::total_rewards >,
            member< author_tag_stats_object, account_id_type, &author_tag_stats_object::author >
         >,
         composite_key_compare< less< tag_name_type >, greater< asset >, less< account_id_type > >
      >
  >
> author_tag_stats_index;

/**
 * Used to parse the metadata from the comment json_meta field.
 */
struct comment_metadata { set<string> tags; };

/**
 *  This plugin will scan all changes to posts and/or their meta data and
 *
 */
class tags_plugin : public steemit::app::plugin
{
   public:
      tags_plugin( application* app );
      virtual ~tags_plugin();

      std::string plugin_name()const override { return TAGS_PLUGIN_NAME; }
      virtual void plugin_set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg) override;
      virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
      virtual void plugin_startup() override;

      friend class detail::tags_plugin_impl;
      std::unique_ptr<detail::tags_plugin_impl> my;
};

/**
 *  This API is used to query data maintained by the tags_plugin
 */
class tag_api : public std::enable_shared_from_this<tag_api> {
   public:
      tag_api(){};
      tag_api(const app::api_context& ctx){}//:_app(&ctx.app){}

      void on_api_startup(){
      }

      vector<tag_stats_object> get_tags()const { return vector<tag_stats_object>(); }

   private:
      //app::application* _app = nullptr;
};



} } //steemit::tag

FC_API( steemit::tags::tag_api, (get_tags) );

FC_REFLECT( steemit::tags::tag_object,
   (id)(tag)(created)(active)(cashout)(net_rshares)(net_votes)(hot)(trending)(promoted_balance)(children)(author)(parent)(comment) )
CHAINBASE_SET_INDEX_TYPE( steemit::tags::tag_object, steemit::tags::tag_index )

FC_REFLECT( steemit::tags::tag_stats_object,
   (id)(tag)(total_payout)(net_votes)(top_posts)(comments)(total_trending) );
CHAINBASE_SET_INDEX_TYPE( steemit::tags::tag_stats_object, steemit::tags::tag_stats_index )

FC_REFLECT( steemit::tags::peer_stats_object,
   (id)(voter)(peer)(direct_positive_votes)(direct_votes)(indirect_positive_votes)(indirect_votes)(rank) );
CHAINBASE_SET_INDEX_TYPE( steemit::tags::peer_stats_object, steemit::tags::peer_stats_index )

FC_REFLECT( steemit::tags::comment_metadata, (tags) );

FC_REFLECT( steemit::tags::author_tag_stats_object, (id)(author)(tag)(total_posts)(total_rewards) )
CHAINBASE_SET_INDEX_TYPE( steemit::tags::author_tag_stats_object, steemit::tags::author_tag_stats_index )
