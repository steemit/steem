#include <boost/test/unit_test.hpp>
#include <boost/program_options.hpp>

#include <golos/time/time.hpp>
#include <graphene/utilities/tempdir.hpp>

#include <golos/chain/steem_objects.hpp>
#include <golos/chain/history_object.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/smart_ref_impl.hpp>

#include "database_fixture.hpp"

//using namespace golos::chain::test;

uint32_t STEEMIT_TESTING_GENESIS_TIMESTAMP = 1431700000;

namespace golos { namespace chain {

        using std::cout;
        using std::cerr;
        using namespace golos::plugins;
        using golos::plugins::json_rpc::msg_pack;


        database_fixture::~database_fixture() {
            if (db_plugin) {
                // clear all debug updates
                db_plugin->plugin_shutdown();
            }

            close_database();
        }

        clean_database_fixture::clean_database_fixture() {
            try {
                initialize();

                open_database();

                startup();
            } catch (const fc::exception &e) {
                edump((e.to_detail_string()));
                throw;
            }
        }

        clean_database_fixture::~clean_database_fixture() {
        }

        void clean_database_fixture::resize_shared_mem(uint64_t size) {
            db->wipe(data_dir->path(), data_dir->path(), true);
            int argc = boost::unit_test::framework::master_test_suite().argc;
            char **argv = boost::unit_test::framework::master_test_suite().argv;
            for (int i = 1; i < argc; i++) {
                const std::string arg = argv[i];
                if (arg == "--record-assert-trip") {
                    fc::enable_record_assert_trip = true;
                }
                if (arg == "--show-test-names") {
                    std::cout << "running test "
                              << boost::unit_test::framework::current_test_case().p_name
                              << std::endl;
                }
            }
            init_account_pub_key = init_account_priv_key.get_public_key();

            db->open(data_dir->path(), data_dir->path(), INITIAL_TEST_SUPPLY, size, chainbase::database::read_write);
            startup();
        }

        live_database_fixture::live_database_fixture() {
            try {
                ilog("Loading saved chain");
                _chain_dir = fc::current_path() / "test_blockchain";
                FC_ASSERT(fc::exists(_chain_dir), "Requires blockchain to test on in ./test_blockchain");

                initialize();

                db->open(_chain_dir, _chain_dir);
                golos::time::now();

                validate_database();
                generate_block();

                ilog("Done loading saved chain");
            }
            FC_LOG_AND_RETHROW()
        }

        live_database_fixture::~live_database_fixture() {
            try {
                db->pop_block();
                return;
            }
            FC_LOG_AND_RETHROW()
        }

        fc::ecc::private_key database_fixture::generate_private_key(string seed) {
            return fc::ecc::private_key::regenerate(fc::sha256::hash(seed));
        }

        string database_fixture::generate_anon_acct_name() {
            // names of the form "anon-acct-x123" ; the "x" is necessary
            //    to workaround issue #46
            return "anon-acct-x" + std::to_string(anon_acct_count++);
        }

        void database_fixture::initialize() {
            int argc = boost::unit_test::framework::master_test_suite().argc;
            char **argv = boost::unit_test::framework::master_test_suite().argv;
            for (int i = 1; i < argc; i++) {
                const std::string arg = argv[i];
                if (arg == "--record-assert-trip") {
                    fc::enable_record_assert_trip = true;
                }
                if (arg == "--show-test-names") {
                    std::cout << "running test "
                              << boost::unit_test::framework::current_test_case().p_name
                              << std::endl;
                }
            }

            ch_plugin = &appbase::app().register_plugin<golos::plugins::chain::plugin>();
            ah_plugin = &appbase::app().register_plugin<account_history::plugin>();
            db_plugin = &appbase::app().register_plugin<debug_node::plugin>();

            appbase::app().initialize<
                    golos::plugins::chain::plugin,
                    account_history::plugin,
                    debug_node::plugin
            >( argc, argv );

            db_plugin->set_logging(false);

            db = &ch_plugin->db();
            BOOST_REQUIRE( db );
        }

