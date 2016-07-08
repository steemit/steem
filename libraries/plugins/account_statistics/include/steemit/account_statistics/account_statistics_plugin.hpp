#include <steemit/app/plugin.hpp>

#include <graphene/db/generic_index.hpp>

#include <boost/multi_index/composite_key.hpp>

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
#ifndef ACCOUNT_STATISTICS_SPACE_ID
#define ACCOUNT_STATISTICS_SPACE_ID 10
#endif

#ifndef ACCOUNT_STATISTICS_PLUGIN_NAME
#define ACCOUNT_STATISTICS_PLUGIN_NAME "account_stats"
#endif

namespace steemit { namespace account_statistics {

using namespace chain;
using namespace graphene::db;

namespace detail
{
   class account_statistics_plugin_impl;
}

class account_statistics_plugin : public steemit::app::plugin
{
   public:
      account_statistics_plugin();
      virtual ~account_statistics_plugin();

      virtual std::string plugin_name()const override { return ACCOUNT_STATISTICS_PLUGIN_NAME; }
      virtual void plugin_set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg ) override;
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;

      const flat_set< uint32_t >& get_tracked_buckets() const;
      uint32_t get_max_history_per_bucket() const;
      const flat_set< std::string >& get_tracked_accounts() const;

   private:
      friend class detail::account_statistics_plugin_impl;
      std::unique_ptr< detail::account_statistics_plugin_impl > _my;
};

struct account_stats_bucket_object
{
   fc::time_point_sec   open;                                  ///< Open time of the bucket
   uint32_t             seconds = 0;                           ///< Seconds accounted for in the bucket
   string               name;                                  ///< Account name
   uint32_t             market_ops = 0;                        ///< Market operations
   uint32_t             forum_ops = 0;                         ///< Forum operations
   uint32_t             root_comments = 0;                     ///< Top level root comments
   uint32_t             root_comment_edits = 0;                ///< Edits to root comments
   uint32_t             root_comments_deleted = 0;             ///< Root comments deleted
   uint32_t             replies = 0;                           ///< Replies to comments
   uint32_t             reply_edits = 0;                       ///< Edits to replies
   uint32_t             replies_deleted = 0;                   ///< Replies deleted
   uint32_t             new_root_votes = 0;                    ///< New votes on root comments
   uint32_t             changed_root_votes = 0;                ///< Changed votes for root comments
   uint32_t             new_reply_votes = 0;                   ///< New votes on replies
   uint32_t             changed_reply_votes = 0;               ///< Changed votes for replies
   share_type           author_rewards_sbd = 0;                ///< SBD paid for author rewards
   share_type           author_rewards_vests = 0;              ///< VESTS paid for author rewards
   share_type           author_rewards_total_steem_value = 0;  ///< STEEM Value of author rewards
   share_type           curation_rewards_vests = 0;            ///< VESTS paid for curation rewards
   share_type           curation_rewards_steem_value = 0;      ///< STEEM Value of curation rewards
   uint32_t             transfers_to = 0;                      ///< Account to account transfers to this account
   uint32_t             transfers_from = 0;                    ///< Account to account transfers from this account
   share_type           steem_sent = 0;                        ///< STEEM sent from this account
   share_type           steem_received = 0;                    ///< STEEM received by this account
   share_type           sbd_sent = 0;                          ///< SBD sent from this account
   share_type           sbd_received = 0;                      ///< SBD received by this account
   uint32_t             transfers_to_vesting = 0;              ///< Transfers to vesting by this account. Note: Transfer to vesting from A to B counts as a transfer from A to B followed by a vesting deposit by B.
   share_type           steem_vested = 0;                      ///< STEEM vested by the account
   share_type           new_vests = 0;                         ///< New VESTS by vesting transfers
   uint32_t             vesting_withdrawals_processed = 0;     ///< Vesting withdrawals processed for this account
   share_type           vests_withdrawn = 0;                   ///< VESTS withdrawn from the account
   share_type           steem_received_from_withdrawls = 0;    ///< STEEM received from this account's vesting withdrawals
   share_type           steem_received_from_routes = 0;        ///< STEEM received from another account's vesting withdrawals
   share_type           vests_received_from_routes = 0;        ///< VESTS received from another account's vesting withdrawals
   uint32_t             limit_orders_created = 0;              ///< Limit orders created by this account
   uint32_t             limit_orders_filled = 0;               ///< Limit orders filled by this account
   uint32_t             limit_orders_cancelled = 0;            ///< Limit orders cancelled by this account
   share_type           limit_order_steem_paid = 0;            ///< STEEM paid by limit orders
   share_type           limit_order_steem_received = 0;        ///< STEEM received from limit orders
   share_type           limit_order_sbd_paid = 0;              ///< SBD paid by limit orders
   share_type           limit_order_sbd_received = 0;          ///< SBD received by limit orders
   uint32_t             total_pow = 0;                         ///< POW completed
   uint128_t            estimated_hashpower = 0;               ///< Estimated hashpower
};

struct account_activity_bucket_object
{
   fc::time_point_sec   open;                                  ///< Open time for the bucket
   uint32_t             seconds;                               ///< Seconds accounted for in the bucket
   uint32_t             active_market_accounts = 0;            ///< Active market accounts in the bucket
   uint32_t             active_forum_accounts = 0;             ///< Active forum accounts in the bucket
   uint32_t             active_market_and_forum_accounts = 0;  ///< Active accounts in both the market and the forum
};

} } // steemit::account_statistics