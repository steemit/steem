#pragma once
#include <steem/chain/steem_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace steem { namespace plugins { namespace follow {

using namespace std;
using namespace steem::chain;

using chainbase::t_vector;

#ifndef STEEM_FOLLOW_SPACE_ID
#define STEEM_FOLLOW_SPACE_ID 8
#endif

enum follow_plugin_object_type
{
   follow_object_type            = ( STEEM_FOLLOW_SPACE_ID << 8 ),
   feed_object_type              = ( STEEM_FOLLOW_SPACE_ID << 8 ) + 1,
   reputation_object_type        = ( STEEM_FOLLOW_SPACE_ID << 8 ) + 2,
   blog_object_type              = ( STEEM_FOLLOW_SPACE_ID << 8 ) + 3,
   follow_count_object_type      = ( STEEM_FOLLOW_SPACE_ID << 8 ) + 4,
   blog_author_stats_object_type = ( STEEM_FOLLOW_SPACE_ID << 8 ) + 5
};

enum follow_type
{
   undefined,
   blog,
   ignore
};

class follow_object : public object< follow_object_type, follow_object >
{
   public:
      template< typename Constructor, typename Allocator >
      follow_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      follow_object() {}

      id_type           id;

      account_name_type follower;
      account_name_type following;
      uint16_t          what = 0;
};

typedef oid< follow_object > follow_id_type;


class feed_object : public object< feed_object_type, feed_object >
{
   public:
      typedef t_vector<account_name_type> t_reblogged_by_container;

      feed_object() = delete;

      template< typename Constructor, typename Allocator >
      feed_object( Constructor&& c, allocator< Allocator > a )
      :reblogged_by( a )
      {
         c( *this );
      }

      id_type                    id;

      account_name_type                account;
      t_reblogged_by_container         reblogged_by;
      account_name_type                first_reblogged_by;
      time_point_sec                   first_reblogged_on;
      comment_id_type                  comment;
      uint32_t                         account_feed_id = 0;
};

typedef oid< feed_object > feed_id_type;


class blog_object : public object< blog_object_type, blog_object >
{
   public:
      template< typename Constructor, typename Allocator >
      blog_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      blog_object() {}

      id_type           id;

      account_name_type account;
      comment_id_type   comment;
      time_point_sec    reblogged_on;
      uint32_t          blog_feed_id = 0;
};
typedef oid< blog_object > blog_id_type;

/**
 *  This index is maintained to get an idea of which authors are resteemed by a particular blogger and
 *  how frequnetly. It is designed to give an overview of the type of people a blogger sponsors as well
 *  as to enable generation of filter set for a blog list.
 *
 *  Give me the top authors promoted by this blog
 *  Give me all blog posts by [authors] that were resteemed by this blog
 */
class blog_author_stats_object : public object< blog_author_stats_object_type, blog_author_stats_object >
{
   public:
      template< typename Constructor, typename Allocator >
      blog_author_stats_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type           id;
      account_name_type blogger;
      account_name_type guest;
      uint32_t          count = 0;
};

typedef oid< blog_author_stats_object > blog_author_stats_id_type;



class reputation_object : public object< reputation_object_type, reputation_object >
{
   public:
      template< typename Constructor, typename Allocator >
      reputation_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      reputation_object() {}

      id_type           id;

      account_name_type account;
      share_type        reputation;
};

typedef oid< reputation_object > reputation_id_type;


class follow_count_object : public object< follow_count_object_type, follow_count_object >
{
   public:
      template< typename Constructor, typename Allocator >
      follow_count_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      follow_count_object() {}

      id_type           id;

      account_name_type account;
      uint32_t          follower_count  = 0;
      uint32_t          following_count = 0;
};

typedef oid< follow_count_object > follow_count_id_type;


struct by_following_follower;
struct by_follower_following;

using namespace boost::multi_index;

typedef multi_index_container<
   follow_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< follow_object, follow_id_type, &follow_object::id > >,
      ordered_unique< tag< by_following_follower >,
         composite_key< follow_object,
            member< follow_object, account_name_type, &follow_object::following >,
            member< follow_object, account_name_type, &follow_object::follower >
         >,
         composite_key_compare< std::less< account_name_type >, std::less< account_name_type > >
      >,
      ordered_unique< tag< by_follower_following >,
         composite_key< follow_object,
            member< follow_object, account_name_type, &follow_object::follower >,
            member< follow_object, account_name_type, &follow_object::following >
         >,
         composite_key_compare< std::less< account_name_type >, std::less< account_name_type > >
      >
   >,
   allocator< follow_object >
> follow_index;

