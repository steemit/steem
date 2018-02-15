
#include <golos/plugins/witness/witness.hpp>

#include <golos/chain/database_exceptions.hpp>
#include <golos/chain/account_object.hpp>
#include <golos/chain/steem_objects.hpp>
#include <golos/chain/witness_objects.hpp>
#include <golos/time/time.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <fc/smart_ref_impl.hpp>

#include <memory>
#include <thread>

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using std::string;
using std::vector;

namespace bpo = boost::program_options;

void new_chain_banner(const golos::chain::database &db) {
    std::cerr << "\n"
            "********************************\n"
            "*                              *\n"
            "*   ------- NEW CHAIN ------   *\n"
            "*   -   Welcome to Golos!  -   *\n"
            "*   ------------------------   *\n"
            "*                              *\n"
            "********************************\n"
            "\n";
    return;
}

template<typename T>
T dejsonify(const string &s) {
    return fc::json::from_string(s).as<T>();
}

#define DEFAULT_VALUE_VECTOR(value) default_value({fc::json::to_string(value)}, fc::json::to_string(value))
#define LOAD_VALUE_SET(options, name, container, type) \
            if( options.count(name) ) { \
                  const std::vector<std::string>& ops = options[name].as<std::vector<std::string>>(); \
                  std::transform(ops.begin(), ops.end(), std::inserter(container, container.end()), &dejsonify<type>); \
            }

namespace golos {
    namespace plugins {
        namespace witness_plugin {

            namespace asio = boost::asio;
            namespace posix_time = boost::posix_time;
            namespace system = boost::system;

            struct witness_plugin::impl final {
                impl():
                    p2p_(appbase::app().get_plugin<golos::plugins::p2p::p2p_plugin>()),
                    database_(appbase::app().get_plugin<chain::plugin>().db()),
                    mining_work_(mining_service_),
                    production_timer_(appbase::app().get_io_service()) {

                }

                ~impl(){}

                golos::chain::database& database() {
                    return database_;
                }

                golos::chain::database& database() const {
                    return database_;
                }

                golos::plugins::p2p::p2p_plugin& p2p(){
                    return p2p_;
                };

                golos::plugins::p2p::p2p_plugin& p2p() const {
                    return p2p_;
                };

                golos::plugins::p2p::p2p_plugin& p2p_;

                golos::chain::database& database_;

                void on_applied_block(const signed_block &b);

                void start_mining(const fc::ecc::public_key &pub, const fc::ecc::private_key &pk, const string &name, const signed_block &b);

                void schedule_production_loop();

                block_production_condition::block_production_condition_enum block_production_loop();

                block_production_condition::block_production_condition_enum maybe_produce_block(fc::mutable_variant_object &capture);

                boost::program_options::variables_map _options;
                uint32_t _required_witness_participation = 33 * STEEMIT_1_PERCENT;

                std::atomic<uint64_t> head_block_num_;
                block_id_type head_block_id_ = block_id_type();
                std::atomic<uint64_t> total_hashes_;
                fc::time_point hash_start_time_;

                uint32_t mining_threads_ = 0;
                asio::io_service mining_service_;
                asio::io_service::work mining_work_;
                std::vector<std::unique_ptr<std::thread>> mining_thread_pool_;

                uint32_t _production_skip_flags = golos::chain::database::skip_nothing;
                bool _production_enabled = false;
                asio::deadline_timer production_timer_;

                std::map<public_key_type, fc::ecc::private_key> _private_keys;
                std::set<string> _witnesses;
                std::map<string, public_key_type> _miners;
                protocol::chain_properties _miner_prop_vote;
            };