        void database_fixture::startup(bool generate_hardfork) {
            generate_block();
            if (generate_hardfork) {
                db->set_hardfork(STEEMIT_NUM_HARDFORKS);
            }
            generate_block();

            vest(STEEMIT_INIT_MINER_NAME, 10000);

            // Fill up the rest of the required miners
            for (int i = STEEMIT_NUM_INIT_MINERS;
                 i < STEEMIT_MAX_WITNESSES; i++) {
                account_create(STEEMIT_INIT_MINER_NAME + fc::to_string(i), init_account_pub_key);
                fund(STEEMIT_INIT_MINER_NAME + fc::to_string(i), STEEMIT_MIN_PRODUCER_REWARD.amount.value);
                witness_create(STEEMIT_INIT_MINER_NAME + fc::to_string(i),
                               init_account_priv_key, "foo.bar", init_account_pub_key,
                               STEEMIT_MIN_PRODUCER_REWARD.amount);
            }

            ah_plugin->plugin_startup();
            db_plugin->plugin_startup();

            validate_database();
        }

        void database_fixture::open_database() {
            if (!data_dir) {
                data_dir = fc::temp_directory(golos::utilities::temp_directory_path());
                db->_log_hardforks = false;
                db->open(data_dir->path(), data_dir->path(), INITIAL_TEST_SUPPLY,
                        1024 * 1024 *
                        8, chainbase::database::read_write); // 8 MB file for testing
            }
        }

        void database_fixture::close_database() {
            if (data_dir) {
                db->wipe(data_dir->path(), data_dir->path(), true);
            }
        }

        void database_fixture::generate_block(uint32_t skip, const fc::ecc::private_key &key, int miss_blocks) {
            skip |= default_skip;
            msg_pack msg;
            msg.args = std::vector<fc::variant>({fc::variant(golos::utilities::key_to_wif(key)), fc::variant(1), fc::variant(skip), fc::variant(miss_blocks), fc::variant(true)});
            db_plugin->debug_generate_blocks(msg);
        }

        void database_fixture::generate_blocks(uint32_t block_count) {
            msg_pack msg;
            msg.args = std::vector<fc::variant>({fc::variant(debug_key), fc::variant(block_count), fc::variant(default_skip), fc::variant(0), fc::variant(true)});
            auto produced = db_plugin->debug_generate_blocks(msg);
            BOOST_REQUIRE(produced == block_count);
        }

        void database_fixture::generate_blocks(fc::time_point_sec timestamp, bool miss_intermediate_blocks) {
            msg_pack msg;
            msg.args = std::vector<fc::variant>({fc::variant(debug_key), fc::variant(timestamp), fc::variant(miss_intermediate_blocks), fc::variant(default_skip)});
            db_plugin->debug_generate_blocks_until(msg);
            BOOST_REQUIRE((db->head_block_time() - timestamp).to_seconds() < STEEMIT_BLOCK_INTERVAL);
        }

        const account_object &database_fixture::account_create(
                const string &name,
                const string &creator,
                const private_key_type &creator_key,
                const share_type &fee,
                const public_key_type &key,
                const public_key_type &post_key,
                const string &json_metadata
        ) {
            try {
                account_create_operation op;
                op.new_account_name = name;
                op.creator = creator;
                op.fee = fee;
                op.owner = authority(1, key, 1);
                op.active = authority(1, key, 1);
                op.posting = authority(1, post_key, 1);
                op.memo_key = key;
                op.json_metadata = json_metadata;

                trx.operations.push_back(op);
                trx.set_expiration(db->head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION);
                trx.sign(creator_key, db->get_chain_id());
                trx.validate();
                db->push_transaction(trx, 0);
                trx.operations.clear();
                trx.signatures.clear();

                const account_object &acct = db->get_account(name);

                return acct;
            }
            FC_CAPTURE_AND_RETHROW((name)(creator))
        }

