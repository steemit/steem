#pragma once

#include <steemit/follow/follow_plugin.hpp>

#include <steemit/chain/steem_object_types.hpp>

namespace steemit { namespace follow {

using namespace std;
using namespace steemit::chain;

#ifndef FOLLOW_SPACE_ID
#define FOLLOW_SPACE_ID 8
#endif

enum follow_plugin_object_type
{
   follow_object_type     = ( FOLLOW_SPACE_ID << 8 ),
   feed_object_type       = ( FOLLOW_SPACE_ID << 8 ) + 1,
   reputation_object_type = ( FOLLOW_SPACE_ID << 8 ) + 2,
   blog_object_type       = ( FOLLOW_SPACE_ID << 8 ) + 3
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

      account_id_type   follower;
      account_id_type   following;
      uint16_t          what = 0;
};

typedef oid< follow_object > follow_id_type;


class feed_object : public object< feed_object_type, feed_object >
{
   public:
      template< typename Constructor, typename Allocator >
      feed_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      feed_object() {}

      id_type           id;

      account_id_type   account;
      account_id_type   first_reblogged_by;
      time_point_sec    first_reblogged_on;
      comment_id_type   comment;
      uint32_t          reblogs;
      uint32_t          account_feed_id = 0;
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

      account_id_type   account;
      comment_id_type   comment;
      time_point_sec    reblogged_on;
      uint32_t          blog_feed_id = 0;
};

typedef oid< blog_object > blog_id_type;


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

      account_id_type   account;
      share_type        reputation;
};

typedef oid< reputation_object > reputation_id_type;


struct by_following_follower;
struct by_follower_following;

using namespace boost::multi_index;

typedef multi_index_container<
   follow_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< follow_object, follow_id_type, &follow_object::id > >,
      ordered_unique< tag< by_following_follower >,
         composite_key< follow_object,
            member< follow_object, account_id_type, &follow_object::following >,
            member< follow_object, account_id_type, &follow_object::follower >
         >
      >,
      ordered_unique< tag< by_follower_following >,
         composite_key< follow_object,
            member< follow_object, account_id_type, &follow_object::follower >,
            member< follow_object, account_id_type, &follow_object::following >
         >
      >
   >,
   allocator< follow_object >
> follow_index;

struct by_feed;
struct by_old_feed;
struct by_account;
struct by_comment;

typedef multi_index_container<
   feed_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< feed_object, feed_id_type, &feed_object::id > >,
      ordered_unique< tag< by_feed >,
         composite_key< feed_object,
            member< feed_object, account_id_type, &feed_object::account >,
            member< feed_object, uint32_t, &feed_object::account_feed_id >
         >,
         composite_key_compare< std::less< account_id_type >, std::greater< uint32_t > >
      >,
      ordered_unique< tag< by_old_feed >,
         composite_key< feed_object,
            member< feed_object, account_id_type, &feed_object::account >,
            member< feed_object, uint32_t, &feed_object::account_feed_id >
         >,
         composite_key_compare< std::less< account_id_type >, std::less< uint32_t > >
      >,
      ordered_unique< tag< by_account >,
         composite_key< feed_object,
            member< feed_object, account_id_type, &feed_object::account >,
            member< feed_object, feed_id_type, &feed_object::id >
         >
      >,
      ordered_unique< tag< by_comment >,
         composite_key< feed_object,
            member< feed_object, comment_id_type, &feed_object::comment >,
            member< feed_object, account_id_type, &feed_object::account >
         >
      >
   >,
   allocator< feed_object >
> feed_index;

struct by_blog;
struct by_old_blog;

typedef multi_index_container<
   blog_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< blog_object, blog_id_type, &blog_object::id > >,
      ordered_unique< tag< by_blog >,
         composite_key< blog_object,
            member< blog_object, account_id_type, &blog_object::account >,
            member< blog_object, uint32_t, &blog_object::blog_feed_id >
         >,
         composite_key_compare< std::less< account_id_type >, std::greater< uint32_t > >
      >,
      ordered_unique< tag< by_old_blog >,
         composite_key< blog_object,
            member< blog_object, account_id_type, &blog_object::account >,
            member< blog_object, uint32_t, &blog_object::blog_feed_id >
         >,
         composite_key_compare< std::less< account_id_type >, std::less< uint32_t > >
      >,
      ordered_unique< tag< by_comment >,
         composite_key< blog_object,
            member< blog_object, comment_id_type, &blog_object::comment >,
            member< blog_object, account_id_type, &blog_object::account >
         >
      >
   >,
   allocator< blog_object >
> blog_index;

struct by_reputation;

typedef multi_index_container<
   reputation_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< reputation_object, reputation_id_type, &reputation_object::id > >,
      ordered_unique< tag< by_reputation >,
         composite_key< reputation_object,
            member< reputation_object, share_type, &reputation_object::reputation >,
            member< reputation_object, account_id_type, &reputation_object::account >
         >,
         composite_key_compare< std::greater< share_type >, std::less< account_id_type > >
      >,
      ordered_unique< tag< by_account >, member< reputation_object, account_id_type, &reputation_object::account > >
   >,
   allocator< reputation_object >
> reputation_index;

} } // steemit::follow

FC_REFLECT_ENUM( steemit::follow::follow_type, (undefined)(blog)(ignore) )

FC_REFLECT( steemit::follow::follow_object, (id)(follower)(following)(what) )
CHAINBASE_SET_INDEX_TYPE( steemit::follow::follow_object, steemit::follow::follow_index )

FC_REFLECT( steemit::follow::feed_object, (id)(account)(first_reblogged_by)(first_reblogged_on)(comment)(reblogs)(account_feed_id) )
CHAINBASE_SET_INDEX_TYPE( steemit::follow::feed_object, steemit::follow::feed_index )

FC_REFLECT( steemit::follow::blog_object, (id)(account)(comment)(reblogged_on)(blog_feed_id) )
CHAINBASE_SET_INDEX_TYPE( steemit::follow::blog_object, steemit::follow::blog_index )

FC_REFLECT( steemit::follow::reputation_object, (id)(account)(reputation) )
CHAINBASE_SET_INDEX_TYPE( steemit::follow::reputation_object, steemit::follow::reputation_index )
