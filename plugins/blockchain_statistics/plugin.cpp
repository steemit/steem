#include <golos/plugins/blockchain_statistics/plugin.hpp>

#include <appbase/application.hpp>

#include <golos/chain/account_object.hpp>
#include <golos/chain/comment_object.hpp>
#include <golos/chain/history_object.hpp>
#include <golos/chain/index.hpp>
#include <golos/chain/operation_notification.hpp>
#include <golos/protocol/block.hpp>
// #include <golos/version/hardfork.hpp>
#include <golos/chain/database.hpp>
#include <fc/io/json.hpp>
#include <boost/program_options.hpp>
#include <golos/plugins/blockchain_statistics/statistics_sender.hpp>



namespace golos {
namespace plugins {
namespace blockchain_statistics {

using namespace golos::protocol;
// struct runtime_bucket_object;

struct plugin::plugin_impl final {
public:
    plugin_impl( ) : database_(appbase::app().get_plugin<chain::plugin>().db()) {
    }

    ~plugin_impl() = default;

    auto database() -> golos::chain::database & {
        return database_;
    }

    void on_block(const signed_block &b);

    void pre_operation(const operation_notification &o);

    void post_operation(const operation_notification &o);

    golos::chain::database &database_;

    std::shared_ptr<statistics_sender> stat_sender;
};

struct operation_process {
    database &_db;
    std::shared_ptr<statistics_sender> stat_sender;

    operation_process(database &db, std::shared_ptr<statistics_sender> stat_sender) : 
        _db(db), stat_sender(stat_sender) {
    }

    typedef void result_type;

    template<typename T>
    void operator()(const T &) const {
    }

    void operator()(const transfer_operation &op) const {
        stat_sender->current_bucket.transfers++;

        if (op.amount.symbol == STEEM_SYMBOL) {
            stat_sender->current_bucket.steem_transferred += op.amount.amount;
        } else {
            stat_sender->current_bucket.sbd_transferred += op.amount.amount;
        }
    }

    void operator()(const interest_operation &op) const {
        stat_sender->current_bucket.sbd_paid_as_interest += op.interest.amount;
    }

    void operator()(const account_create_operation &op) const {
        stat_sender->current_bucket.paid_accounts_created++;
    }

    void operator()(const pow_operation &op) const {
        auto &worker = _db.get_account(op.worker_account);

        if (worker.created == _db.head_block_time()) {
           stat_sender->current_bucket.mined_accounts_created++;
        }

       stat_sender->current_bucket.total_pow++;

        uint64_t bits =
                (_db.get_dynamic_global_properties().num_pow_witnesses /
                 4) + 4;
        uint128_t estimated_hashes = (1 << bits);
        uint32_t delta_t;

        if (stat_sender->current_bucket.seconds == 0) {
            delta_t = _db.head_block_time().sec_since_epoch() -
                        stat_sender->current_bucket.open.sec_since_epoch();
        } else {
             delta_t = stat_sender->current_bucket.seconds;
        }

        stat_sender->current_bucket.estimated_hashpower =
                (stat_sender->current_bucket.estimated_hashpower * delta_t +
                 estimated_hashes) / delta_t;
    }

    void operator()(const comment_operation &op) const {
        auto &comment = _db.get_comment(op.author, op.permlink);

        if (comment.created == _db.head_block_time()) {
            if (comment.parent_author.length()) {
                stat_sender->current_bucket.replies++;
            } else {
                stat_sender->current_bucket.root_comments++;
            }
        } else {
            if (comment.parent_author.length()) {
                stat_sender->current_bucket.reply_edits++;
            } else {
                stat_sender->current_bucket.root_comment_edits++;
            }
        }
    }

    void operator()(const vote_operation &op) const {
        const auto &cv_idx = _db.get_index<comment_vote_index>().indices().get<by_comment_voter>();
        auto &comment = _db.get_comment(op.author, op.permlink);
        auto &voter = _db.get_account(op.voter);
        auto itr = cv_idx.find(boost::make_tuple(comment.id, voter.id));

        if (itr->num_changes) {
            if (comment.parent_author.size()) {
                stat_sender->current_bucket.new_reply_votes++;
            } else {
                stat_sender->current_bucket.new_root_votes++;
            }
        } else {
            if (comment.parent_author.size()) {
                stat_sender->current_bucket.changed_reply_votes++;
            } else {
                stat_sender->current_bucket.changed_root_votes++;
            }
        }
    }