            void witness_plugin::set_program_options(
                    boost::program_options::options_description &command_line_options,
                    boost::program_options::options_description &config_file_options) {
                    string witness_id_example = "initwitness";

                command_line_options.add_options()
                        ("enable-stale-production",  bpo::value<bool>()->implicit_value(false) , "Enable block production, even if the chain is stale.")
                        ("required-participation", bpo::value<int>()->implicit_value(uint32_t(3 * STEEMIT_1_PERCENT)), "Percent of witnesses (0-99) that must be participating in order to produce blocks")
                        ("witness,w", bpo::value<vector<string>>()->composing()->multitoken(), ("name of witness controlled by this node (e.g. " + witness_id_example + " )").c_str())
                        ("miner,m", bpo::value<vector<string>>()->composing()->multitoken(), "name of miner and its private key (e.g. [\"account\",\"WIF PRIVATE KEY\"] )")
                        ("mining-threads,t", bpo::value<uint32_t>(), "Number of threads to use for proof of work mining")
                        ("private-key", bpo::value<vector<string>>()->composing()->multitoken(), "WIF PRIVATE KEY to be used by one or more witnesses or miners")
                        ("miner-account-creation-fee", bpo::value<uint64_t>()->implicit_value(100000), "Account creation fee to be voted on upon successful POW - Minimum fee is 100.000 STEEM (written as 100000)")
                        ("miner-maximum-block-size", bpo::value<uint32_t>()->implicit_value(131072), "Maximum block size (in bytes) to be voted on upon successful POW - Max block size must be between 128 KB and 750 MB")
                        ("miner-sbd-interest-rate", bpo::value<uint32_t>()->implicit_value(1000), "SBD interest rate to be vote on upon successful POW - Default interest rate is 10% (written as 1000)")
                        ;

                config_file_options.add(command_line_options);
            }

            using std::vector;
            using std::pair;
            using std::string;

            void witness_plugin::plugin_initialize(const boost::program_options::variables_map &options) {
                try {
                    ilog("witness plugin:  plugin_initialize() begin");
                    pimpl = std::make_unique<witness_plugin::impl>();

                    pimpl->total_hashes_.store(0, std::memory_order_relaxed);
                    pimpl->_options = &options;
                    LOAD_VALUE_SET(options, "witness", pimpl->_witnesses, string)
                    edump((pimpl->_witnesses));

                    if (options.count("miner")) {

                        const vector<string> miner_to_wif_pair_strings = options["miner"].as<vector<string>>();
                        for (auto p : miner_to_wif_pair_strings) {
                            auto m = dejsonify<pair<string, string>>(p);
                            idump((m));

                            fc::optional<fc::ecc::private_key> private_key = golos::utilities::wif_to_key(m.second);
                            FC_ASSERT(private_key.valid(), "unable to parse private key");
                            pimpl->_private_keys[private_key->get_public_key()] = *private_key;
                            pimpl->_miners[m.first] = private_key->get_public_key();
                        }
                    }

                    if(options.count("enable-stale-production")){
                        pimpl->_production_enabled = options["enable-stale-production"].as<bool>();
                    }

                    if(options.count("required-participation")){
                        int e = static_cast<int>(options["required-participation"].as<int>());
                        pimpl->_required_witness_participation = uint32_t(e * STEEMIT_1_PERCENT);
                    }


                    if (options.count("mining-threads")) {
                        pimpl->mining_threads_ = std::min(options["mining-threads"].as<uint32_t>(), uint32_t(64));
                        pimpl->mining_thread_pool_.resize(pimpl->mining_threads_);
                        for (uint32_t i = 0; i < pimpl->mining_threads_; ++i) {
                            pimpl-> mining_thread_pool_[i].reset(new std::thread( [&] { pimpl->mining_service_.run(); } ));
                        }
                    }

                    if (options.count("private-key")) {
                        const std::vector<std::string> keys = options["private-key"].as<std::vector<std::string>>();
                        for (const std::string &wif_key : keys) {
                            fc::optional<fc::ecc::private_key> private_key = golos::utilities::wif_to_key(wif_key);
                            FC_ASSERT(private_key.valid(), "unable to parse private key");
                            pimpl->_private_keys[private_key->get_public_key()] = *private_key;
                        }
                    }

                    if (options.count("miner-account-creation-fee")) {
                        const uint64_t account_creation_fee = options["miner-account-creation-fee"].as<uint64_t>();

                        if (account_creation_fee < STEEMIT_MIN_ACCOUNT_CREATION_FEE)
                            wlog("miner-account-creation-fee is below the minimum fee, using minimum instead");
                        else {
                            pimpl->_miner_prop_vote.account_creation_fee.amount = account_creation_fee;
                        }
                    }

                    if (options.count("miner-maximum-block-size")) {
                        const uint32_t maximum_block_size = options["miner-maximum-block-size"].as<uint32_t>();

                        if (maximum_block_size < STEEMIT_MIN_BLOCK_SIZE_LIMIT)
                            wlog("miner-maximum-block-size is below the minimum block size limit, using default of 128 KB instead");
                        else if (maximum_block_size > STEEMIT_MAX_BLOCK_SIZE) {
                            wlog("miner-maximum-block-size is above the maximum block size limit, using maximum of 750 MB instead");
                            pimpl->_miner_prop_vote.maximum_block_size = STEEMIT_MAX_BLOCK_SIZE;
                        } else {
                            pimpl->_miner_prop_vote.maximum_block_size = maximum_block_size;
                        }
                    }

                    if (options.count("miner-sbd-interest-rate")) {
                        pimpl->_miner_prop_vote.sbd_interest_rate = options["miner-sbd-interest-rate"].as<uint32_t>();
                    }

                    ilog("witness plugin:  plugin_initialize() end");
                } FC_LOG_AND_RETHROW()
            }

