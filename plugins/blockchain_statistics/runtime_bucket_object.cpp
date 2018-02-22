#include <golos/plugins/blockchain_statistics/bucket_object.hpp>
#include <golos/plugins/blockchain_statistics/runtime_bucket_object.hpp>

namespace golos {
namespace plugins {
namespace blockchain_statistics {
   
std::vector < std::string > runtime_bucket_object::get_as_string () {
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

std::vector < std::string > runtime_bucket_object::calculate_delta_with (const bucket_object & b) {
    std::vector < std::string > result;
    if (b.seconds - seconds != 0 ) {
        result.push_back("seconds:" + std::to_string(b.seconds - seconds) + "|c");
    }
    if (b.blocks - blocks != 0 ) {
        result.push_back("blocks:" + std::to_string(b.blocks - blocks) + "|c");
    }
    if (b.bandwidth - bandwidth != 0 ) {
        result.push_back("bandwidth:" + std::to_string(b.bandwidth - bandwidth) + "|c");
    }
    if (b.operations - operations != 0 ) {
        result.push_back("operations:" + std::to_string(b.operations - operations) + "|c");
    }
    if (b.transactions - transactions != 0 ) {
        result.push_back("transactions:" + std::to_string(b.transactions - transactions) + "|c");
    }
    if (b.transfers - transfers != 0 ) {
        result.push_back("transfers:" + std::to_string(b.transfers - transfers) + "|c");
    }
    if (b.steem_transferred - steem_transferred != 0 ) {
        result.push_back("steem_transferred:" + std::string(b.steem_transferred - steem_transferred) + "|c");
    }
    if (b.sbd_transferred - sbd_transferred != 0 ) {
        result.push_back("sbd_transferred:" + std::string(b.sbd_transferred - sbd_transferred) + "|c");
    }
    if (b.sbd_paid_as_interest - sbd_paid_as_interest != 0 ) {
        result.push_back("sbd_paid_as_interest:" + std::string(b.sbd_paid_as_interest - sbd_paid_as_interest) + "|c");
    }
    if (b.paid_accounts_created - paid_accounts_created != 0 ) {
        result.push_back("paid_accounts_created:" + std::to_string(b.paid_accounts_created - paid_accounts_created) + "|c");
    }
    if (b.mined_accounts_created - mined_accounts_created != 0 ) {
        result.push_back("mined_accounts_created:" + std::to_string(b.mined_accounts_created - mined_accounts_created) + "|c");
    }
    if (b.root_comments - root_comments != 0 ) {
        result.push_back("root_comments:" + std::to_string(b.root_comments - root_comments) + "|c");
    }
    if (b.root_comment_edits - root_comment_edits != 0 ) {
        result.push_back("root_comment_edits:" + std::to_string(b.root_comment_edits - root_comment_edits) + "|c");
    }
    if (b.root_comments_deleted - root_comments_deleted != 0 ) {
        result.push_back("root_comments_deleted:" + std::to_string(b.root_comments_deleted - root_comments_deleted) + "|c");
    }
    if (b.replies - replies != 0 ) {
        result.push_back("replies:" + std::to_string(b.replies - replies) + "|c");
    }
    if (b.reply_edits - reply_edits != 0 ) {
        result.push_back("reply_edits:" + std::to_string(b.reply_edits - reply_edits) + "|c");
    }
    if (b.replies_deleted - replies_deleted != 0 ) {
        result.push_back("replies_deleted:" + std::to_string(b.replies_deleted - replies_deleted) + "|c");
    }
    if (b.new_root_votes - new_root_votes != 0 ) {
        result.push_back("new_root_votes:" + std::to_string(b.new_root_votes - new_root_votes) + "|c");
    }
    if (b.changed_root_votes - changed_root_votes != 0 ) {
        result.push_back("changed_root_votes:" + std::to_string(b.changed_root_votes - changed_root_votes) + "|c");
    }
    if (b.new_reply_votes - new_reply_votes != 0 ) {
        result.push_back("new_reply_votes:" + std::to_string(b.new_reply_votes - new_reply_votes) + "|c");
    }
    if (b.changed_reply_votes - changed_reply_votes != 0 ) {
        result.push_back("changed_reply_votes:" + std::to_string(b.changed_reply_votes - changed_reply_votes) + "|c");
    }
    if (b.payouts - payouts != 0 ) {
        result.push_back("payouts:" + std::to_string(b.payouts - payouts) + "|c");
    }
    if (b.sbd_paid_to_authors - sbd_paid_to_authors != 0 ) {
        result.push_back("sbd_paid_to_authors:" + std::string(b.sbd_paid_to_authors - sbd_paid_to_authors) + "|c");
    }
    if (b.vests_paid_to_authors - vests_paid_to_authors != 0 ) {
        result.push_back("vests_paid_to_authors:" + std::string(b.vests_paid_to_authors - vests_paid_to_authors) + "|c");
    }
    if (b.vests_paid_to_curators - vests_paid_to_curators != 0 ) {
        result.push_back("vests_paid_to_curators:" + std::string(b.vests_paid_to_curators - vests_paid_to_curators) + "|c");
    }
    if (b.liquidity_rewards_paid - liquidity_rewards_paid != 0 ) {
        result.push_back("liquidity_rewards_paid:" + std::string(b.liquidity_rewards_paid - liquidity_rewards_paid) + "|c");
    }
    if (b.transfers_to_vesting - transfers_to_vesting != 0 ) {
        result.push_back("transfers_to_vesting:" + std::to_string(b.transfers_to_vesting - transfers_to_vesting) + "|c");
    }
    if (b.steem_vested - steem_vested != 0 ) {
        result.push_back("steem_vested:" + std::string(b.steem_vested - steem_vested) + "|c");
    }
    if (b.new_vesting_withdrawal_requests - new_vesting_withdrawal_requests != 0 ) {
        result.push_back("new_vesting_withdrawal_requests:" + std::to_string(b.new_vesting_withdrawal_requests - new_vesting_withdrawal_requests) + "|c");
    }
    if (b.modified_vesting_withdrawal_requests - modified_vesting_withdrawal_requests != 0 ) {
        result.push_back("modified_vesting_withdrawal_requests:" + std::to_string(b.modified_vesting_withdrawal_requests - modified_vesting_withdrawal_requests) + "|c");
    }
    if (b.vesting_withdraw_rate_delta - vesting_withdraw_rate_delta != 0 ) {
        result.push_back("vesting_withdraw_rate_delta:" + std::string(b.vesting_withdraw_rate_delta - vesting_withdraw_rate_delta) + "|c");
    }
    if (b.vesting_withdrawals_processed - vesting_withdrawals_processed != 0 ) {
        result.push_back("vesting_withdrawals_processed:" + std::to_string(b.vesting_withdrawals_processed - vesting_withdrawals_processed) + "|c");
    }
    if (b.finished_vesting_withdrawals - finished_vesting_withdrawals != 0 ) {
        result.push_back("finished_vesting_withdrawals:" + std::to_string(b.finished_vesting_withdrawals - finished_vesting_withdrawals) + "|c");
    }
    if (b.vests_withdrawn - vests_withdrawn != 0 ) {
        result.push_back("vests_withdrawn:" + std::string(b.vests_withdrawn - vests_withdrawn) + "|c");
    }
    if (b.vests_transferred - vests_transferred != 0 ) {
        result.push_back("vests_transferred:" + std::string(b.vests_transferred - vests_transferred) + "|c");
    }
    if (b.sbd_conversion_requests_created - sbd_conversion_requests_created != 0 ) {
        result.push_back("sbd_conversion_requests_created:" + std::to_string(b.sbd_conversion_requests_created - sbd_conversion_requests_created) + "|c");
    }
    if (b.sbd_to_be_converted - sbd_to_be_converted != 0 ) {
        result.push_back("sbd_to_be_converted:" + std::string(b.sbd_to_be_converted - sbd_to_be_converted) + "|c");
    }
    if (b.sbd_conversion_requests_filled - sbd_conversion_requests_filled != 0 ) {
        result.push_back("sbd_conversion_requests_filled:" + std::to_string(b.sbd_conversion_requests_filled - sbd_conversion_requests_filled) + "|c");
    }
    if (b.steem_converted - steem_converted != 0 ) {
        result.push_back("steem_converted:" + std::string(b.steem_converted - steem_converted) + "|c");
    }
    if (b.limit_orders_created - limit_orders_created != 0 ) {
        result.push_back("limit_orders_created:" + std::to_string(b.limit_orders_created - limit_orders_created) + "|c");
    }
    if (b.limit_orders_filled - limit_orders_filled != 0 ) {
        result.push_back("limit_orders_filled:" + std::to_string(b.limit_orders_filled - limit_orders_filled) + "|c");
    }
    if (b.limit_orders_cancelled - limit_orders_cancelled != 0 ) {
        result.push_back("limit_orders_cancelled:" + std::to_string(b.limit_orders_cancelled - limit_orders_cancelled) + "|c");
    }
    if (b.total_pow - total_pow != 0 ) {
        result.push_back("total_pow:" + std::to_string(b.total_pow - total_pow) + "|c");
    }
    if (b.estimated_hashpower - estimated_hashpower != 0 ) {
        result.push_back("estimated_hashpower:" + std::string(b.estimated_hashpower - estimated_hashpower) + "|c");
    }
    return result;
}

void runtime_bucket_object::operator=(const bucket_object & b) {   
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
} } }