        const account_object &database_fixture::account_create(
                const string &name,
                const public_key_type &key,
                const public_key_type &post_key
        ) {
            try {
                return account_create(
                        name,
                        STEEMIT_INIT_MINER_NAME,
                        init_account_priv_key,
                        30*1e3,
                        key,
                        post_key,
                        "");
            }
            FC_CAPTURE_AND_RETHROW((name));
        }

        const account_object &database_fixture::account_create(
                const string &name,
                const public_key_type &key
        ) {
            return account_create(name, key, key);
        }

        const witness_object &database_fixture::witness_create(
                const string &owner,
                const private_key_type &owner_key,
                const string &url,
                const public_key_type &signing_key,
                const share_type &fee) {
            try {
                witness_update_operation op;
                op.owner = owner;
                op.url = url;
                op.block_signing_key = signing_key;
                op.fee = asset(fee, STEEM_SYMBOL);

                trx.operations.push_back(op);
                trx.set_expiration(db->head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION);
                trx.sign(owner_key, db->get_chain_id());
                trx.validate();
                db->push_transaction(trx, 0);
                trx.operations.clear();
                trx.signatures.clear();

                return db->get_witness(owner);
            }
            FC_CAPTURE_AND_RETHROW((owner)(url))
        }

        void database_fixture::fund(
                const string &account_name,
                const share_type &amount
        ) {
            try {
                transfer(STEEMIT_INIT_MINER_NAME, account_name, amount);

            } FC_CAPTURE_AND_RETHROW((account_name)(amount))
        }

        void database_fixture::fund(
                const string &account_name,
                const asset &amount
        ) {
            try {
                db_plugin->debug_update([=](database &db) {
                    db.modify(db.get_account(account_name), [&](account_object &a) {
                        if (amount.symbol == STEEM_SYMBOL) {
                            a.balance += amount;
                        } else if (amount.symbol == SBD_SYMBOL) {
                            a.sbd_balance += amount;
                        }
                    });

                    db.modify(db.get_dynamic_global_properties(), [&](dynamic_global_property_object &gpo) {
                        if (amount.symbol == STEEM_SYMBOL) {
                            gpo.current_supply += amount;
                        } else if (amount.symbol == SBD_SYMBOL) {
                            gpo.current_sbd_supply += amount;
                        }
                    });

                    if (amount.symbol == SBD_SYMBOL) {
                        const auto &median_feed = db.get_feed_history();
                        if (median_feed.current_median_history.is_null()) {
                            db.modify(median_feed, [&](feed_history_object &f) {
                                f.current_median_history = price(asset(1, SBD_SYMBOL), asset(1, STEEM_SYMBOL));
                            });
                        }
                    }

                    db.update_virtual_supply();
                }, default_skip);
            }
            FC_CAPTURE_AND_RETHROW((account_name)(amount))
        }

        void database_fixture::convert(
                const string &account_name,
                const asset &amount) {
            try {
                const account_object &account = db->get_account(account_name);


                if (amount.symbol == STEEM_SYMBOL) {
                    db->adjust_balance(account, -amount);
                    db->adjust_balance(account, db->to_sbd(amount));
                    db->adjust_supply(-amount);
                    db->adjust_supply(db->to_sbd(amount));
                } else if (amount.symbol == SBD_SYMBOL) {
                    db->adjust_balance(account, -amount);
                    db->adjust_balance(account, db->to_steem(amount));
                    db->adjust_supply(-amount);
                    db->adjust_supply(db->to_steem(amount));
                }
            } FC_CAPTURE_AND_RETHROW((account_name)(amount))
        }