            void witness_plugin::plugin_startup() {
                try {
                    ilog("witness plugin:  plugin_startup() begin");
                    auto &d = pimpl->database();
                    //Start NTP time client
                    golos::time::now();

                    if (!pimpl->_witnesses.empty()) {
                        ilog("Launching block production for ${n} witnesses.", ("n", pimpl->_witnesses.size()));
                        pimpl->p2p().set_block_production(true);
                        if (pimpl->_production_enabled) {
                            if (d.head_block_num() == 0) {
                                new_chain_banner(d);
                            }
                            pimpl->_production_skip_flags |= golos::chain::database::skip_undo_history_check;
                        }
                        pimpl->schedule_production_loop();
                    } else
                        elog("No witnesses configured! Please add witness names and private keys to configuration.");
                    if (!pimpl->_miners.empty()) {
                        ilog("Starting mining...");
                        d.applied_block.connect([this](const protocol::signed_block &b) { pimpl->on_applied_block(b); });
                    } else {
                        elog("No miners configured! Please add miner names and private keys to configuration.");
                    }
                    ilog("witness plugin:  plugin_startup() end");
                } FC_CAPTURE_AND_RETHROW()
            }

            void witness_plugin::plugin_shutdown() {
                golos::time::shutdown_ntp_time();
                if (pimpl->mining_threads_) {
                    ilog("shutting downing mining threads");
                    pimpl->mining_service_.stop();
                    for (auto &t : pimpl->mining_thread_pool_) {
                        t->join();
                        t.reset();
                    }
                    pimpl->mining_thread_pool_.clear();
                }

                if (!pimpl->_witnesses.empty()) {
                    ilog("shutting downing production timer");
                    pimpl->production_timer_.cancel();
                }
            }

            witness_plugin::witness_plugin() {}

            witness_plugin::~witness_plugin() {}

            void witness_plugin::impl::schedule_production_loop() {
                //Schedule for the next second's tick regardless of chain state
                // If we would wait less than 50ms, wait for the whole second.
                int64_t ntp_microseconds = golos::time::now().time_since_epoch().count();
                int64_t next_microseconds = 1000000 - (ntp_microseconds % 1000000);
                if (next_microseconds < 50000) { // we must sleep for at least 50ms
                    next_microseconds += 1000000;
                }

                production_timer_.expires_from_now( posix_time::microseconds(next_microseconds) );
                production_timer_.async_wait( [this](const system::error_code &) { block_production_loop(); } );
            }

