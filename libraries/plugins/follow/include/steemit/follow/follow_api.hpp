#pragma once

#include <steemit/app/application.hpp>
#include <steemit/app/steem_api_objects.hpp>

#include <steemit/follow/follow_objects.hpp>

#include <fc/api.hpp>

namespace steemit { namespace follow {

using std::vector;
using std::string;
using app::comment_api_obj;

struct feed_entry
{
   string                    author;
   string                    permlink;
   vector<account_name_type> reblog_by;
   time_point_sec reblog_on;
   uint32_t       entry_id = 0;
};

struct comment_feed_entry
{
   comment_api_obj   comment;
   vector<account_name_type> reblog_by;
   time_point_sec    reblog_on;
   uint32_t          entry_id = 0;
};

struct blog_entry
{
   string         author;
   string         permlink;
   string         blog;
   time_point_sec reblog_on;
   uint32_t entry_id = 0;
};

struct comment_blog_entry
{
   comment_api_obj   comment;
   string            blog;
   time_point_sec    reblog_on;
   uint32_t          entry_id = 0;
};

struct account_reputation
{
   string      account;
   share_type  reputation;
};

struct follow_api_obj
{
   string                follower;
   string                following;
   vector< follow_type > what;
};

struct follow_count_api_obj
{
   follow_count_api_obj() {}
   follow_count_api_obj( const follow_count_object& o ) :
      account( o.account ),
      follower_count( o.follower_count ),
      following_count( o.following_count ) {}

   string   account;
   uint32_t follower_count  = 0;
   uint32_t following_count = 0;
};

namespace detail
{
   class follow_api_impl;
}
/**
 *  Follow api provides an api for retrieving account information concerning the Follow plugin.
 *  This information includes followers pertaining to an account, blogs made by an account, and the reputation of the account.
 *  API calls can be invoked using the @ref custom_json rpc call.
*/
class follow_api
{
   public:
      follow_api( const app::api_context& ctx );

      void on_api_startup();

      /**
       * Returns an vector array of @ref follow_api_obj of each user that is following this account. This includes: Follower, Following, and "what" they're following.
       * If alice creates an account and bob begins to follower her this array will be mapped out like so: \n
       * follow_api_obj followObject = "follow_api_obj" : { \n
       *  0: {"follower":"bob",  \n
       *    "following":"alice",  \n
       *    "what":["blog"]}}  \n
       *
       * @param to The user who's followers you are trying to query
       * @param start The account you wish to begin iterating at, "" for the begining.
       * @param type "what" you wish to query ["undefined", "blog", "ignore"]
       * @return vector< @ref follow_api_obj >
       */
      vector< follow_api_obj > get_followers( string to, string start, follow_type type, uint16_t limit )const;

      /**
       * Returns an vector array of @ref follow_api_obj for accounts that this user is following This includes: Follower, Following, and "what" they're following.
       * If alice creates an account and bob begins to follower her this array will be mapped out like so: \n
       * follow_api_obj followObject = "follow_api_obj" : { \n
       *  0: {"follower":"bob",  \n
       *    "following":"alice",  \n
       *    "what":["blog"]}}  \n
       *
       * @param from The user you are trying to query
       * @param start The account you wish to begin iterating at, "" for the begining.
       * @param type "what" you wish to query ["undefined", "blog", "ignore"]
       * @return vector< @ref follow_api_obj >
       */ //FIXME: Possibly remove documentation duplication here?
      vector< follow_api_obj > get_following( string from, string start, follow_type type, uint16_t limit )const;

      /**
       * Returns the number of followers of a given account.
       *
       * @param account The user who's followers you are trying to count
       * @return vector< @ref follow_count_api_obj >
       */
      follow_count_api_obj get_follow_count( string account )const;

      /**
       * Returns an vector array of @ref feed_obj for each entry that this account has in there feed.
       *
       * @param account The account you are trying to query
       * @param entry_id The entry_id you wish to begin iterating at, 0 for the first.
       * @param limit How many objects you wish to iterate over.
       * @return vector< @ref feed_entry >
       */
      vector< feed_entry > get_feed_entries( string account, uint32_t entry_id = 0, uint16_t limit = 500 )const;

      /**
       * Returns an vector array of @ref comment_feed_obj for each comment feed entry that this account has in there feed.
       *
       * @param account The account you are trying to query
       * @param entry_id The entry_id you wish to begin iterating at, 0 for the first.
       * @param limit How many objects you wish to iterate over.
       * @return vector< @ref feed_entry >
       */
      vector< comment_feed_entry > get_feed( string account, uint32_t entry_id = 0, uint16_t limit = 500 )const;

      /**
       * Returns an vector array of @ref blog_entry for each blog entry that this account has in there feed.
       *
       * @param account The account you are trying to query
       * @param entry_id The entry_id you wish to begin iterating at, 0 for the first.
       * @param limit How many objects you wish to iterate over.
       * @return vector< @ref feed_entry >
       */
      vector< blog_entry > get_blog_entries( string account, uint32_t entry_id = 0, uint16_t limit = 500 )const;

      /**
       * Returns an vector array of @ref comment_blog_entry for each blog entry that this account has in there feed.
       *
       * @param account The account you are trying to query
       * @param entry_id The entry_id you wish to begin iterating at, 0 for the first.
       * @param limit How many objects you wish to iterate over.
       * @return vector< @ref comment_blog_entry >
       */
      vector< comment_blog_entry > get_blog( string account, uint32_t entry_id = 0, uint16_t limit = 500 )const;

      /**
       * Returns an vector array of @account_reputation for each blog entry that this account has in there feed.
       *
       * @param lower_bound_name The account to begin iterating at.
       * @param limit How many objects you wish to iterate over in alphabetical order.
       * @return vector< @ref account_reputation >
       */
      vector< account_reputation > get_account_reputations( string lower_bound_name, uint32_t limit = 1000 )const;

      /**
       * Gets list of accounts that have reblogged a particular post
       */
      vector< account_name_type > get_reblogged_by( const string& author, const string& permlink )const;

      /**
       * Gets a list of authors that have had their content reblogged on a given blog account
       */
      vector< pair< account_name_type, uint32_t > > get_blog_authors( const account_name_type& blog_account )const;

   private:
      std::shared_ptr< detail::follow_api_impl > my;
};

} } // steemit::follow

FC_REFLECT( steemit::follow::feed_entry, (author)(permlink)(reblog_by)(reblog_on)(entry_id) );
FC_REFLECT( steemit::follow::comment_feed_entry, (comment)(reblog_by)(reblog_on)(entry_id) );
FC_REFLECT( steemit::follow::blog_entry, (author)(permlink)(blog)(reblog_on)(entry_id) );
FC_REFLECT( steemit::follow::comment_blog_entry, (comment)(blog)(reblog_on)(entry_id) );
FC_REFLECT( steemit::follow::account_reputation, (account)(reputation) );
FC_REFLECT( steemit::follow::follow_api_obj, (follower)(following)(what) );
FC_REFLECT( steemit::follow::follow_count_api_obj, (account)(follower_count)(following_count) );

FC_API( steemit::follow::follow_api,
   (get_followers)
   (get_following)
   (get_follow_count)
   (get_feed_entries)
   (get_feed)
   (get_blog_entries)
   (get_blog)
   (get_account_reputations)
   (get_reblogged_by)
   (get_blog_authors)
)
