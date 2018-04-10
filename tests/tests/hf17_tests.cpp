#ifdef STEEMIT_BUILD_TESTNET

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

#include <golos/protocol/exceptions.hpp>

#include <golos/chain/database.hpp>
#include <golos/chain/hardfork.hpp>
#include <golos/chain/steem_objects.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace golos { namespace chain {

    struct hf17_database_fixture: public database_fixture {
        hf17_database_fixture() {
            try {
                initialize();
                open_database();

                generate_block();
                db->set_hardfork(STEEMIT_HARDFORK_0_16);
                startup(false);
            } catch (const fc::exception &e) {
                edump((e.to_detail_string()));
                throw;
            }
        }

        ~hf17_database_fixture() {
            try {
                if (!std::uncaught_exception()) {
                    BOOST_CHECK(db->get_node_properties().skip_flags == database::skip_nothing);
                }
            } FC_CAPTURE_AND_RETHROW()
        }

        uint128_t quadratic_curve(uint128_t rshares) const {
            auto s = db->get_content_constant_s();
            return (rshares + s) * (rshares + s) - s * s;
        }

        uint128_t linear_curve(uint128_t rshares) {
            return rshares;
        }

    }; // struct hf17_database_fixture
} } // namespace golos::chain

using namespace golos;
using namespace golos::chain;
using namespace golos::protocol;
using std::string;

