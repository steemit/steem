#pragma once
#include <steem/plugins/json_rpc/utility.hpp>
#include <steem/plugins/database_api/database_api_objects.hpp>
#include <steem/plugins/tags/tags_plugin.hpp>

#include <steem/protocol/types.hpp>
#include <steem/chain/database.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>

namespace steem { namespace plugins { namespace tags {

using steem::protocol::share_type;
using steem::protocol::asset;
using steem::protocol::account_name_type;
using fc::time_point_sec;

namespace detail { class tags_api_impl; }

struct api_tag_object
{
   api_tag_object( const tag_stats_object& o ) :
      name( o.tag ),
      total_payouts( o.total_payout ),
      net_votes( o.net_votes ),
      top_posts( o.top_posts ),
      comments( o.comments ),
      trending( o.total_trending ) {}

   api_tag_object() {}

   string               name;
   asset                total_payouts;
   int32_t              net_votes = 0;
   uint32_t             top_posts = 0;
   uint32_t             comments = 0;
   fc::uint128          trending = 0;
};

struct vote_state
{
   string         voter;
   uint64_t       weight = 0;
   int64_t        rshares = 0;
   int16_t        percent = 0;
   share_type     reputation = 0;
   time_point_sec time;
};

struct discussion : public database_api::api_comment_object
{
   discussion( const steem::chain::comment_object& o, const steem::chain::database& db ) :
      database_api::api_comment_object( o, db ) {}

   discussion(){}

   string                        url; /// /category/@rootauthor/root_permlink#author/permlink
   string                        root_title;
   asset                         pending_payout_value; ///< sbd
   asset                         total_pending_payout_value; ///< sbd including replies
   vector< vote_state >          active_votes;
   vector< string >              replies; ///< author/slug mapping
   share_type                    author_reputation = 0;
   asset                         promoted = asset(0, SBD_SYMBOL);
   uint32_t                      body_length = 0;
   vector< account_name_type >   reblogged_by;
   optional< account_name_type > first_reblogged_by;
   optional< time_point_sec >    first_reblogged_on;
};

struct get_trending_tags_args
{
   string   start_tag;
   uint32_t limit = 100;
};

struct get_trending_tags_return
{
   vector< api_tag_object > tags;
};

struct get_tags_used_by_author_args
{
   account_name_type author;
};

struct tag_count_object
{
   string   tag;
   uint32_t count;
};

struct get_tags_used_by_author_return
{
   vector< tag_count_object > tags;
};

struct get_discussion_args
{
   account_name_type author;
   string            permlink;
};

typedef discussion get_discussion_return;

struct discussion_query
{
   void validate() const
   {
      FC_ASSERT( filter_tags.find(tag) == filter_tags.end() );
      FC_ASSERT( limit <= 100 );
   }

   string               tag;
   uint32_t             limit = 0;
   set< string >        filter_tags;
   set< string >        select_authors; ///< list of authors to include, posts not by this author are filtered
   set< string >        select_tags; ///< list of tags to include, posts without these tags are filtered
   uint32_t             truncate_body = 0; ///< the number of bytes of the post body to return, 0 for all
   optional< string >   start_author;
   optional< string >   start_permlink;
   optional< string >   parent_author;
   optional< string >   parent_permlink;
};

struct discussion_query_result
{
   vector< discussion > discussions;
};

typedef get_discussion_args      get_content_replies_args;
typedef discussion_query_result  get_content_replies_return;

typedef discussion_query         get_post_discussions_by_payout_args;
typedef discussion_query_result  get_post_discussions_by_payout_return;

typedef discussion_query         get_comment_discussions_by_payout_args;
typedef discussion_query_result  get_comment_discussions_by_payout_return;

typedef discussion_query         get_discussions_by_trending_args;
typedef discussion_query_result  get_discussions_by_trending_return;

typedef discussion_query         get_discussions_by_created_args;
typedef discussion_query_result  get_discussions_by_created_return;

typedef discussion_query         get_discussions_by_active_args;
typedef discussion_query_result  get_discussions_by_active_return;

typedef discussion_query         get_discussions_by_cashout_args;
typedef discussion_query_result  get_discussions_by_cashout_return;

typedef discussion_query         get_discussions_by_votes_args;
typedef discussion_query_result  get_discussions_by_votes_return;

typedef discussion_query         get_discussions_by_children_args;
typedef discussion_query_result  get_discussions_by_children_return;

typedef discussion_query         get_discussions_by_hot_args;
typedef discussion_query_result  get_discussions_by_hot_return;

typedef discussion_query         get_discussions_by_feed_args;
typedef discussion_query_result  get_discussions_by_feed_return;

typedef discussion_query         get_discussions_by_blog_args;
typedef discussion_query_result  get_discussions_by_blog_return;

typedef discussion_query         get_discussions_by_comments_args;
typedef discussion_query_result  get_discussions_by_comments_return;

typedef discussion_query         get_discussions_by_promoted_args;
typedef discussion_query_result  get_discussions_by_promoted_return;

struct get_replies_by_last_update_args
{
   account_name_type start_parent_author;
   string            start_permlink;
   uint32_t          limit = 100;
};

typedef discussion_query_result get_replies_by_last_update_return;

struct get_discussions_by_author_before_date_args
{
   account_name_type author;
   string            start_permlink;
   time_point_sec    before_date;
   uint32_t          limit = 100;
};

typedef discussion_query_result get_discussions_by_author_before_date_return;

typedef get_discussion_args get_active_votes_args;

struct get_active_votes_return
{
   vector< vote_state > votes;
};

class tags_api
{
   public:
      tags_api();
      ~tags_api();