            block_production_condition::block_production_condition_enum witness_plugin::impl::block_production_loop() {
                if (fc::time_point::now() < fc::time_point(STEEMIT_GENESIS_TIME)) {
                    wlog("waiting until genesis time to produce block: ${t}", ("t", STEEMIT_GENESIS_TIME));
                    schedule_production_loop();
                    return block_production_condition::wait_for_genesis;
                }

                block_production_condition::block_production_condition_enum result;
                fc::mutable_variant_object capture;
                try {
                    result = maybe_produce_block(capture);
                }
                catch (const fc::canceled_exception &) {
                    //We're trying to exit. Go ahead and let this one out.
                    throw;
                }
                catch (const golos::chain::unknown_hardfork_exception &e) {
                    // Hit a hardfork that the current node know nothing about, stop production and inform user
                    elog("${e}\nNode may be out of date...", ("e", e.to_detail_string()));
                    throw;
                }
                catch (const fc::exception &e) {
                    elog("Got exception while generating block:\n${e}", ("e", e.to_detail_string()));
                    result = block_production_condition::exception_producing_block;
                }

                switch (result) {
                    case block_production_condition::produced:
                        ilog("Generated block #${n} with timestamp ${t} at time ${c} by ${w}", (capture));
                        break;
                    case block_production_condition::not_synced:
                        // This log-record is commented, because it outputs very often
                        // ilog("Not producing block because production is disabled until we receive a recent block (see: --enable-stale-production)");
                        break;
                    case block_production_condition::not_my_turn:
                        // This log-record is commented, because it outputs very often
                        // ilog("Not producing block because it isn't my turn");
                        break;
                    case block_production_condition::not_time_yet:
                        // This log-record is commented, because it outputs very often
                        // ilog("Not producing block because slot has not yet arrived");
                        break;
                    case block_production_condition::no_private_key:
                        ilog("Not producing block for ${scheduled_witness} because I don't have the private key for ${scheduled_key}",
                             (capture));
                        break;
                    case block_production_condition::low_participation:
                        elog("Not producing block because node appears to be on a minority fork with only ${pct}% witness participation",
                             (capture));
                        break;
                    case block_production_condition::lag:
                        elog("Not producing block because node didn't wake up within 500ms of the slot time.");
                        break;
                    case block_production_condition::consecutive:
                        elog("Not producing block because the last block was generated by the same witness.\nThis node is probably disconnected from the network so block production has been disabled.\nDisable this check with --allow-consecutive option.");
                        break;
                    case block_production_condition::exception_producing_block:
                        elog("Failure when producing block with no transactions");
                        break;
                    case block_production_condition::wait_for_genesis:
                        break;
                }

                schedule_production_loop();
                return result;
            }