        void database_fixture::transfer(
                const string &from,
                const string &to,
                const share_type &amount) {
            try {
                transfer_operation op;
                op.from = from;
                op.to = to;
                op.amount = amount;

                trx.operations.push_back(op);
                trx.set_expiration(db->head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION);
                trx.validate();
                db->push_transaction(trx, ~0);
                trx.operations.clear();
            } FC_CAPTURE_AND_RETHROW((from)(to)(amount))
        }

        void database_fixture::vest(const string &from, const share_type &amount) {
            try {
                transfer_to_vesting_operation op;
                op.from = from;
                op.to = "";
                op.amount = asset(amount, STEEM_SYMBOL);

                trx.operations.push_back(op);
                trx.set_expiration(db->head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION);
                trx.validate();
                db->push_transaction(trx, ~0);
                trx.operations.clear();
            } FC_CAPTURE_AND_RETHROW((from)(amount))
        }

        void database_fixture::vest(const string &account, const asset &amount) {
            if (amount.symbol != STEEM_SYMBOL) {
                return;
            }

            db_plugin->debug_update([=](database &db) {
                db.modify(db.get_dynamic_global_properties(), [&](dynamic_global_property_object &gpo) {
                    gpo.current_supply += amount;
                });

                db.create_vesting(db.get_account(account), amount);

                db.update_virtual_supply();
            }, default_skip);
        }

        void database_fixture::proxy(const string &account, const string &proxy) {
            try {
                account_witness_proxy_operation op;
                op.account = account;
                op.proxy = proxy;
                trx.operations.push_back(op);
                db->push_transaction(trx, ~0);
                trx.operations.clear();
            } FC_CAPTURE_AND_RETHROW((account)(proxy))
        }

        void database_fixture::set_price_feed(const price &new_price) {
            try {
                for (int i = 1; i < STEEMIT_MAX_WITNESSES; i++) {
                    feed_publish_operation op;
                    op.publisher = STEEMIT_INIT_MINER_NAME + fc::to_string(i);
                    op.exchange_rate = new_price;
                    trx.operations.push_back(op);
                    trx.set_expiration(db->head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION);
                    db->push_transaction(trx, ~0);
                    trx.operations.clear();
                }
            } FC_CAPTURE_AND_RETHROW((new_price))

            generate_blocks(STEEMIT_BLOCKS_PER_HOUR);
#ifdef STEEMIT_BUILD_TESTNET
            BOOST_REQUIRE(!db->skip_price_feed_limit_check ||
                          db->get(feed_history_id_type()).current_median_history ==
                          new_price);
#else
            BOOST_REQUIRE(db->get(feed_history_id_type()).current_median_history == new_price);
#endif
        }

        const asset &database_fixture::get_balance(const string &account_name) const {
            return db->get_account(account_name).balance;
        }

        void database_fixture::sign(signed_transaction &trx, const fc::ecc::private_key &key) {
            trx.sign(key, db->get_chain_id());
        }

        vector<operation> database_fixture::get_last_operations(uint32_t num_ops) {
            vector<operation> ops;
            const auto &acc_hist_idx = db->get_index<account_history_index>().indices().get<by_id>();
            auto itr = acc_hist_idx.end();

            while (itr != acc_hist_idx.begin() && ops.size() < num_ops) {
                itr--;
                ops.push_back(fc::raw::unpack<golos::chain::operation>(db->get(itr->op).serialized_op));
            }

            return ops;
        }

        void database_fixture::validate_database(void) {
            try {
                db->validate_invariants();
            }
            FC_LOG_AND_RETHROW();
        }

        namespace test {

            bool _push_block(database &db, const signed_block &b, uint32_t skip_flags /* = 0 */ ) {
                return db.push_block(b, skip_flags);
            }

            void _push_transaction(database &db, const signed_transaction &tx, uint32_t skip_flags /* = 0 */ ) {
                try {
                    db.push_transaction(tx, skip_flags);
                } FC_CAPTURE_AND_RETHROW((tx))
            }

        } // golos::chain::test

} } // golos::chain