struct by_blogger_guest_count;
typedef chainbase::shared_multi_index_container<
   blog_author_stats_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< blog_author_stats_object, blog_author_stats_id_type, &blog_author_stats_object::id > >,
      ordered_unique< tag< by_blogger_guest_count >,
         composite_key< blog_author_stats_object,
            member< blog_author_stats_object, account_name_type, &blog_author_stats_object::blogger >,
            member< blog_author_stats_object, account_name_type, &blog_author_stats_object::guest >,
            member< blog_author_stats_object, uint32_t, &blog_author_stats_object::count >
         >,
         composite_key_compare< std::less< account_name_type >, std::less< account_name_type >, greater<uint32_t> >
      >
   >
> blog_author_stats_index;

struct by_feed;
struct by_account;
struct by_comment;

typedef multi_index_container<
   feed_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< feed_object, feed_id_type, &feed_object::id > >,
      ordered_unique< tag< by_feed >,
         composite_key< feed_object,
            member< feed_object, account_name_type, &feed_object::account >,
            member< feed_object, uint32_t, &feed_object::account_feed_id >
         >,
         composite_key_compare< std::less< account_name_type >, std::greater< uint32_t > >
      >,
      ordered_unique< tag< by_comment >,
         composite_key< feed_object,
            member< feed_object, comment_id_type, &feed_object::comment >,
            member< feed_object, account_name_type, &feed_object::account >,
            member< feed_object, feed_id_type, &feed_object::id >
         >,
         composite_key_compare< std::less< comment_id_type >, std::less< account_name_type >, std::less< feed_id_type > >
      >
   >,
   allocator< feed_object >
> feed_index;

struct by_blog;

typedef multi_index_container<
   blog_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< blog_object, blog_id_type, &blog_object::id > >,
      ordered_unique< tag< by_blog >,
         composite_key< blog_object,
            member< blog_object, account_name_type, &blog_object::account >,
            member< blog_object, uint32_t, &blog_object::blog_feed_id >
         >,
         composite_key_compare< std::less< account_name_type >, std::greater< uint32_t > >
      >,
      ordered_unique< tag< by_comment >,
         composite_key< blog_object,
            member< blog_object, comment_id_type, &blog_object::comment >,
            member< blog_object, account_name_type, &blog_object::account >,
            member< blog_object, blog_id_type, &blog_object::id >
         >,
         composite_key_compare< std::less< comment_id_type >, std::less< account_name_type >, std::less< blog_id_type > >
      >
   >,
   allocator< blog_object >
> blog_index;

typedef multi_index_container<
   reputation_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< reputation_object, reputation_id_type, &reputation_object::id > >,
      ordered_unique< tag< by_account >, member< reputation_object, account_name_type, &reputation_object::account > >
   >,
   allocator< reputation_object >
> reputation_index;


struct by_followers;
struct by_following;

typedef multi_index_container<
   follow_count_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< follow_count_object, follow_count_id_type, &follow_count_object::id > >,
      ordered_unique< tag< by_account >, member< follow_count_object, account_name_type, &follow_count_object::account > >
   >,
   allocator< follow_count_object >
> follow_count_index;

} } } // steem::plugins::follow

FC_REFLECT_ENUM( steem::plugins::follow::follow_type, (undefined)(blog)(ignore) )

FC_REFLECT( steem::plugins::follow::follow_object, (id)(follower)(following)(what) )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::follow::follow_object, steem::plugins::follow::follow_index )

FC_REFLECT( steem::plugins::follow::feed_object, (id)(account)(first_reblogged_by)(first_reblogged_on)(reblogged_by)(comment)(account_feed_id) )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::follow::feed_object, steem::plugins::follow::feed_index )

FC_REFLECT( steem::plugins::follow::blog_object, (id)(account)(comment)(reblogged_on)(blog_feed_id) )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::follow::blog_object, steem::plugins::follow::blog_index )

FC_REFLECT( steem::plugins::follow::reputation_object, (id)(account)(reputation) )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::follow::reputation_object, steem::plugins::follow::reputation_index )

FC_REFLECT( steem::plugins::follow::follow_count_object, (id)(account)(follower_count)(following_count) )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::follow::follow_count_object, steem::plugins::follow::follow_count_index )

FC_REFLECT( steem::plugins::follow::blog_author_stats_object, (id)(blogger)(guest)(count) )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::follow::blog_author_stats_object, steem::plugins::follow::blog_author_stats_index );

namespace helpers
{
   template <>
   class index_statistic_provider<steem::plugins::follow::feed_index>
   {
   public:
      typedef steem::plugins::follow::feed_index IndexType;
      typedef typename steem::plugins::follow::feed_object::t_reblogged_by_container t_reblogged_by_container;

      index_statistic_info gather_statistics(const IndexType& index, bool onlyStaticInfo) const
      {
         index_statistic_info info;
         gather_index_static_data(index, &info);

         if(onlyStaticInfo == false)
         {
            for(const auto& o : index)
            {
               info._item_additional_allocation += o.reblogged_by.capacity()*sizeof(t_reblogged_by_container::value_type);
            }
         }

         return info;
      }
   };

} /// namespace helpers