    void operator()(const author_reward_operation &op) const {
        stat_sender->current_bucket.payouts++;
        stat_sender->current_bucket.sbd_paid_to_authors += op.sbd_payout.amount;
        stat_sender->current_bucket.vests_paid_to_authors += op.vesting_payout.amount;
    }

    void operator()(const curation_reward_operation &op) const {
        stat_sender->current_bucket.vests_paid_to_curators += op.reward.amount;
    }

    void operator()(const liquidity_reward_operation &op) const {
        stat_sender->current_bucket.liquidity_rewards_paid += op.payout.amount;
    }

    void operator()(const transfer_to_vesting_operation &op) const {
        stat_sender->current_bucket.transfers_to_vesting++;
        stat_sender->current_bucket.steem_vested += op.amount.amount;        
    }

    void operator()(const fill_vesting_withdraw_operation &op) const {
        auto &account = _db.get_account(op.from_account);

        stat_sender->current_bucket.vesting_withdrawals_processed++;

        if (op.deposited.symbol == STEEM_SYMBOL) {
            stat_sender->current_bucket.vests_withdrawn += op.withdrawn.amount;
        } else {
            stat_sender->current_bucket.vests_transferred += op.withdrawn.amount;
        }

        if (account.vesting_withdraw_rate.amount == 0) {
            stat_sender->current_bucket.finished_vesting_withdrawals++;
        }
    }

    void operator()(const limit_order_create_operation &op) const {
        stat_sender->current_bucket.limit_orders_created++;
    }

    void operator()(const fill_order_operation &op) const {
        stat_sender->current_bucket.limit_orders_filled += 2;
    }

    void operator()(const limit_order_cancel_operation &op) const {
        stat_sender->current_bucket.limit_orders_cancelled++;
    }

    void operator()(const convert_operation &op) const {
        stat_sender->current_bucket.sbd_conversion_requests_created++;
        stat_sender->current_bucket.sbd_to_be_converted += op.amount.amount;
    }