   DECLARE_API(
      (get_trending_tags)
      (get_tags_used_by_author)
      (get_discussion)
      (get_content_replies)
      (get_post_discussions_by_payout)
      (get_comment_discussions_by_payout)
      (get_discussions_by_trending)
      (get_discussions_by_created)
      (get_discussions_by_active)
      (get_discussions_by_cashout)
      (get_discussions_by_votes)
      (get_discussions_by_children)
      (get_discussions_by_hot)
      (get_discussions_by_feed)
      (get_discussions_by_blog)
      (get_discussions_by_comments)
      (get_discussions_by_promoted)
      (get_replies_by_last_update)
      (get_discussions_by_author_before_date)
      (get_active_votes)
   )

   void set_pending_payout( discussion& d );

   private:
      friend class tags_api_plugin;
      void api_startup();

      std::unique_ptr< detail::tags_api_impl > my;
};

} } } // steem::plugins::tags

FC_REFLECT( steem::plugins::tags::api_tag_object,
            (name)(total_payouts)(net_votes)(top_posts)(comments)(trending) )

FC_REFLECT( steem::plugins::tags::vote_state,
            (voter)(weight)(rshares)(percent)(reputation)(time) )

FC_REFLECT_DERIVED( steem::plugins::tags::discussion, (steem::plugins::database_api::api_comment_object),
            (url)(root_title)(pending_payout_value)(total_pending_payout_value)(active_votes)(replies)(author_reputation)(promoted)(body_length)(reblogged_by)(first_reblogged_by)(first_reblogged_on) )

FC_REFLECT( steem::plugins::tags::get_trending_tags_args,
            (start_tag)(limit) )

FC_REFLECT( steem::plugins::tags::get_trending_tags_return,
            (tags) )

FC_REFLECT( steem::plugins::tags::get_tags_used_by_author_args,
            (author) )

FC_REFLECT( steem::plugins::tags::tag_count_object,
            (tag)(count) )

FC_REFLECT( steem::plugins::tags::get_tags_used_by_author_return,
            (tags) )

FC_REFLECT( steem::plugins::tags::get_discussion_args,
            (author)(permlink) )

FC_REFLECT( steem::plugins::tags::discussion_query,
            (tag)(limit)(filter_tags)(select_authors)(select_tags)(truncate_body)(start_author)(start_permlink)(parent_author)(parent_permlink) )

FC_REFLECT( steem::plugins::tags::discussion_query_result,
            (discussions) )

FC_REFLECT( steem::plugins::tags::get_replies_by_last_update_args,
            (start_parent_author)(start_permlink)(limit) )

FC_REFLECT( steem::plugins::tags::get_discussions_by_author_before_date_args,
            (author)(start_permlink)(before_date)(limit) )

FC_REFLECT( steem::plugins::tags::get_active_votes_return,
            (votes) )
