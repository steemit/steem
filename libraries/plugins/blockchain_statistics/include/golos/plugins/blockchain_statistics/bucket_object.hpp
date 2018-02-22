#pragma once
#include <appbase/application.hpp>
#include <golos/plugins/chain/plugin.hpp>

#include <golos/chain/steem_object_types.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/program_options.hpp>


namespace golos {
namespace plugins {
namespace blockchain_statistics {

using namespace golos::chain;


#ifndef BLOCKCHAIN_STATISTICS_SPACE_ID
#define BLOCKCHAIN_STATISTICS_SPACE_ID 9
#endif

enum blockchain_statistics_object_type {
    bucket_object_type = (BLOCKCHAIN_STATISTICS_SPACE_ID << 8)
};

struct bucket_object
        : public object<bucket_object_type, bucket_object> {
    template<typename Constructor, typename Allocator>
    bucket_object(Constructor &&c, allocator<Allocator> a) {
        c(*this);
    }

    id_type id;

    fc::time_point_sec open;                                        ///< Open time of the bucket
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
    uint32_t total_pow = 0;                               ///< POW submitted
    uint128_t estimated_hashpower = 0;                     ///< Estimated average hashpower over interval

    std::vector < std::string > get_as_string () {
        std::vector < std::string > result;
        result.push_back("seconds:" + std::to_string(seconds ));
        result.push_back("blocks:" + std::to_string(blocks ));
        result.push_back("bandwidth:" + std::to_string(bandwidth ));
        result.push_back("operations:" + std::to_string(operations ));
        result.push_back("transactions:" + std::to_string(transactions ));
        result.push_back("transfers:" + std::to_string(transfers ));
        result.push_back("steem_transferred:" + std::string(steem_transferred ));
        result.push_back("sbd_transferred:" + std::string(sbd_transferred ));
        result.push_back("sbd_paid_as_interest:" + std::string(sbd_paid_as_interest ));
        result.push_back("paid_accounts_created:" + std::to_string(paid_accounts_created ));
        result.push_back("mined_accounts_created:" + std::to_string(mined_accounts_created ));
        result.push_back("root_comments:" + std::to_string(root_comments ));
        result.push_back("root_comment_edits:" + std::to_string(root_comment_edits ));
        result.push_back("root_comments_deleted:" + std::to_string(root_comments_deleted ));
        result.push_back("replies:" + std::to_string(replies ));
        result.push_back("reply_edits:" + std::to_string(reply_edits ));
        result.push_back("replies_deleted:" + std::to_string(replies_deleted));
        result.push_back("new_root_votes:" + std::to_string(new_root_votes));
        result.push_back("changed_root_votes:" + std::to_string(changed_root_votes));
        result.push_back("new_reply_votes:" + std::to_string(new_reply_votes));
        result.push_back("changed_reply_votes:" + std::to_string(changed_reply_votes));
        result.push_back("payouts:" + std::to_string(payouts));
        result.push_back("sbd_paid_to_authors:" + std::string(sbd_paid_to_authors));
        result.push_back("vests_paid_to_authors:" + std::string(vests_paid_to_authors));
        result.push_back("vests_paid_to_curators:" + std::string(vests_paid_to_curators));
        result.push_back("liquidity_rewards_paid:" + std::string(liquidity_rewards_paid));
        result.push_back("transfers_to_vesting:" + std::to_string(transfers_to_vesting));
        result.push_back("steem_vested:" + std::string(steem_vested));
        result.push_back("new_vesting_withdrawal_requests:" + std::to_string(new_vesting_withdrawal_requests));
        result.push_back("modified_vesting_withdrawal_requests:" + std::to_string(modified_vesting_withdrawal_requests));
        result.push_back("vesting_withdraw_rate_delta:" + std::string(vesting_withdraw_rate_delta));
        result.push_back("vesting_withdrawals_processed:" + std::to_string(vesting_withdrawals_processed));
        result.push_back("finished_vesting_withdrawals:" + std::to_string(finished_vesting_withdrawals));
        result.push_back("vests_withdrawn:" + std::string(vests_withdrawn));
        result.push_back("vests_transferred:" + std::string(vests_transferred));
        result.push_back("sbd_conversion_requests_created:" + std::to_string(sbd_conversion_requests_created));
        result.push_back("sbd_to_be_converted:" + std::string(sbd_to_be_converted));
        result.push_back("sbd_conversion_requests_filled:" + std::to_string(sbd_conversion_requests_filled));
        result.push_back("steem_converted:" + std::string(steem_converted));
        result.push_back("limit_orders_created:" + std::to_string(limit_orders_created));
        result.push_back("limit_orders_filled:" + std::to_string(limit_orders_filled));
        result.push_back("limit_orders_cancelled:" + std::to_string(limit_orders_cancelled));
        result.push_back("total_pow:" + std::to_string(total_pow));
        result.push_back("estimated_hashpower:" + std::string(estimated_hashpower));
        return result;
    }

    std::vector < std::string > calculate_buckets_delta_with (const bucket_object & b) {
        std::vector < std::string > result;
        if (seconds - b.seconds != 0 ) {
            result.push_back("seconds:" + std::to_string(seconds - b.seconds) + "|c");
        }
        if (blocks - b.blocks != 0 ) {
            result.push_back("blocks:" + std::to_string(blocks - b.blocks) + "|c");
        }
        if (bandwidth - b.bandwidth != 0 ) {
            result.push_back("bandwidth:" + std::to_string(bandwidth - b.bandwidth) + "|c");
        }
        if (operations - b.operations != 0 ) {
            result.push_back("operations:" + std::to_string(operations - b.operations) + "|c");
        }
        if (transactions - b.transactions != 0 ) {
            result.push_back("transactions:" + std::to_string(transactions - b.transactions) + "|c");
        }
        if (transfers - b.transfers != 0 ) {
            result.push_back("transfers:" + std::to_string(transfers - b.transfers) + "|c");
        }
        if (steem_transferred - b.steem_transferred != 0 ) {
            result.push_back("steem_transferred:" + std::string(steem_transferred - b.steem_transferred) + "|c");
        }
        if (sbd_transferred - b.sbd_transferred != 0 ) {
            result.push_back("sbd_transferred:" + std::string(sbd_transferred - b.sbd_transferred) + "|c");
        }
        if (sbd_paid_as_interest - b.sbd_paid_as_interest != 0 ) {
            result.push_back("sbd_paid_as_interest:" + std::string(sbd_paid_as_interest - b.sbd_paid_as_interest) + "|c");
        }
        if (paid_accounts_created - b.paid_accounts_created != 0 ) {
            result.push_back("paid_accounts_created:" + std::to_string(paid_accounts_created - b.paid_accounts_created) + "|c");
        }
        if (mined_accounts_created - b.mined_accounts_created != 0 ) {
            result.push_back("mined_accounts_created:" + std::to_string(mined_accounts_created - b.mined_accounts_created) + "|c");
        }
        if (root_comments - b.root_comments != 0 ) {
            result.push_back("root_comments:" + std::to_string(root_comments - b.root_comments) + "|c");
        }
        if (root_comment_edits - b.root_comment_edits != 0 ) {
            result.push_back("root_comment_edits:" + std::to_string(root_comment_edits - b.root_comment_edits) + "|c");
        }
        if (root_comments_deleted - b.root_comments_deleted != 0 ) {
            result.push_back("root_comments_deleted:" + std::to_string(root_comments_deleted - b.root_comments_deleted) + "|c");
        }
        if (replies - b.replies != 0 ) {
            result.push_back("replies:" + std::to_string(replies - b.replies) + "|c");
        }
        if (reply_edits - b.reply_edits != 0 ) {
            result.push_back("reply_edits:" + std::to_string(reply_edits - b.reply_edits) + "|c");
        }
        if (replies_deleted - b.replies_deleted != 0 ) {
            result.push_back("replies_deleted:" + std::to_string(replies_deleted - b.replies_deleted) + "|c");
        }
        if (new_root_votes - b.new_root_votes != 0 ) {
            result.push_back("new_root_votes:" + std::to_string(new_root_votes - b.new_root_votes) + "|c");
        }
        if (changed_root_votes - b.changed_root_votes != 0 ) {
            result.push_back("changed_root_votes:" + std::to_string(changed_root_votes - b.changed_root_votes) + "|c");
        }
        if (new_reply_votes - b.new_reply_votes != 0 ) {
            result.push_back("new_reply_votes:" + std::to_string(new_reply_votes - b.new_reply_votes) + "|c");
        }
        if (changed_reply_votes - b.changed_reply_votes != 0 ) {
            result.push_back("changed_reply_votes:" + std::to_string(changed_reply_votes - b.changed_reply_votes) + "|c");
        }
        if (payouts - b.payouts != 0 ) {
            result.push_back("payouts:" + std::to_string(payouts - b.payouts) + "|c");
        }
        if (sbd_paid_to_authors - b.sbd_paid_to_authors != 0 ) {
            result.push_back("sbd_paid_to_authors:" + std::string(sbd_paid_to_authors - b.sbd_paid_to_authors) + "|c");
        }
        if (vests_paid_to_authors - b.vests_paid_to_authors != 0 ) {
            result.push_back("vests_paid_to_authors:" + std::string(vests_paid_to_authors - b.vests_paid_to_authors) + "|c");
        }
        if (vests_paid_to_curators - b.vests_paid_to_curators != 0 ) {
            result.push_back("vests_paid_to_curators:" + std::string(vests_paid_to_curators - b.vests_paid_to_curators) + "|c");
        }
        if (liquidity_rewards_paid - b.liquidity_rewards_paid != 0 ) {
            result.push_back("liquidity_rewards_paid:" + std::string(liquidity_rewards_paid - b.liquidity_rewards_paid) + "|c");
        }
        if (transfers_to_vesting - b.transfers_to_vesting != 0 ) {
            result.push_back("transfers_to_vesting:" + std::to_string(transfers_to_vesting - b.transfers_to_vesting) + "|c");
        }
        if (steem_vested - b.steem_vested != 0 ) {
            result.push_back("steem_vested:" + std::string(steem_vested - b.steem_vested) + "|c");
        }
        if (new_vesting_withdrawal_requests - b.new_vesting_withdrawal_requests != 0 ) {
            result.push_back("new_vesting_withdrawal_requests:" + std::to_string(new_vesting_withdrawal_requests - b.new_vesting_withdrawal_requests) + "|c");
        }
        if (modified_vesting_withdrawal_requests - b.modified_vesting_withdrawal_requests != 0 ) {
            result.push_back("modified_vesting_withdrawal_requests:" + std::to_string(modified_vesting_withdrawal_requests - b.modified_vesting_withdrawal_requests) + "|c");
        }
        if (vesting_withdraw_rate_delta - b.vesting_withdraw_rate_delta != 0 ) {
            result.push_back("vesting_withdraw_rate_delta:" + std::string(vesting_withdraw_rate_delta - b.vesting_withdraw_rate_delta) + "|c");
        }
        if (vesting_withdrawals_processed - b.vesting_withdrawals_processed != 0 ) {
            result.push_back("vesting_withdrawals_processed:" + std::to_string(vesting_withdrawals_processed - b.vesting_withdrawals_processed) + "|c");
        }
        if (finished_vesting_withdrawals - b.finished_vesting_withdrawals != 0 ) {
            result.push_back("finished_vesting_withdrawals:" + std::to_string(finished_vesting_withdrawals - b.finished_vesting_withdrawals) + "|c");
        }
        if (vests_withdrawn - b.vests_withdrawn != 0 ) {
            result.push_back("vests_withdrawn:" + std::string(vests_withdrawn - b.vests_withdrawn) + "|c");
        }
        if (vests_transferred - b.vests_transferred != 0 ) {
            result.push_back("vests_transferred:" + std::string(vests_transferred - b.vests_transferred) + "|c");
        }
        if (sbd_conversion_requests_created - b.sbd_conversion_requests_created != 0 ) {
            result.push_back("sbd_conversion_requests_created:" + std::to_string(sbd_conversion_requests_created - b.sbd_conversion_requests_created) + "|c");
        }
        if (sbd_to_be_converted - b.sbd_to_be_converted != 0 ) {
            result.push_back("sbd_to_be_converted:" + std::string(sbd_to_be_converted - b.sbd_to_be_converted) + "|c");
        }
        if (sbd_conversion_requests_filled - b.sbd_conversion_requests_filled != 0 ) {
            result.push_back("sbd_conversion_requests_filled:" + std::to_string(sbd_conversion_requests_filled - b.sbd_conversion_requests_filled) + "|c");
        }
        if (steem_converted - b.steem_converted != 0 ) {
            result.push_back("steem_converted:" + std::string(steem_converted - b.steem_converted) + "|c");
        }
        if (limit_orders_created - b.limit_orders_created != 0 ) {
            result.push_back("limit_orders_created:" + std::to_string(limit_orders_created - b.limit_orders_created) + "|c");
        }
        if (limit_orders_filled - b.limit_orders_filled != 0 ) {
            result.push_back("limit_orders_filled:" + std::to_string(limit_orders_filled - b.limit_orders_filled) + "|c");
        }
        if (limit_orders_cancelled - b.limit_orders_cancelled != 0 ) {
            result.push_back("limit_orders_cancelled:" + std::to_string(limit_orders_cancelled - b.limit_orders_cancelled) + "|c");
        }
        if (total_pow - b.total_pow != 0 ) {
            result.push_back("total_pow:" + std::to_string(total_pow - b.total_pow) + "|c");
        }
        if (estimated_hashpower - b.estimated_hashpower != 0 ) {
            result.push_back("estimated_hashpower:" + std::string(estimated_hashpower - b.estimated_hashpower) + "|c");
        }
        return result;
    }

    void operator=(const bucket_object & b) {   
        seconds = b.seconds;
        blocks = b.blocks;
        bandwidth = b.bandwidth;
        operations = b.operations;
        transactions = b.transactions;
        transfers = b.transfers;
        steem_transferred = b.steem_transferred;
        sbd_transferred = b.sbd_transferred;
        sbd_paid_as_interest = b.sbd_paid_as_interest;
        paid_accounts_created = b.paid_accounts_created;
        mined_accounts_created = b.mined_accounts_created;
        root_comments = b.root_comments;
        root_comment_edits = b.root_comment_edits;
        root_comments_deleted = b.root_comments_deleted;
        replies = b.replies;
        reply_edits = b.reply_edits;
        replies_deleted = b.replies_deleted;
        new_root_votes = b.new_root_votes;
        changed_root_votes = b.changed_root_votes;
        new_reply_votes = b.new_reply_votes;
        changed_reply_votes = b.changed_reply_votes;
        payouts = b.payouts;
        sbd_paid_to_authors = b.sbd_paid_to_authors;
        vests_paid_to_authors = b.vests_paid_to_authors;
        vests_paid_to_curators = b.vests_paid_to_curators;
        liquidity_rewards_paid = b.liquidity_rewards_paid;
        transfers_to_vesting = b.transfers_to_vesting;
        steem_vested = b.steem_vested;
        new_vesting_withdrawal_requests = b.new_vesting_withdrawal_requests;
        modified_vesting_withdrawal_requests = b.modified_vesting_withdrawal_requests;
        vesting_withdraw_rate_delta = b.vesting_withdraw_rate_delta;
        vesting_withdrawals_processed = b.vesting_withdrawals_processed;
        finished_vesting_withdrawals = b.finished_vesting_withdrawals;
        vests_withdrawn = b.vests_withdrawn;
        vests_transferred = b.vests_transferred;
        sbd_conversion_requests_created = b.sbd_conversion_requests_created;
        sbd_to_be_converted = b.sbd_to_be_converted;
        sbd_conversion_requests_filled = b.sbd_conversion_requests_filled;
        steem_converted = b.steem_converted;
        limit_orders_created = b.limit_orders_created;
        limit_orders_filled = b.limit_orders_filled;
        limit_orders_cancelled = b.limit_orders_cancelled;
        total_pow = b.total_pow;
        estimated_hashpower = b.estimated_hashpower;
    }
};



typedef object_id<bucket_object> bucket_id_type;

struct by_id;
struct by_bucket;
typedef multi_index_container<
        bucket_object,
        indexed_by<
                ordered_unique<tag<by_id>, member<bucket_object, bucket_id_type, &bucket_object::id>>,
                ordered_unique<tag<by_bucket>,
                        composite_key<bucket_object,
                                member<bucket_object, uint32_t, &bucket_object::seconds>,
                                member<bucket_object, fc::time_point_sec, &bucket_object::open>
                        >
                >
        >,
        allocator<bucket_object>
> bucket_index;

} } } // golos::plugins::blockchain_statistics