    void operator()(const fill_convert_request_operation &op) const {
        stat_sender->current_bucket.sbd_conversion_requests_filled++;
        stat_sender->current_bucket.steem_converted += op.amount_out.amount;
    }
};

void plugin::plugin_impl::on_block(const signed_block &b) {
    if (b.block_num() == 1) {
        stat_sender->current_bucket.open = b.timestamp;
        stat_sender->current_bucket.seconds = 0;
        stat_sender->current_bucket.blocks = 1;
    } else {
        stat_sender->current_bucket.blocks++;
        
        if (!stat_sender->is_previous_bucket_set) {
            stat_sender->previous_bucket = stat_sender->current_bucket;
            stat_sender->is_previous_bucket_set = true;
        }
        else {
            auto statistics_delta = stat_sender->previous_bucket.calculate_delta_with( stat_sender->current_bucket ); 

            for (auto delta : statistics_delta) {
                stat_sender->push(delta);
            }

            stat_sender->previous_bucket = stat_sender->current_bucket;
        }
    }

    uint32_t trx_size = 0;
    uint32_t num_trx = b.transactions.size();

    for (auto trx : b.transactions) {
        trx_size += fc::raw::pack_size(trx);
    }

    stat_sender->current_bucket.transactions += num_trx;
    stat_sender->current_bucket.bandwidth += trx_size;
}

void plugin::plugin_impl::pre_operation(const operation_notification &o) {
    auto &db = database();

    if (o.op.which() == operation::tag<delete_comment_operation>::value) {

        delete_comment_operation op = o.op.get<delete_comment_operation>();
        auto comment = db.get_comment(op.author, op.permlink);

        if (comment.parent_author.length()) {
            stat_sender->current_bucket.replies_deleted++;
        } else {
            stat_sender->current_bucket.root_comments_deleted++;
        }
    } else if (o.op.which() == operation::tag<withdraw_vesting_operation>::value) {
        withdraw_vesting_operation op = o.op.get<withdraw_vesting_operation>();
        auto &account = db.get_account(op.account);

        auto new_vesting_withdrawal_rate =
                op.vesting_shares.amount /
                STEEMIT_VESTING_WITHDRAW_INTERVALS;

        if (op.vesting_shares.amount > 0 && new_vesting_withdrawal_rate == 0) {
                new_vesting_withdrawal_rate = 1;
        }

        if (!db.has_hardfork(STEEMIT_HARDFORK_0_1)) {
            new_vesting_withdrawal_rate *= 10000;
        }

        if (account.vesting_withdraw_rate.amount > 0) {
            stat_sender->current_bucket.modified_vesting_withdrawal_requests++;
        } else {
            stat_sender->current_bucket.new_vesting_withdrawal_requests++;
        }

        // TODO: Figure out how to change delta when a vesting withdraw finishes. Have until March 24th 2018 to figure that out...
        stat_sender->current_bucket.vesting_withdraw_rate_delta +=
                new_vesting_withdrawal_rate -
                account.vesting_withdraw_rate.amount;
    }
}

void plugin::plugin_impl::post_operation(const operation_notification &o) {
    try {

        if (!is_virtual_operation(o.op)) {
            stat_sender->current_bucket.operations++;
        }
        o.op.visit( operation_process( database(), stat_sender ) );
    } FC_CAPTURE_AND_RETHROW()
}

plugin::plugin() {

}

plugin::~plugin() {
    _my->stat_sender.reset();//TODO: move to plugin_shutdown
}

void plugin::set_program_options( options_description& cli, options_description& cfg ) {
    cli.add_options()
            // ("chain-stats-bucket-size", boost::program_options::value<string>()->default_value("[60,3600,21600,86400,604800,2592000]"),
            //         "Track blockchain statistics by grouping orders into buckets of equal size measured in seconds specified as a JSON array of numbers")
            // ("chain-stats-history-per-bucket", boost::program_options::value<uint32_t>()->default_value(100),
            //         "How far back in time to track history for each bucket size, measured in the number of buckets (default: 100)")
            ("statsd-endpoints", boost::program_options::value<std::vector<std::string>>()->multitoken()->
                    zero_tokens()->composing(), "StatsD endpoints that will receive the statistics in StatsD string format.")
            ("statsd-default-port", boost::program_options::value<uint32_t>()->default_value(8125), "Default port for StatsD nodes.");
    cfg.add(cli);
}

void plugin::plugin_initialize(const boost::program_options::variables_map &options) {
    try {
        ilog("chain_stats_plugin: plugin_initialize() begin");

        _my.reset(new plugin_impl());
        auto &db = _my -> database();

        uint32_t statsd_default_port; // default port(8125) for statsd https://github.com/etsy/statsd

        if (options.count("statsd-default-port")) {
            statsd_default_port = options["statsd-default-port"].as<uint32_t>();
        }
        _my->stat_sender = std::shared_ptr<statistics_sender>(new statistics_sender(statsd_default_port) );
        
        db.applied_block.connect([&](const signed_block &b) {
            _my->on_block(b);
        });
        
        db.pre_apply_operation.connect([&](const operation_notification &o) {
            _my->pre_operation(o);
        });

        db.post_apply_operation.connect([&](const operation_notification &o) {
            _my->post_operation(o);
        });

        if (options.count("statsd-endpoints")) {
            for (auto it : options["statsd-endpoints"].as<std::vector<std::string>>()) {
                _my->stat_sender->add_address(it);
            }
        }

        ilog("chain_stats_plugin: plugin_initialize() end");
    } FC_CAPTURE_AND_RETHROW()
}

void plugin::plugin_startup() {
    ilog("chain_stats plugin: plugin_startup() begin");

    if (_my->stat_sender->can_start()) {
        wlog("chain_stats plugin: statitistics sender was started");
        wlog("StatsD endpoints: ${endpoints}", ( "endpoints", _my->stat_sender->get_endpoint_string_vector() ) );
    }
    else {
        wlog("chain_stats plugin: statitistics sender was not started: no recipient's IPs were provided");
    }

    ilog("chain_stats plugin: plugin_startup() end");
}

void plugin::plugin_shutdown() {
}

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

std::vector < std::string > runtime_bucket_object::calculate_delta_with (const runtime_bucket_object & b) {
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

void runtime_bucket_object::operator=(const runtime_bucket_object & b) {   
    open = b.open;
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
} } } // golos::plugins::blockchain_statistics