            block_production_condition::block_production_condition_enum witness_plugin::impl::maybe_produce_block(fc::mutable_variant_object &capture) {
                auto &db = database();
                fc::time_point now_fine = golos::time::now();
                fc::time_point_sec now = now_fine + fc::microseconds(500000);

                // If the next block production opportunity is in the present or future, we're synced.
                if (!_production_enabled) {
                    if (db.get_slot_time(1) >= now) {
                        _production_enabled = true;
                    } else {
                        return block_production_condition::not_synced;
                    }
                }

                // is anyone scheduled to produce now or one second in the future?
                uint32_t slot = db.get_slot_at_time(now);
                if (slot == 0) {
                    capture("next_time", db.get_slot_time(1));
                    return block_production_condition::not_time_yet;
                }

                //
                // this assert should not fail, because now <= db.head_block_time()
                // should have resulted in slot == 0.
                //
                // if this assert triggers, there is a serious bug in get_slot_at_time()
                // which would result in allowing a later block to have a timestamp
                // less than or equal to the previous block
                //
                assert(now > db.head_block_time());

                string scheduled_witness = db.get_scheduled_witness(slot);
                // we must control the witness scheduled to produce the next block.
                if (_witnesses.find(scheduled_witness) == _witnesses.end()) {
                    capture("scheduled_witness", scheduled_witness);
                    return block_production_condition::not_my_turn;
                }

                const auto &witness_by_name = db.get_index<golos::chain::witness_index>().indices().get<golos::chain::by_name>();
                auto itr = witness_by_name.find(scheduled_witness);

                fc::time_point_sec scheduled_time = db.get_slot_time(slot);
                golos::protocol::public_key_type scheduled_key = itr->signing_key;
                auto private_key_itr = _private_keys.find(scheduled_key);

                if (private_key_itr == _private_keys.end()) {
                    capture("scheduled_witness", scheduled_witness);
                    capture("scheduled_key", scheduled_key);
                    return block_production_condition::no_private_key;
                }

                uint32_t prate = db.witness_participation_rate();
                if (prate < _required_witness_participation) {
                    capture("pct", uint32_t(100 * uint64_t(prate) / STEEMIT_1_PERCENT));
                    return block_production_condition::low_participation;
                }

                if (llabs((scheduled_time - now).count()) > fc::milliseconds(500).count()) {
                    capture("scheduled_time", scheduled_time)("now", now);
                    return block_production_condition::lag;
                }

                int retry = 0;
                do {
                    try {
                        auto block = db.generate_block(
                                scheduled_time,
                                scheduled_witness,
                                private_key_itr->second,
                                _production_skip_flags
                        );
                        capture("n", block.block_num())("t", block.timestamp)("c", now)("w", scheduled_witness);
                        p2p().broadcast_block(block);

                        return block_production_condition::produced;
                    }
                    catch (fc::exception &e) {
                        elog("${e}", ("e", e.to_detail_string()));
                        elog("Clearing pending transactions and attempting again");
                        db.clear_pending();
                        retry++;
                    }
                } while (retry < 2);

                return block_production_condition::exception_producing_block;
            }

/**
 * Every time a block is produced, this method is called. This method will iterate through all
 * mining accounts specified by commandline and for which the private key is known. The first
 * account that isn't already scheduled in the mining queue is selected to mine for the
 * BLOCK_INTERVAL minus 1 second. If a POW is solved or a a new block comes in then the
 * worker will stop early.
 *
 * Work is farmed out to N threads in parallel based upon the value specified on the command line.
 *
 * The miner assumes that the next block will be produced on time and that network propagation
 * will take at least 1 second. This 1 second consists of the time it took to receive the block
 * and how long it will take to broadcast the work. In other words, we assume 0.5s broadcast times
 * and therefore do not even attempt work that cannot be delivered on time.
 */
            void witness_plugin::impl::on_applied_block(const golos::protocol::signed_block &b) {
                try {
                    if (!mining_threads_ || _miners.size() == 0) {
                        return;
                    }
                    auto &db = database();

                    const auto &dgp = db.get_dynamic_global_properties();
                    const uint64_t total_hashes = total_hashes_.load(std::memory_order_acquire);
                    double hps = (total_hashes * 1000000) /
                                 (fc::time_point::now() - hash_start_time_).count();
                    uint64_t i_hps = uint64_t(hps + 0.5);

                    uint32_t summary_target = db.get_pow_summary_target();

                    double target = fc::sha256::inverse_approx_log_32_double(summary_target);
                    static const double max_target = std::ldexp(1.0, 256);

                    double seconds_needed = 0.0;
                    if (i_hps > 0) {
                        double hashes_needed = max_target / target;
                        seconds_needed = hashes_needed / i_hps;
                    }

                    uint64_t minutes_needed = uint64_t(seconds_needed / 60.0 + 0.5);

                    fc::sha256 hash_target;
                    hash_target.set_to_inverse_approx_log_32(summary_target);

                    if (total_hashes > 0)
                        ilog("hash rate: ${x} hps  target: ${t} queue: ${l} estimated time to produce: ${m} minutes",
                             ("x", i_hps)("t", hash_target.str())("m", minutes_needed)("l", dgp.num_pow_witnesses)
                        );

                    for (const auto &miner : _miners) {
                        const auto *w = db.find_witness(miner.first);
                        if (!w || w->pow_worker == 0) {
                            auto miner_pub_key = miner.second; //a.active.key_auths.begin()->first;
                            auto priv_key_itr = _private_keys.find(miner_pub_key);
                            if (priv_key_itr == _private_keys.end()) {
                                continue; /// skipping miner for lack of private key
                            }

                            auto miner_priv_key = priv_key_itr->second;
                            start_mining(miner_pub_key, priv_key_itr->second, miner.first, b);
                            break;
                        } else {
                            // ilog( "Skipping miner ${m} because it is already scheduled to produce a block", ("m",miner) );
                        }
                    } // for miner in miners

                } catch (const fc::exception &e) {
                    ilog("exception thrown while attempting to mine");
                }
            }

