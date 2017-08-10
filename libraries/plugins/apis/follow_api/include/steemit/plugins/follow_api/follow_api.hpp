#pragma once
#include <steemit/plugins/json_rpc/utility.hpp>
#include <steemit/plugins/follow/follow_objects.hpp>
#include <steemit/plugins/database_api/database_api_objects.hpp>

#include <steemit/protocol/types.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>

namespace steemit { namespace plugins { namespace follow {

using steemit::protocol::account_name_type;

namespace detail
{
   class follow_api_impl;
}

struct feed_entry
{
   account_name_type             author;
   string                        permlink;
   vector< account_name_type >   reblog_by;
   time_point_sec                reblog_on;
   uint32_t                      entry_id = 0;
};

struct comment_feed_entry
{
   database_api::api_comment_object comment;
   vector< account_name_type >      reblog_by;
   time_point_sec                   reblog_on;
   uint32_t                         entry_id = 0;
};

struct blog_entry
{
   account_name_type author;
   account_name_type permlink;
   account_name_type blog;
   time_point_sec    reblog_on;
   uint32_t          entry_id = 0;
};

struct comment_blog_entry
{
   database_api::api_comment_object comment;
   string                           blog;
   time_point_sec                   reblog_on;
   uint32_t                         entry_id = 0;
};

struct account_reputation
{
   account_name_type             account;
   steemit::protocol::share_type reputation;
};

struct api_follow_object
{
   account_name_type             follower;
   account_name_type             following;
   vector< follow::follow_type > what;
};

struct reblog_count
{
   account_name_type author;
   uint32_t          count;
};

struct get_followers_args
{
   account_name_type    account;
   account_name_type    start;
   follow::follow_type  type;
   uint32_t             limit = 1000;
};

struct get_followers_return
{
   vector< api_follow_object > followers;
};

typedef get_followers_args get_following_args;

struct get_following_return
{
   vector< api_follow_object > following;
};

struct get_follow_count_args
{
   account_name_type account;
};

struct get_follow_count_return
{
   account_name_type account;
   uint32_t          follower_count = 0;
   uint32_t          following_count = 0;
};

struct get_feed_entries_args
{
   account_name_type account;
   uint32_t          start_entry_id = 0;
   uint32_t          limit = 500;
};

struct get_feed_entries_return
{
   vector< feed_entry > feed;
};

typedef get_feed_entries_args get_feed_args;

struct get_feed_return
{
   vector< comment_feed_entry > feed;
};

typedef get_feed_entries_args get_blog_entries_args;

struct get_blog_entries_return
{
   vector< blog_entry > blog;
};

typedef get_feed_entries_args get_blog_args;

struct get_blog_return
{
   vector< comment_blog_entry > blog;
};

struct get_account_reputations_args
{
   account_name_type account_lower_bound;
   uint32_t          limit = 1000;
};

struct get_account_reputations_return
{
   vector< account_reputation > reputations;
};

struct get_reblogged_by_args
{
   account_name_type author;
   string            permlink;
};

struct get_reblogged_by_return
{
   vector< account_name_type > accounts;
};

struct get_blog_authors_args
{
   account_name_type blog_account;
};

struct get_blog_authors_return
{
   vector< reblog_count > blog_authors;
};

class follow_api
{
   public:
      follow_api();

      DECLARE_API( get_followers )
      DECLARE_API( get_following )
      DECLARE_API( get_follow_count )
      DECLARE_API( get_feed_entries )
      DECLARE_API( get_feed )
      DECLARE_API( get_blog_entries )
      DECLARE_API( get_blog )
      DECLARE_API( get_account_reputations )

      /**
       * Gets list of accounts that have reblogged a particular post
       */
      DECLARE_API( get_reblogged_by )

      /**
       * Gets a list of authors that have had their content reblogged on a given blog account
       */
      DECLARE_API( get_blog_authors )

   private:
      std::shared_ptr< detail::follow_api_impl > my;
};

} } } // steemit::plugins::follow

FC_REFLECT( steemit::plugins::follow::feed_entry,
            (author)(permlink)(reblog_by)(reblog_on)(entry_id) );

FC_REFLECT( steemit::plugins::follow::comment_feed_entry,
            (comment)(reblog_by)(reblog_on)(entry_id) );

FC_REFLECT( steemit::plugins::follow::blog_entry,
            (author)(permlink)(blog)(reblog_on)(entry_id) );

FC_REFLECT( steemit::plugins::follow::comment_blog_entry,
            (comment)(blog)(reblog_on)(entry_id) );

FC_REFLECT( steemit::plugins::follow::account_reputation,
            (account)(reputation) );

FC_REFLECT( steemit::plugins::follow::api_follow_object,
            (follower)(following)(what) );

FC_REFLECT( steemit::plugins::follow::reblog_count,
            (author)(count) );

FC_REFLECT( steemit::plugins::follow::get_followers_args,
            (account)(start)(type)(limit) );

FC_REFLECT( steemit::plugins::follow::get_followers_return,
            (followers) );

FC_REFLECT( steemit::plugins::follow::get_following_return,
            (following) );

FC_REFLECT( steemit::plugins::follow::get_follow_count_args,
            (account) );

FC_REFLECT( steemit::plugins::follow::get_follow_count_return,
            (account)(follower_count)(following_count) );

FC_REFLECT( steemit::plugins::follow::get_feed_entries_args,
            (account)(start_entry_id)(limit) );

FC_REFLECT( steemit::plugins::follow::get_feed_entries_return,
            (feed) );

FC_REFLECT( steemit::plugins::follow::get_feed_return,
            (feed) );

FC_REFLECT( steemit::plugins::follow::get_blog_entries_return,
            (blog) );

FC_REFLECT( steemit::plugins::follow::get_blog_return,
            (blog) );

FC_REFLECT( steemit::plugins::follow::get_account_reputations_args,
            (account_lower_bound)(limit) );

FC_REFLECT( steemit::plugins::follow::get_account_reputations_return,
            (reputations) );

FC_REFLECT( steemit::plugins::follow::get_reblogged_by_args,
            (author)(permlink) );

FC_REFLECT( steemit::plugins::follow::get_reblogged_by_return,
            (accounts) );

FC_REFLECT( steemit::plugins::follow::get_blog_authors_args,
            (blog_account) );

FC_REFLECT( steemit::plugins::follow::get_blog_authors_return,
            (blog_authors) );