BOOST_FIXTURE_TEST_SUITE(hf17_tests, hf17_database_fixture)

    BOOST_AUTO_TEST_CASE(hf17_linear_reward_curve) {
        try {
            BOOST_TEST_MESSAGE("Testing: hf17_linear_reward_curve");

            signed_transaction tx;
            comment_operation comment_op;
            vote_operation vote_op;

            ACTORS((alice)(bob)(sam))
            generate_block();

            const auto &alice_account = db->get_account("alice");
            const auto &bob_account = db->get_account("bob");
            const auto &sam_account = db->get_account("sam");

            vest("alice", ASSET("30.000 GOLOS"));
            vest("bob", ASSET("20.000 GOLOS"));
            vest("sam", ASSET("10.000 GOLOS"));
            generate_block();
            validate_database();

            const auto &gpo = db->get_dynamic_global_properties();
            BOOST_REQUIRE(gpo.total_reward_shares2 == 0);

            comment_op.author = "alice";
            comment_op.permlink = "foo";
            comment_op.parent_permlink = "test";
            comment_op.title = "bar";
            comment_op.body = "foo bar";

            tx.operations = { comment_op };
            tx.set_expiration(db->head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION);
            tx.sign(alice_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);
            generate_block();

            const auto &alice_comment = db->get_comment(comment_op.author, comment_op.permlink);

            comment_op.author = "bob";

            tx.operations = { comment_op };
            tx.signatures.clear();
            tx.sign(bob_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);
            generate_block();

            const auto &bob_comment = db->get_comment(comment_op.author, comment_op.permlink);

            vote_op.voter = "bob";
            vote_op.author = "alice";
            vote_op.permlink = "foo";
            vote_op.weight = STEEMIT_100_PERCENT;

            tx.operations = { vote_op };
            tx.signatures.clear();
            tx.sign(bob_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);

            vote_op.voter = "sam";
            vote_op.weight = STEEMIT_1_PERCENT * 50;

            tx.operations = { vote_op };
            tx.signatures.clear();
            tx.sign(sam_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);
            generate_blocks(db->head_block_time() + fc::seconds(STEEMIT_MIN_VOTE_INTERVAL_SEC), true);

            vote_op.voter = "alice";
            vote_op.author = "bob";
            vote_op.permlink = "foo";
            vote_op.weight = STEEMIT_1_PERCENT * 75;

            tx.operations = { vote_op };
            tx.signatures.clear();
            tx.sign(alice_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);

            vote_op.voter = "sam";
            vote_op.weight = STEEMIT_1_PERCENT * 50;

            tx.operations = { vote_op };
            tx.signatures.clear();
            tx.sign(sam_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);
            generate_block();

            validate_database();

            const auto vote_denom = gpo.vote_regeneration_per_day * STEEMIT_VOTE_REGENERATION_SECONDS / (60 * 60 * 24);

            const auto bob_power = (STEEMIT_100_PERCENT + vote_denom - 1) / vote_denom;
            const auto bob_vshares = bob_account.vesting_shares.amount.value * bob_power / STEEMIT_100_PERCENT;

            const auto sam_power = (STEEMIT_1_PERCENT * 50 + vote_denom - 1) / vote_denom;
            const auto sam_vshares = sam_account.vesting_shares.amount.value * sam_power / STEEMIT_100_PERCENT;

            const auto alice_power = (STEEMIT_1_PERCENT * 75 + vote_denom - 1) / vote_denom;
            const auto alice_vshares = alice_account.vesting_shares.amount.value * alice_power / STEEMIT_100_PERCENT;

            BOOST_REQUIRE(gpo.total_reward_shares2 ==
                              quadratic_curve(sam_vshares + bob_vshares) +
                              quadratic_curve(sam_vshares + alice_vshares));
            BOOST_REQUIRE(gpo.total_reward_shares2 ==
                              quadratic_curve(alice_comment.net_rshares.value) +
                              quadratic_curve(bob_comment.net_rshares.value));

            db->set_hardfork(STEEMIT_HARDFORK_0_17);
            generate_block();
            validate_database();

            BOOST_REQUIRE(gpo.total_reward_shares2 ==
                              linear_curve(sam_vshares + bob_vshares) +
                              linear_curve(sam_vshares + alice_vshares));
            BOOST_REQUIRE(gpo.total_reward_shares2 ==
                              linear_curve(alice_comment.net_rshares.value) +
                              linear_curve(bob_comment.net_rshares.value));

            comment_op.author = "sam";

            tx.operations = { comment_op };
            tx.signatures.clear();
            tx.sign(sam_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);
            generate_block();

            const auto &sam_comment = db->get_comment(comment_op.author, comment_op.permlink);

            vote_op.voter = "alice";
            vote_op.author = "sam";
            vote_op.permlink = "foo";
            vote_op.weight = STEEMIT_1_PERCENT * 75;

            tx.operations = { vote_op };
            tx.signatures.clear();
            tx.sign(alice_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);
            generate_block();

            BOOST_REQUIRE(gpo.total_reward_shares2 ==
                              linear_curve(sam_vshares + bob_vshares) +
                              linear_curve(sam_vshares + alice_vshares) +
                              linear_curve(alice_vshares));
            BOOST_REQUIRE(gpo.total_reward_shares2 ==
                              linear_curve(alice_comment.net_rshares.value) +
                              linear_curve(bob_comment.net_rshares.value) +
                              linear_curve(sam_comment.net_rshares.value));

        }
        FC_LOG_AND_RETHROW()
    }

    BOOST_AUTO_TEST_CASE(hf17_update_cashout_window) {
        try {
            BOOST_TEST_MESSAGE("Testing: hf17_update_cashout_window");

            signed_transaction tx;
            comment_operation comment_op;

            ACTORS((alice)(bob))
            generate_block();

            vest("alice", ASSET("30.000 GOLOS"));
            vest("bob", ASSET("20.000 GOLOS"));
            generate_block();
            validate_database();

            comment_op.author = "alice";
            comment_op.permlink = "foo";
            comment_op.parent_permlink = "test";
            comment_op.title = "bar";
            comment_op.body = "foo bar";

            tx.operations = { comment_op };
            tx.set_expiration(db->head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION);
            tx.sign(alice_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);
            generate_block();

            const auto &alice_comment = db->get_comment(comment_op.author, comment_op.permlink);

            comment_op.author = "bob";

            tx.operations = { comment_op };
            tx.signatures.clear();
            tx.sign(bob_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);
            generate_block();

            const auto &bob_comment = db->get_comment(comment_op.author, comment_op.permlink);

            validate_database();

            BOOST_REQUIRE(alice_comment.cashout_time == alice_comment.created +STEEMIT_CASHOUT_WINDOW_SECONDS_PRE_HF17);
            BOOST_REQUIRE(bob_comment.cashout_time == bob_comment.created + STEEMIT_CASHOUT_WINDOW_SECONDS_PRE_HF17);

            db->set_hardfork(STEEMIT_HARDFORK_0_17);
            generate_block();
            validate_database();

            BOOST_REQUIRE(alice_comment.cashout_time == alice_comment.created +STEEMIT_CASHOUT_WINDOW_SECONDS);
            BOOST_REQUIRE(bob_comment.cashout_time == bob_comment.created + STEEMIT_CASHOUT_WINDOW_SECONDS);

            generate_blocks(bob_comment.cashout_time);

            BOOST_REQUIRE(alice_comment.cashout_time == fc::time_point_sec::maximum());
            BOOST_REQUIRE(bob_comment.cashout_time == fc::time_point_sec::maximum());
        }
        FC_LOG_AND_RETHROW()
    }

    BOOST_AUTO_TEST_CASE(hf17_nested_comment) {
        try {
            BOOST_TEST_MESSAGE("Testing: hf17_nested_comment");

            signed_transaction tx;
            comment_operation comment_op;

            ACTORS((alice)(bob))
            generate_block();

            vest("alice", ASSET("30.000 GOLOS"));
            vest("bob", ASSET("20.000 GOLOS"));
            generate_block();
            validate_database();

            comment_op.author = "alice";
            comment_op.permlink = "foo";
            comment_op.parent_permlink = "test";
            comment_op.title = "bar";
            comment_op.body = "foo bar";
            tx.operations = { comment_op };
            tx.set_expiration(db->head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION);
            tx.sign(alice_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);
            generate_block();

            comment_op.parent_permlink = comment_op.permlink;
            comment_op.parent_author = comment_op.author;
            comment_op.author = "bob";
            comment_op.permlink = "bob-comment-1";
            tx.operations = { comment_op };
            tx.signatures.clear();
            tx.sign(bob_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);
            generate_blocks(db->head_block_time() + STEEMIT_MIN_REPLY_INTERVAL);

            comment_op.parent_permlink = comment_op.permlink;
            comment_op.parent_author = comment_op.author;
            comment_op.author = "alice";
            comment_op.permlink = "alice-comment-2";
            tx.operations = { comment_op };
            tx.signatures.clear();
            tx.sign(alice_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);
            generate_blocks(db->head_block_time() + STEEMIT_MIN_REPLY_INTERVAL);

            comment_op.parent_permlink = comment_op.permlink;
            comment_op.parent_author = comment_op.author;
            comment_op.author = "bob";
            comment_op.permlink = "bob-comment-3";
            tx.operations = { comment_op };
            tx.signatures.clear();
            tx.sign(bob_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);
            generate_blocks(db->head_block_time() + STEEMIT_MIN_REPLY_INTERVAL);

            comment_op.parent_permlink = comment_op.permlink;
            comment_op.parent_author = comment_op.author;
            comment_op.author = "alice";
            comment_op.permlink = "alice-comment-4";
            tx.operations = { comment_op };
            tx.signatures.clear();
            tx.sign(alice_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);
            generate_blocks(db->head_block_time() + STEEMIT_MIN_REPLY_INTERVAL);

            comment_op.parent_permlink = comment_op.permlink;
            comment_op.parent_author = comment_op.author;
            comment_op.author = "bob";
            comment_op.permlink = "bob-comment-5";
            tx.operations = { comment_op };
            tx.signatures.clear();
            tx.sign(bob_private_key, db->get_chain_id());
            db->push_transaction(tx, 0);
            generate_blocks(db->head_block_time() + STEEMIT_MIN_REPLY_INTERVAL);

            comment_op.parent_permlink = comment_op.permlink;
            comment_op.parent_author = comment_op.author;
            comment_op.author = "alice";
            comment_op.permlink = "alice-comment-6";
            tx.operations = { comment_op };
            tx.signatures.clear();
            tx.sign(alice_private_key, db->get_chain_id());
            STEEMIT_REQUIRE_THROW(db->push_transaction(tx, 0), fc::exception);

            generate_block();
            db->set_hardfork(STEEMIT_HARDFORK_0_17);
            generate_block();
            validate_database();

            db->push_transaction(tx, 0);
        }
        FC_LOG_AND_RETHROW()
    }
BOOST_AUTO_TEST_SUITE_END()

#endif