FC_REFLECT( (golos::plugins::blockchain_statistics::bucket_object),
    (id)
    (open)
    (seconds)
    (blocks)
    (bandwidth)
    (operations)
    (transactions)
    (transfers)
    (steem_transferred)
    (sbd_transferred)
    (sbd_paid_as_interest)
    (paid_accounts_created)
    (mined_accounts_created)
    (root_comments)
    (root_comment_edits)
    (root_comments_deleted)
    (replies)
    (reply_edits)
    (replies_deleted)
    (new_root_votes)
    (changed_root_votes)
    (new_reply_votes)
    (changed_reply_votes)
    (payouts)
    (sbd_paid_to_authors)
    (vests_paid_to_authors)
    (vests_paid_to_curators)
    (liquidity_rewards_paid)
    (transfers_to_vesting)
    (steem_vested)
    (new_vesting_withdrawal_requests)
    (modified_vesting_withdrawal_requests)
    (vesting_withdraw_rate_delta)
    (vesting_withdrawals_processed)
    (finished_vesting_withdrawals)
    (vests_withdrawn)
    (vests_transferred)
    (sbd_conversion_requests_created)
    (sbd_to_be_converted)
    (sbd_conversion_requests_filled)
    (steem_converted)
    (limit_orders_created)
    (limit_orders_filled)
    (limit_orders_cancelled)
    (total_pow)
    (estimated_hashpower)
)
CHAINBASE_SET_INDEX_TYPE(golos::plugins::blockchain_statistics::bucket_object, golos::plugins::blockchain_statistics::bucket_index)
