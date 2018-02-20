#ifdef STEEMIT_BUILD_TESTNET

#include <boost/test/unit_test.hpp>

#include <golos/chain/account_object.hpp>
#include <golos/chain/comment_object.hpp>
#include <golos/protocol/steem_operations.hpp>

#include <golos/plugins/market_history/market_history_plugin.hpp>

#include "../common/database_fixture.hpp"

using namespace golos::chain;
using namespace golos::protocol;

BOOST_FIXTURE_TEST_SUITE(market_history, database_fixture)

    BOOST_AUTO_TEST_CASE(mh_test) {
        using namespace golos::plugins::market_history;
        using by_id = golos::plugins::market_history::by_id;

        try {
            initialize();

            auto &mh_plugin = appbase::app().register_plugin<market_history_plugin>();
            boost::program_options::variables_map options;
            mh_plugin.plugin_initialize(options);

            open_database();

            startup();
            mh_plugin.plugin_startup();

            ACTORS((alice)(bob)(sam));
            generate_block();

            fund("alice", ASSET("1000.000 GOLOS"));
            fund("alice", ASSET("1000.000 GBG"));
            fund("bob", ASSET("1000.000 GOLOS"));
            fund("sam", ASSET("1000.000 GOLOS"));

            set_price_feed(price(ASSET("0.500 GBG"), ASSET("1.000 GOLOS")));

            signed_transaction tx;

            const auto &bucket_idx = db->get_index<bucket_index>().indices().get<by_bucket>();
            const auto &order_hist_idx = db->get_index<order_history_index>().indices().get<by_id>();

            BOOST_REQUIRE(bucket_idx.begin() == bucket_idx.end());
            BOOST_REQUIRE(order_hist_idx.begin() == order_hist_idx.end());
            validate_database();

            auto fill_order_a_time = db->head_block_time();
            auto time_a = fc::time_point_sec(
                    (fill_order_a_time.sec_since_epoch() / 15) * 15);

            limit_order_create_operation op;
            op.owner = "alice";
            op.amount_to_sell = ASSET("1.000 GBG");
            op.min_to_receive = ASSET("2.000 GOLOS");
            tx.operations.push_back(op);
            tx.set_expiration(
                    db->head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION);
            tx.sign(alice_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);

            tx.operations.clear();
            tx.signatures.clear();

            op.owner = "bob";
            op.amount_to_sell = ASSET("1.500 GOLOS");
            op.min_to_receive = ASSET("0.750 GBG");
            tx.operations.push_back(op);
            tx.sign(bob_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);

            generate_blocks(db->head_block_time() + (60 * 90));

            auto fill_order_b_time = db->head_block_time();

            tx.operations.clear();
            tx.signatures.clear();

            op.owner = "sam";
            op.amount_to_sell = ASSET("1.000 GOLOS");
            op.min_to_receive = ASSET("0.500 GBG");
            tx.operations.push_back(op);
            tx.set_expiration(
                    db->head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION);
            tx.sign(sam_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);

            generate_blocks(db->head_block_time() + 60);

            auto fill_order_c_time = db->head_block_time();

            tx.operations.clear();
            tx.signatures.clear();

            op.owner = "alice";
            op.amount_to_sell = ASSET("0.500 GBG");
            op.min_to_receive = ASSET("0.900 GOLOS");
            tx.operations.push_back(op);
            tx.set_expiration(
                    db->head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION);
            tx.sign(alice_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);

            tx.operations.clear();
            tx.signatures.clear();

            op.owner = "bob";
            op.amount_to_sell = ASSET("0.450 GOLOS");
            op.min_to_receive = ASSET("0.250 GBG");
            tx.operations.push_back(op);
            tx.set_expiration(
                    db->head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION);
            tx.sign(bob_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);
            validate_database();

            auto bucket = bucket_idx.begin();

            BOOST_REQUIRE(bucket->seconds == 15);
            BOOST_REQUIRE(bucket->open == time_a);
            BOOST_REQUIRE(bucket->high_steem == ASSET("1.500 GOLOS ").amount);
            BOOST_REQUIRE(bucket->high_sbd == ASSET("0.750 GBG").amount);
            BOOST_REQUIRE(bucket->low_steem == ASSET("1.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->low_sbd == ASSET("0.750 GBG").amount);
            BOOST_REQUIRE(bucket->open_steem == ASSET("1.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->open_sbd == ASSET("0.750 GBG").amount);
            BOOST_REQUIRE(bucket->close_steem == ASSET("1.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->close_sbd == ASSET("0.750 GBG").amount);
            BOOST_REQUIRE(bucket->steem_volume == ASSET("1.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->sbd_volume == ASSET("0.750 GBG").amount);
            bucket++;

            BOOST_REQUIRE(bucket->seconds == 15);
            BOOST_REQUIRE(bucket->open == time_a + (60 * 90));
            BOOST_REQUIRE(bucket->high_steem == ASSET("0.500 GOLOS ").amount);
            BOOST_REQUIRE(bucket->high_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->low_steem == ASSET("0.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->low_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->open_steem == ASSET("0.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->open_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->close_steem == ASSET("0.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->close_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->steem_volume == ASSET("0.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->sbd_volume == ASSET("0.250 GBG").amount);
            bucket++;

            BOOST_REQUIRE(bucket->seconds == 15);
            BOOST_REQUIRE(bucket->open == time_a + (60 * 90) + 60);
            BOOST_REQUIRE(bucket->high_steem == ASSET("0.450 GOLOS ").amount);
            BOOST_REQUIRE(bucket->high_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->low_steem == ASSET("0.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->low_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->open_steem == ASSET("0.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->open_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->close_steem == ASSET("0.450 GOLOS").amount);
            BOOST_REQUIRE(bucket->close_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->steem_volume == ASSET("0.950 GOLOS").amount);
            BOOST_REQUIRE(bucket->sbd_volume == ASSET("0.500 GBG").amount);
            bucket++;

            BOOST_REQUIRE(bucket->seconds == 60);
            BOOST_REQUIRE(bucket->open == time_a);
            BOOST_REQUIRE(bucket->high_steem == ASSET("1.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->high_sbd == ASSET("0.750 GBG").amount);
            BOOST_REQUIRE(bucket->low_steem == ASSET("1.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->low_sbd == ASSET("0.750 GBG").amount);
            BOOST_REQUIRE(bucket->open_steem == ASSET("1.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->open_sbd == ASSET("0.750 GBG").amount);
            BOOST_REQUIRE(bucket->close_steem == ASSET("1.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->close_sbd == ASSET("0.750 GBG").amount);
            BOOST_REQUIRE(bucket->steem_volume == ASSET("1.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->sbd_volume == ASSET("0.750 GBG").amount);
            bucket++;

            BOOST_REQUIRE(bucket->seconds == 60);
            BOOST_REQUIRE(bucket->open == time_a + (60 * 90));
            BOOST_REQUIRE(bucket->high_steem == ASSET("0.500 GOLOS ").amount);
            BOOST_REQUIRE(bucket->high_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->low_steem == ASSET("0.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->low_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->open_steem == ASSET("0.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->open_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->close_steem == ASSET("0.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->close_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->steem_volume == ASSET("0.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->sbd_volume == ASSET("0.250 GBG").amount);
            bucket++;

            BOOST_REQUIRE(bucket->seconds == 60);
            BOOST_REQUIRE(bucket->open == time_a + (60 * 90) + 60);
            BOOST_REQUIRE(bucket->high_steem == ASSET("0.450 GOLOS ").amount);
            BOOST_REQUIRE(bucket->high_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->low_steem == ASSET("0.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->low_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->open_steem == ASSET("0.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->open_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->close_steem == ASSET("0.450 GOLOS").amount);
            BOOST_REQUIRE(bucket->close_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->steem_volume == ASSET("0.950 GOLOS").amount);
            BOOST_REQUIRE(bucket->sbd_volume == ASSET("0.500 GBG").amount);
            bucket++;

            BOOST_REQUIRE(bucket->seconds == 300);
            BOOST_REQUIRE(bucket->open == time_a);
            BOOST_REQUIRE(bucket->high_steem == ASSET("1.500 GOLOS ").amount);
            BOOST_REQUIRE(bucket->high_sbd == ASSET("0.750 GBG").amount);
            BOOST_REQUIRE(bucket->low_steem == ASSET("1.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->low_sbd == ASSET("0.750 GBG").amount);
            BOOST_REQUIRE(bucket->open_steem == ASSET("1.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->open_sbd == ASSET("0.750 GBG").amount);
            BOOST_REQUIRE(bucket->close_steem == ASSET("1.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->close_sbd == ASSET("0.750 GBG").amount);
            BOOST_REQUIRE(bucket->steem_volume == ASSET("1.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->sbd_volume == ASSET("0.750 GBG").amount);
            bucket++;

            BOOST_REQUIRE(bucket->seconds == 300);
            BOOST_REQUIRE(bucket->open == time_a + (60 * 90));
            BOOST_REQUIRE(bucket->high_steem == ASSET("0.450 GOLOS ").amount);
            BOOST_REQUIRE(bucket->high_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->low_steem == ASSET("0.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->low_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->open_steem == ASSET("0.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->open_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->close_steem == ASSET("0.450 GOLOS").amount);
            BOOST_REQUIRE(bucket->close_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->steem_volume == ASSET("1.450 GOLOS").amount);
            BOOST_REQUIRE(bucket->sbd_volume == ASSET("0.750 GBG").amount);
            bucket++;

            BOOST_REQUIRE(bucket->seconds == 3600);
            BOOST_REQUIRE(bucket->open == time_a);
            BOOST_REQUIRE(bucket->high_steem == ASSET("1.500 GOLOS ").amount);
            BOOST_REQUIRE(bucket->high_sbd == ASSET("0.750 GBG").amount);
            BOOST_REQUIRE(bucket->low_steem == ASSET("1.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->low_sbd == ASSET("0.750 GBG").amount);
            BOOST_REQUIRE(bucket->open_steem == ASSET("1.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->open_sbd == ASSET("0.750 GBG").amount);
            BOOST_REQUIRE(bucket->close_steem == ASSET("1.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->close_sbd == ASSET("0.750 GBG").amount);
            BOOST_REQUIRE(bucket->steem_volume == ASSET("1.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->sbd_volume == ASSET("0.750 GBG").amount);
            bucket++;

            BOOST_REQUIRE(bucket->seconds == 3600);
            BOOST_REQUIRE(bucket->open == time_a + (60 * 60));
            BOOST_REQUIRE(bucket->high_steem == ASSET("0.450 GOLOS ").amount);
            BOOST_REQUIRE(bucket->high_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->low_steem == ASSET("0.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->low_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->open_steem == ASSET("0.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->open_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->close_steem == ASSET("0.450 GOLOS").amount);
            BOOST_REQUIRE(bucket->close_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->steem_volume == ASSET("1.450 GOLOS").amount);
            BOOST_REQUIRE(bucket->sbd_volume == ASSET("0.750 GBG").amount);
            bucket++;

            BOOST_REQUIRE(bucket->seconds == 86400);
            // FIXME: broken check
            // BOOST_REQUIRE(bucket->open == STEEMIT_GENESIS_TIME);
            BOOST_REQUIRE(bucket->high_steem == ASSET("0.450 GOLOS ").amount);
            BOOST_REQUIRE(bucket->high_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->low_steem == ASSET("1.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->low_sbd == ASSET("0.750 GBG").amount);
            BOOST_REQUIRE(bucket->open_steem == ASSET("1.500 GOLOS").amount);
            BOOST_REQUIRE(bucket->open_sbd == ASSET("0.750 GBG").amount);
            BOOST_REQUIRE(bucket->close_steem == ASSET("0.450 GOLOS").amount);
            BOOST_REQUIRE(bucket->close_sbd == ASSET("0.250 GBG").amount);
            BOOST_REQUIRE(bucket->steem_volume == ASSET("2.950 GOLOS").amount);
            BOOST_REQUIRE(bucket->sbd_volume == ASSET("1.500 GBG").amount);
            bucket++;

            BOOST_REQUIRE(bucket == bucket_idx.end());

            auto order = order_hist_idx.begin();

            BOOST_REQUIRE(order->time == fill_order_a_time);
            BOOST_REQUIRE(order->op.current_owner == "bob");
            BOOST_REQUIRE(order->op.current_orderid == 0);
            BOOST_REQUIRE(order->op.current_pays == ASSET("1.500 GOLOS"));
            BOOST_REQUIRE(order->op.open_owner == "alice");
            BOOST_REQUIRE(order->op.open_orderid == 0);
            BOOST_REQUIRE(order->op.open_pays == ASSET("0.750 GBG"));
            order++;

            BOOST_REQUIRE(order->time == fill_order_b_time);
            BOOST_REQUIRE(order->op.current_owner == "sam");
            BOOST_REQUIRE(order->op.current_orderid == 0);
            BOOST_REQUIRE(order->op.current_pays == ASSET("0.500 GOLOS"));
            BOOST_REQUIRE(order->op.open_owner == "alice");
            BOOST_REQUIRE(order->op.open_orderid == 0);
            BOOST_REQUIRE(order->op.open_pays == ASSET("0.250 GBG"));
            order++;

            BOOST_REQUIRE(order->time == fill_order_c_time);
            BOOST_REQUIRE(order->op.current_owner == "alice");
            BOOST_REQUIRE(order->op.current_orderid == 0);
            BOOST_REQUIRE(order->op.current_pays == ASSET("0.250 GBG"));
            BOOST_REQUIRE(order->op.open_owner == "sam");
            BOOST_REQUIRE(order->op.open_orderid == 0);
            BOOST_REQUIRE(order->op.open_pays == ASSET("0.500 GOLOS"));
            order++;

            BOOST_REQUIRE(order->time == fill_order_c_time);
            BOOST_REQUIRE(order->op.current_owner == "bob");
            BOOST_REQUIRE(order->op.current_orderid == 0);
            BOOST_REQUIRE(order->op.current_pays == ASSET("0.450 GOLOS"));
            BOOST_REQUIRE(order->op.open_owner == "alice");
            BOOST_REQUIRE(order->op.open_orderid == 0);
            BOOST_REQUIRE(order->op.open_pays == ASSET("0.250 GBG"));
            order++;

            BOOST_REQUIRE(order == order_hist_idx.end());
        }
        FC_LOG_AND_RETHROW()
    }

BOOST_AUTO_TEST_SUITE_END()
#endif