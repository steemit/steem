#pragma once

#include <steemit/blockchain_statistics/blockchain_statistics_plugin.hpp>

#include <fc/api.hpp>

namespace steemit { namespace app {
   struct api_context;
} }

namespace steemit { namespace blockchain_statistics {

namespace detail
{
   class blockchain_statistics_api_impl;
}

struct statistics
{
   uint32_t             blocks = 0;
   uint32_t             operations = 0;
   uint32_t             transactions = 0;
   uint32_t             transfers = 0;
   share_type           steem_transferred = 0;
   share_type           sbd_transferred = 0;
   share_type           sbd_paid_as_interest = 0;
   uint32_t             accounts_created = 0;
   uint32_t             total_posts = 0;
   uint32_t             top_level_posts = 0;
   uint32_t             replies = 0;
   uint32_t             posts_modified = 0;
   uint32_t             posts_deleted = 0;
   uint32_t             total_votes = 0;
   uint32_t             new_votes = 0;
   uint32_t             changed_votes = 0;
   uint32_t             payouts = 0;
   share_type           sbd_paid_to_comments = 0;
   share_type           vests_paid_to_comments = 0;
   share_type           vests_paid_to_curators = 0;
   share_type           liquidity_rewards_paid = 0;
   uint32_t             transfers_to_vesting = 0;
   share_type           steem_vested = 0;
   uint32_t             vesting_withdrawals_processed = 0;
   share_type           vests_withdrawn = 0;
   uint32_t             limit_orders_created = 0;
   uint32_t             limit_orders_filled = 0;
   uint32_t             limit_orders_cancelled = 0;
   uint32_t             total_pow = 0;
   uint128_t            estimated_hashpower = 0;

   statistics& operator += ( const bucket_object& b );
};

struct account_activity
{
   account_activity() {}
   account_activity( const account_activity_object& aao )
      :account_name( aao.account_name ),
       last_activity( aao.last_activity ),
       last_forum_activity( aao.last_forum_activity ),
       ops_per_day( aao.ops_per_day ),
       forum_ops_per_day( aao.forum_ops_per_day ),
       num_ops( aao.num_ops ),
       num_forum_ops( aao.num_forum_ops ) {}

   string               account_name;
   fc::time_point_sec   last_activity;
   fc::time_point_sec   last_forum_activity;
   double               ops_per_day;
   double               forum_ops_per_day;
   uint32_t             num_ops;
   uint32_t             num_forum_ops;

   account_activity operator()( const account_activity_object& aao )
   {
      return account_activity( aao );
   }
};

class blockchain_statistics_api
{
   public:
      blockchain_statistics_api( const steemit::app::api_context& ctx );

      void on_api_startup();

      /**
      * @brief Gets statistics over the time window length, interval, that contains time, open.
      * @param open The opening time, or a time contained within the window.
      * @param interval The size of the window for which statistics were aggregated.
      * @returns Statistics for the window.
      */
      statistics get_stats_for_time( fc::time_point_sec open, uint32_t interval )const;

      /**
      * @brief Aggregates statistics over a time interval.
      * @param start The beginning time of the window.
      * @param stop The end time of the window. stop must take place after start.
      * @returns Aggregated statistics over the interval.
      */
      statistics get_stats_for_interval( fc::time_point_sec start, fc::time_point_sec end )const;

      /**
       * @brief Returns lifetime statistics.
       */
      statistics get_lifetime_stats()const;

      vector< account_activity > get_account_activity_by_name( string start, string end, uint32_t limit )const;

      vector< account_activity > get_account_activity_by_last_activity( uint32_t limit )const;

      vector< account_activity > get_account_activity_by_longest_inactive( uint32_t limit )const;

      vector< account_activity > get_account_activity_by_most_active( uint32_t limit )const;

      vector< account_activity > get_account_activity_by_least_active( uint32_t limit )const;

      vector< account_activity > get_account_activity_by_most_ops( uint32_t limit )const;

      vector< account_activity > get_account_activity_by_least_ops( uint32_t limit )const;

      vector< account_activity > get_account_forum_activity_by_last_activity( uint32_t limit )const;

      vector< account_activity > get_account_forum_activity_by_longest_inactive( uint32_t limit )const;

      vector< account_activity > get_account_forum_activity_by_most_active( uint32_t limit )const;

      vector< account_activity > get_account_forum_activity_by_least_active( uint32_t limit )const;

      vector< account_activity > get_account_forum_activity_by_most_ops( uint32_t limit )const;

      vector< account_activity > get_account_forum_activity_by_least_ops( uint32_t limit )const;

   private:
      std::shared_ptr< detail::blockchain_statistics_api_impl > my;
};

} } // steemit::blockchain_statistics

FC_REFLECT( steemit::blockchain_statistics::statistics,
   (blocks)
   (operations)
   (transactions)
   (transfers)
   (steem_transferred)
   (sbd_transferred)
   (sbd_paid_as_interest)
   (accounts_created)
   (total_posts)
   (top_level_posts)
   (replies)
   (posts_modified)
   (posts_deleted)
   (total_votes)
   (new_votes)
   (changed_votes)
   (payouts)
   (sbd_paid_to_comments)
   (vests_paid_to_comments)
   (vests_paid_to_curators)
   (liquidity_rewards_paid)
   (transfers_to_vesting)
   (steem_vested)
   (vesting_withdrawals_processed)
   (vests_withdrawn)
   (limit_orders_created)
   (limit_orders_filled)
   (limit_orders_cancelled)
   (total_pow)
   (estimated_hashpower) )

FC_REFLECT( steemit::blockchain_statistics::account_activity,
   (account_name)
   (last_activity)
   (last_forum_activity)
   (ops_per_day)
   (forum_ops_per_day)
   (num_ops)
   (num_forum_ops) )

FC_API( steemit::blockchain_statistics::blockchain_statistics_api,
   (get_stats_for_time)
   (get_stats_for_interval)
   (get_lifetime_stats)
   (get_account_activity_by_name)
   (get_account_activity_by_last_activity)
   (get_account_activity_by_longest_inactive)
   (get_account_activity_by_most_active)
   (get_account_activity_by_least_active)
   (get_account_activity_by_most_ops)
   (get_account_activity_by_least_ops)
   (get_account_forum_activity_by_last_activity)
   (get_account_forum_activity_by_longest_inactive)
   (get_account_forum_activity_by_most_active)
   (get_account_forum_activity_by_least_active)
   (get_account_forum_activity_by_most_ops)
   (get_account_forum_activity_by_least_ops)
)