            void witness_plugin::impl::start_mining(
                    const fc::ecc::public_key &pub,
                    const fc::ecc::private_key &pk,
                    const string &miner,
                    const golos::protocol::signed_block &b
            ) {
                static const uint64_t seed = fc::time_point::now().time_since_epoch().count();
                static const uint64_t start = fc::city_hash64((const char *) &seed, sizeof(seed));
                auto &db = database();
                const auto head_block_num = b.block_num();
                const auto head_block_time = b.timestamp;
                const auto block_id = b.id();
                const auto stop = head_block_time + fc::seconds(STEEMIT_BLOCK_INTERVAL * 2);
                uint32_t thread_num = 0;
                const uint32_t target = db.get_pow_summary_target();
                const auto &acct_idx = db.get_index<golos::chain::account_index>().indices().get<golos::chain::by_name>();
                auto acct_it = acct_idx.find(miner);
                const bool has_account = (acct_it != acct_idx.end());
                const bool has_hardfork_16 = db.has_hardfork(STEEMIT_HARDFORK_0_16__551);

                head_block_id_ = b.id();
                total_hashes_.store(0, std::memory_order_release);
                head_block_num_.store(head_block_num, std::memory_order_release);
                hash_start_time_ = fc::time_point::now();

                for (uint32_t i = 0; i <= mining_threads_; ++i) {
                    thread_num++; // TODO: why it is incremented two times? second incrementation see after mining_service_.post(...)
                    mining_service_.post( [=]{
                        // hardfork_16: differences with previous version are commented with `hardfork_16`
                        if (has_hardfork_16) {
                            protocol::pow2_operation op;
                            protocol::equihash_pow work; // hardfork_16: changed type of `work`
                            work.input.prev_block = block_id;
                            work.input.worker_account = miner;
                            work.input.nonce = start + thread_num;
                            op.props = _miner_prop_vote;

                            while (true) {
                                if (golos::time::nonblocking_now() > stop) {
                                    // ilog( "stop mining due to time out, nonce: ${n}", ("n",op.nonce) );
                                    return;
                                }
                                if (head_block_num_.load(std::memory_order_acquire) != head_block_num) {
                                    // wlog( "stop mining due new block arrival, nonce: ${n}", ("n",op.nonce));
                                    return;
                                }

                                total_hashes_.fetch_add(1, std::memory_order_relaxed); /// signal other workers to stop
                                work.input.nonce += mining_threads_;
                                work.create(block_id, miner, work.input.nonce);

                                if (work.proof.is_valid() && work.pow_summary < target) { // hardfork_16: added `proof.is_valid()`
                                    protocol::signed_transaction trx;
                                    work.prev_block = head_block_id_; // hardfork_16: added field 'prev_block'
                                    op.work = work;
                                    if (!has_account) {
                                        op.new_owner_key = pub;
                                    }
                                    trx.operations.push_back(op);
                                    trx.ref_block_num = head_block_num;
                                    trx.ref_block_prefix = work.input.prev_block._hash[1];
                                    trx.set_expiration(head_block_time + STEEMIT_MAX_TIME_UNTIL_EXPIRATION);
                                    trx.sign(pk, STEEMIT_CHAIN_ID);
                                    head_block_num_.fetch_add(1, std::memory_order_relaxed);

                                    try {
                                        database().push_transaction(trx);
                                        ilog("Broadcasting Proof of Work for ${miner}", ("miner", miner));
                                        p2p().broadcast_transaction(trx);
                                    } catch (const fc::exception &e) {
                                        // wdump((e.to_detail_string()));
                                    }

                                    return;
                                }
                            }
                        }
                        else { // delete after hardfork 16
                            protocol::pow2_operation op;
                            protocol::pow2 work;
                            work.input.prev_block = block_id;
                            work.input.worker_account = miner;
                            work.input.nonce = start + thread_num;
                            op.props = _miner_prop_vote;
                            while (true) {
                                //  if( ((op.nonce/num_threads) % 1000) == 0 ) idump((op.nonce));
                                if (golos::time::nonblocking_now() > stop) {
                                    // ilog( "stop mining due to time out, nonce: ${n}", ("n",op.nonce) );
                                    return;
                                }
                                if (head_block_num_.load(std::memory_order_acquire) != head_block_num) {
                                    // wlog( "stop mining due new block arrival, nonce: ${n}", ("n",op.nonce));
                                    return;
                                }

                                total_hashes_.fetch_add(1, std::memory_order_relaxed);
                                work.input.nonce += mining_threads_;
                                work.create(block_id, miner, work.input.nonce);
                                if (work.pow_summary < target) {
                                    head_block_num_.fetch_add(1, std::memory_order_relaxed); /// signal other workers to stop
                                    protocol::signed_transaction trx;
                                    op.work = work;
                                    if (!has_account) {
                                        op.new_owner_key = pub;
                                    }
                                    trx.operations.push_back(op);
                                    trx.ref_block_num = head_block_num;
                                    trx.ref_block_prefix = work.input.prev_block._hash[1];
                                    trx.set_expiration(head_block_time + STEEMIT_MAX_TIME_UNTIL_EXPIRATION);
                                    trx.sign(pk, STEEMIT_CHAIN_ID);

                                    try {
                                        database().push_transaction(trx);
                                        ilog("Broadcasting Proof of Work for ${miner}", ("miner", miner));
                                        p2p().broadcast_transaction(trx);
                                    } catch (const fc::exception &e) {
                                        // wdump((e.to_detail_string()));
                                    }

                                    return;
                                }
                            }
                        }
                    });
                    thread_num++; // TODO: second incrementation
                }
            }
        }
    }
}

