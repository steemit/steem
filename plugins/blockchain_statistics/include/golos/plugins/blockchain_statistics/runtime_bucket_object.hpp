#pragma once
#include <appbase/application.hpp>
#include <golos/plugins/chain/plugin.hpp>
#include <golos/chain/steem_object_types.hpp>


namespace golos {
namespace plugins {
namespace blockchain_statistics {

using namespace golos::chain;

struct runtime_bucket_object {

    std::vector < std::string > get_as_string ();
    std::vector < std::string > calculate_delta_with (const runtime_bucket_object & b);
    void operator=(const runtime_bucket_object & b);

    uint32_t seconds = 0;                                 ///< Seconds accounted for in the bucket
    uint32_t blocks = 0;                                  ///< Blocks produced
    uint32_t bandwidth = 0;                               ///< Bandwidth in bytes
    uint32_t operations = 0;                              ///< Operations evaluated
    uint32_t transactions = 0;                            ///< Transactions processed
    uint32_t transfers = 0;                               ///< Account to account transfers
    share_type steem_transferred = 0;                       ///< STEEM transferred from account to account
    share_type sbd_transferred = 0;                         ///< SBD transferred from account to account
    share_type sbd_paid_as_interest = 0;                    ///< SBD paid as interest
    uint32_t paid_accounts_created = 0;                   ///< Accounts created with fee
    uint32_t mined_accounts_created = 0;                  ///< Accounts mined for free
    uint32_t root_comments = 0;                           ///< Top level root comments
    uint32_t root_comment_edits = 0;                      ///< Edits to root comments
    uint32_t root_comments_deleted = 0;                   ///< Root comments deleted
    uint32_t replies = 0;                                 ///< Replies to comments
    uint32_t reply_edits = 0;                             ///< Edits to replies
    uint32_t replies_deleted = 0;                         ///< Replies deleted
    uint32_t new_root_votes = 0;                          ///< New votes on root comments
    uint32_t changed_root_votes = 0;                      ///< Changed votes on root comments
    uint32_t new_reply_votes = 0;                         ///< New votes on replies
    uint32_t changed_reply_votes = 0;                     ///< Changed votes on replies
    uint32_t payouts = 0;                                 ///< Number of comment payouts
    share_type sbd_paid_to_authors = 0;                     ///< Ammount of SBD paid to authors
    share_type vests_paid_to_authors = 0;                   ///< Ammount of VESS paid to authors
    share_type vests_paid_to_curators = 0;                  ///< Ammount of VESTS paid to curators
    share_type liquidity_rewards_paid = 0;                  ///< Ammount of STEEM paid to market makers
    uint32_t transfers_to_vesting = 0;                    ///< Transfers of STEEM into VESTS
    share_type steem_vested = 0;                            ///< Ammount of STEEM vested
    uint32_t new_vesting_withdrawal_requests = 0;         ///< New vesting withdrawal requests
    uint32_t modified_vesting_withdrawal_requests = 0;    ///< Changes to vesting withdrawal requests
    share_type vesting_withdraw_rate_delta = 0;
    uint32_t vesting_withdrawals_processed = 0;           ///< Number of vesting withdrawals
    uint32_t finished_vesting_withdrawals = 0;            ///< Processed vesting withdrawals that are now finished
    share_type vests_withdrawn = 0;                         ///< Ammount of VESTS withdrawn to STEEM
    share_type vests_transferred = 0;                       ///< Ammount of VESTS transferred to another account
    uint32_t sbd_conversion_requests_created = 0;         ///< SBD conversion requests created
    share_type sbd_to_be_converted = 0;                     ///< Amount of SBD to be converted
    uint32_t sbd_conversion_requests_filled = 0;          ///< SBD conversion requests filled
    share_type steem_converted = 0;                         ///< Amount of STEEM that was converted
    uint32_t limit_orders_created = 0;                    ///< Limit orders created
    uint32_t limit_orders_filled = 0;                     ///< Limit orders filled
    uint32_t limit_orders_cancelled = 0;                  ///< Limit orders cancelled
    uint32_t total_pow = 0;                               ///< POW submitte
    uint32_t num_pow_witnesses = 0;                       /// < The current count of how many pending POW witnesses
                                                          /// there are, determines the difficulty of doing pow
};

} } } // golos::plugins::blockchain_statistics
