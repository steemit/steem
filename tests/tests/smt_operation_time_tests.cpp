
#if defined IS_TEST_NET && defined STEEM_ENABLE_SMT

#include <boost/test/unit_test.hpp>

#include <steem/chain/steem_fwd.hpp>

#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/hardfork.hpp>

#include <steem/chain/block_summary_object.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/history_object.hpp>
#include <steem/chain/steem_objects.hpp>
#include <steem/chain/smt_objects.hpp>

#include <steem/chain/util/reward.hpp>

#include <steem/plugins/debug_node/debug_node_plugin.hpp>

#include <fc/crypto/digest.hpp>

#include "../db_fixture/database_fixture.hpp"

#include <cmath>

using namespace steem;
using namespace steem::chain;
using namespace steem::protocol;

BOOST_FIXTURE_TEST_SUITE( smt_operation_time_tests, smt_database_fixture )

BOOST_AUTO_TEST_CASE( smt_liquidity_rewards )
{
   using std::abs;

   try
   {
      db->liquidity_rewards_enabled = false;

      ACTORS( (alice)(bob)(sam)(dave)(smtcreator) )

      //Create SMT and give some SMT to creators.
      signed_transaction tx;
      asset_symbol_type any_smt_symbol = create_smt( "smtcreator", smtcreator_private_key, 3);

      generate_block();
      vest( STEEM_INIT_MINER_NAME, "alice", ASSET( "10.000 TESTS" ) );
      vest( STEEM_INIT_MINER_NAME, "bob", ASSET( "10.000 TESTS" ) );
      vest( STEEM_INIT_MINER_NAME, "sam", ASSET( "10.000 TESTS" ) );
      vest( STEEM_INIT_MINER_NAME, "dave", ASSET( "10.000 TESTS" ) );

      tx.operations.clear();
      tx.signatures.clear();

      BOOST_TEST_MESSAGE( "Rewarding Bob with TESTS" );

      auto exchange_rate = price( ASSET( "1.250 TESTS" ), asset( 1000, any_smt_symbol ) );

      const account_object& alice_account = db->get_account( "alice" );
      FUND( "alice", asset( 25522, any_smt_symbol ) );
      asset alice_smt = db->get_balance( alice_account, any_smt_symbol );

      FUND( "alice", alice_smt.amount );
      FUND( "bob", alice_smt.amount );
      FUND( "sam", alice_smt.amount );
      FUND( "dave", alice_smt.amount );

      int64_t alice_smt_volume = 0;
      int64_t alice_steem_volume = 0;
      time_point_sec alice_reward_last_update = fc::time_point_sec::min();
      int64_t bob_smt_volume = 0;
      int64_t bob_steem_volume = 0;
      time_point_sec bob_reward_last_update = fc::time_point_sec::min();
      int64_t sam_smt_volume = 0;
      int64_t sam_steem_volume = 0;
      time_point_sec sam_reward_last_update = fc::time_point_sec::min();
      int64_t dave_smt_volume = 0;
      int64_t dave_steem_volume = 0;
      time_point_sec dave_reward_last_update = fc::time_point_sec::min();

      BOOST_TEST_MESSAGE( "Creating Limit Order for STEEM that will stay on the books for 30 minutes exactly." );

      limit_order_create_operation op;
      op.owner = "alice";
      op.amount_to_sell = asset( alice_smt.amount.value / 20, any_smt_symbol ) ;
      op.min_to_receive = op.amount_to_sell * exchange_rate;
      op.orderid = 1;
      op.expiration = db->head_block_time() + fc::seconds( STEEM_MAX_LIMIT_ORDER_EXPIRATION );

      tx.signatures.clear();
      tx.operations.clear();
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Waiting 10 minutes" );

      generate_blocks( db->head_block_time() + STEEM_MIN_LIQUIDITY_REWARD_PERIOD_SEC_HF10, true );

      BOOST_TEST_MESSAGE( "Creating Limit Order for SMT that will be filled immediately." );

      op.owner = "bob";
      op.min_to_receive = op.amount_to_sell;
      op.amount_to_sell = op.min_to_receive * exchange_rate;
      op.fill_or_kill = false;
      op.orderid = 2;

      tx.signatures.clear();
      tx.operations.clear();
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      alice_steem_volume += ( asset( alice_smt.amount / 20, any_smt_symbol ) * exchange_rate ).amount.value;
      alice_reward_last_update = db->head_block_time();
      bob_steem_volume -= ( asset( alice_smt.amount / 20, any_smt_symbol ) * exchange_rate ).amount.value;
      bob_reward_last_update = db->head_block_time();

      auto ops = get_last_operations( 1 );
      const auto& liquidity_idx = db->get_index< liquidity_reward_balance_index >().indices().get< by_owner >();
      const auto& limit_order_idx = db->get_index< limit_order_index >().indices().get< by_account >();

      auto reward = liquidity_idx.find( db->get_account( "alice" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "alice" ).id );
      BOOST_REQUIRE( reward->sbd_volume == alice_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == alice_steem_volume );
      BOOST_CHECK( reward->last_update == alice_reward_last_update );*/

      reward = liquidity_idx.find( db->get_account( "bob" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "bob" ).id );
      BOOST_REQUIRE( reward->sbd_volume == bob_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == bob_steem_volume );
      BOOST_CHECK( reward->last_update == bob_reward_last_update );*/

      auto fill_order_op = ops[0].get< fill_order_operation >();

      BOOST_REQUIRE( fill_order_op.open_owner == "alice" );
      BOOST_REQUIRE( fill_order_op.open_orderid == 1 );
      BOOST_REQUIRE( fill_order_op.open_pays.amount.value == asset( alice_smt.amount.value / 20, any_smt_symbol ).amount.value );
      BOOST_REQUIRE( fill_order_op.current_owner == "bob" );
      BOOST_REQUIRE( fill_order_op.current_orderid == 2 );
      BOOST_REQUIRE( fill_order_op.current_pays.amount.value == ( asset( alice_smt.amount.value / 20, any_smt_symbol ) * exchange_rate ).amount.value );

      BOOST_CHECK( limit_order_idx.find( boost::make_tuple( "alice", 1 ) ) == limit_order_idx.end() );
      BOOST_CHECK( limit_order_idx.find( boost::make_tuple( "bob", 2 ) ) == limit_order_idx.end() );

      BOOST_TEST_MESSAGE( "Creating Limit Order for SMT that will stay on the books for 60 minutes." );

      op.owner = "sam";
      op.amount_to_sell = asset( ( alice_smt.amount.value / 20 ), STEEM_SYMBOL );
      op.min_to_receive = asset( ( alice_smt.amount.value / 20 ), any_smt_symbol );
      op.orderid = 3;

      tx.signatures.clear();
      tx.operations.clear();
      tx.operations.push_back( op );
      sign( tx, sam_private_key );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Waiting 10 minutes" );

      generate_blocks( db->head_block_time() + STEEM_MIN_LIQUIDITY_REWARD_PERIOD_SEC_HF10, true );

      BOOST_TEST_MESSAGE( "Creating Limit Order for SMT that will stay on the books for 30 minutes." );

      op.owner = "bob";
      op.orderid = 4;
      op.amount_to_sell = asset( ( alice_smt.amount.value / 10 ) * 3 - alice_smt.amount.value / 20, STEEM_SYMBOL );
      op.min_to_receive = asset( ( alice_smt.amount.value / 10 ) * 3 - alice_smt.amount.value / 20, any_smt_symbol );

      tx.signatures.clear();
      tx.operations.clear();
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Waiting 30 minutes" );

      generate_blocks( db->head_block_time() + STEEM_MIN_LIQUIDITY_REWARD_PERIOD_SEC_HF10, true );

      BOOST_TEST_MESSAGE( "Filling both limit orders." );

      op.owner = "alice";
      op.orderid = 5;
      op.amount_to_sell = asset( ( alice_smt.amount.value / 10 ) * 3, any_smt_symbol );
      op.min_to_receive = asset( ( alice_smt.amount.value / 10 ) * 3, STEEM_SYMBOL );

      tx.signatures.clear();
      tx.operations.clear();
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      alice_smt_volume -= ( alice_smt.amount.value / 10 ) * 3;
      alice_reward_last_update = db->head_block_time();
      sam_smt_volume += alice_smt.amount.value / 20;
      sam_reward_last_update = db->head_block_time();
      bob_smt_volume += ( alice_smt.amount.value / 10 ) * 3 - ( alice_smt.amount.value / 20 );
      bob_reward_last_update = db->head_block_time();
      ops = get_last_operations( 4 );

      fill_order_op = ops[1].get< fill_order_operation >();
      BOOST_REQUIRE( fill_order_op.open_owner == "bob" );
      BOOST_REQUIRE( fill_order_op.open_orderid == 4 );
      BOOST_REQUIRE( fill_order_op.open_pays.amount.value == asset( ( alice_smt.amount.value / 10 ) * 3 - alice_smt.amount.value / 20, STEEM_SYMBOL ).amount.value );
      BOOST_REQUIRE( fill_order_op.current_owner == "alice" );
      BOOST_REQUIRE( fill_order_op.current_orderid == 5 );
      BOOST_REQUIRE( fill_order_op.current_pays.amount.value == asset( ( alice_smt.amount.value / 10 ) * 3 - alice_smt.amount.value / 20, any_smt_symbol ).amount.value );

      fill_order_op = ops[3].get< fill_order_operation >();
      BOOST_REQUIRE( fill_order_op.open_owner == "sam" );
      BOOST_REQUIRE( fill_order_op.open_orderid == 3 );
      BOOST_REQUIRE( fill_order_op.open_pays.amount.value == asset( alice_smt.amount.value / 20, STEEM_SYMBOL ).amount.value );
      BOOST_REQUIRE( fill_order_op.current_owner == "alice" );
      BOOST_REQUIRE( fill_order_op.current_orderid == 5 );
      BOOST_REQUIRE( fill_order_op.current_pays.amount.value == asset( alice_smt.amount.value / 20, any_smt_symbol ).amount.value );

      reward = liquidity_idx.find( db->get_account( "alice" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "alice" ).id );
      BOOST_REQUIRE( reward->sbd_volume == alice_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == alice_steem_volume );
      BOOST_CHECK( reward->last_update == alice_reward_last_update );*/

      reward = liquidity_idx.find( db->get_account( "bob" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "bob" ).id );
      BOOST_REQUIRE( reward->sbd_volume == bob_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == bob_steem_volume );
      BOOST_CHECK( reward->last_update == bob_reward_last_update );*/

      reward = liquidity_idx.find( db->get_account( "sam" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "sam" ).id );
      BOOST_REQUIRE( reward->sbd_volume == sam_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == sam_steem_volume );
      BOOST_CHECK( reward->last_update == sam_reward_last_update );*/

      BOOST_TEST_MESSAGE( "Testing a partial fill before minimum time and full fill after minimum time" );

      op.orderid = 6;
      op.amount_to_sell = asset( alice_smt.amount.value / 20 * 2, any_smt_symbol );
      op.min_to_receive = asset( alice_smt.amount.value / 20 * 2, STEEM_SYMBOL );

      tx.signatures.clear();
      tx.operations.clear();
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      generate_blocks( db->head_block_time() + fc::seconds( STEEM_MIN_LIQUIDITY_REWARD_PERIOD_SEC_HF10.to_seconds() / 2 ), true );

      op.owner = "bob";
      op.orderid = 7;
      op.amount_to_sell = asset( alice_smt.amount.value / 20, STEEM_SYMBOL );
      op.min_to_receive = asset( alice_smt.amount.value / 20, any_smt_symbol );

      tx.signatures.clear();
      tx.operations.clear();
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      generate_blocks( db->head_block_time() + fc::seconds( STEEM_MIN_LIQUIDITY_REWARD_PERIOD_SEC_HF10.to_seconds() / 2 ), true );

      ops = get_last_operations( 3 );
      fill_order_op = ops[2].get< fill_order_operation >();

      BOOST_REQUIRE( fill_order_op.open_owner == "alice" );
      BOOST_REQUIRE( fill_order_op.open_orderid == 6 );
      BOOST_REQUIRE( fill_order_op.open_pays.amount.value == asset( alice_smt.amount.value / 20, any_smt_symbol ).amount.value );
      BOOST_REQUIRE( fill_order_op.current_owner == "bob" );
      BOOST_REQUIRE( fill_order_op.current_orderid == 7 );
      BOOST_REQUIRE( fill_order_op.current_pays.amount.value == asset( alice_smt.amount.value / 20, STEEM_SYMBOL ).amount.value );

      reward = liquidity_idx.find( db->get_account( "alice" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "alice" ).id );
      BOOST_REQUIRE( reward->sbd_volume == alice_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == alice_steem_volume );
      BOOST_CHECK( reward->last_update == alice_reward_last_update );*/

      reward = liquidity_idx.find( db->get_account( "bob" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "bob" ).id );
      BOOST_REQUIRE( reward->sbd_volume == bob_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == bob_steem_volume );
      BOOST_CHECK( reward->last_update == bob_reward_last_update );*/

      reward = liquidity_idx.find( db->get_account( "sam" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "sam" ).id );
      BOOST_REQUIRE( reward->sbd_volume == sam_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == sam_steem_volume );
      BOOST_CHECK( reward->last_update == sam_reward_last_update );*/

      generate_blocks( db->head_block_time() + STEEM_MIN_LIQUIDITY_REWARD_PERIOD_SEC_HF10, true );

      op.owner = "sam";
      op.orderid = 8;

      tx.signatures.clear();
      tx.operations.clear();
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      sign( tx, sam_private_key );
      db->push_transaction( tx, 0 );

      alice_steem_volume += alice_smt.amount.value / 20;
      alice_reward_last_update = db->head_block_time();
      sam_steem_volume -= alice_smt.amount.value / 20;
      sam_reward_last_update = db->head_block_time();

      ops = get_last_operations( 2 );
      fill_order_op = ops[1].get< fill_order_operation >();

      BOOST_REQUIRE( fill_order_op.open_owner == "alice" );
      BOOST_REQUIRE( fill_order_op.open_orderid == 6 );
      BOOST_REQUIRE( fill_order_op.open_pays.amount.value == asset( alice_smt.amount.value / 20, any_smt_symbol ).amount.value );
      BOOST_REQUIRE( fill_order_op.current_owner == "sam" );
      BOOST_REQUIRE( fill_order_op.current_orderid == 8 );
      BOOST_REQUIRE( fill_order_op.current_pays.amount.value == asset( alice_smt.amount.value / 20, STEEM_SYMBOL ).amount.value );

      reward = liquidity_idx.find( db->get_account( "alice" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "alice" ).id );
      BOOST_REQUIRE( reward->sbd_volume == alice_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == alice_steem_volume );
      BOOST_CHECK( reward->last_update == alice_reward_last_update );*/

      reward = liquidity_idx.find( db->get_account( "bob" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "bob" ).id );
      BOOST_REQUIRE( reward->sbd_volume == bob_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == bob_steem_volume );
      BOOST_CHECK( reward->last_update == bob_reward_last_update );*/

      reward = liquidity_idx.find( db->get_account( "sam" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "sam" ).id );
      BOOST_REQUIRE( reward->sbd_volume == sam_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == sam_steem_volume );
      BOOST_CHECK( reward->last_update == sam_reward_last_update );*/

      BOOST_TEST_MESSAGE( "Trading to give Alice and Bob positive volumes to receive rewards" );

      transfer_operation transfer;
      transfer.to = "dave";
      transfer.from = "alice";
      transfer.amount = asset( alice_smt.amount / 2, any_smt_symbol );

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( transfer );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      op.owner = "alice";
      op.amount_to_sell = asset( 8 * ( alice_smt.amount.value / 20 ), STEEM_SYMBOL );
      op.min_to_receive = asset( op.amount_to_sell.amount, any_smt_symbol );
      op.orderid = 9;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      generate_blocks( db->head_block_time() + STEEM_MIN_LIQUIDITY_REWARD_PERIOD_SEC_HF10, true );

      op.owner = "dave";
      op.amount_to_sell = asset( 7 * ( alice_smt.amount.value / 20 ), any_smt_symbol );;
      op.min_to_receive = asset( op.amount_to_sell.amount, STEEM_SYMBOL );
      op.orderid = 10;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, dave_private_key );
      db->push_transaction( tx, 0 );

      alice_smt_volume += op.amount_to_sell.amount.value;
      alice_reward_last_update = db->head_block_time();
      dave_smt_volume -= op.amount_to_sell.amount.value;
      dave_reward_last_update = db->head_block_time();

      ops = get_last_operations( 1 );
      fill_order_op = ops[0].get< fill_order_operation >();

      BOOST_REQUIRE( fill_order_op.open_owner == "alice" );
      BOOST_REQUIRE( fill_order_op.open_orderid == 9 );
      BOOST_REQUIRE( fill_order_op.open_pays.amount.value == 7 * ( alice_smt.amount.value / 20 ) );
      BOOST_REQUIRE( fill_order_op.current_owner == "dave" );
      BOOST_REQUIRE( fill_order_op.current_orderid == 10 );
      BOOST_REQUIRE( fill_order_op.current_pays.amount.value == 7 * ( alice_smt.amount.value / 20 ) );

      reward = liquidity_idx.find( db->get_account( "alice" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "alice" ).id );
      BOOST_REQUIRE( reward->sbd_volume == alice_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == alice_steem_volume );
      BOOST_CHECK( reward->last_update == alice_reward_last_update );*/

      reward = liquidity_idx.find( db->get_account( "bob" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "bob" ).id );
      BOOST_REQUIRE( reward->sbd_volume == bob_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == bob_steem_volume );
      BOOST_CHECK( reward->last_update == bob_reward_last_update );*/

      reward = liquidity_idx.find( db->get_account( "sam" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "sam" ).id );
      BOOST_REQUIRE( reward->sbd_volume == sam_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == sam_steem_volume );
      BOOST_CHECK( reward->last_update == sam_reward_last_update );*/

      reward = liquidity_idx.find( db->get_account( "dave" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "dave" ).id );
      BOOST_REQUIRE( reward->sbd_volume == dave_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == dave_steem_volume );
      BOOST_CHECK( reward->last_update == dave_reward_last_update );*/

      op.owner = "bob";
      op.amount_to_sell.amount = alice_smt.amount / 20;
      op.min_to_receive.amount = op.amount_to_sell.amount;
      op.orderid = 11;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      alice_smt_volume += op.amount_to_sell.amount.value;
      alice_reward_last_update = db->head_block_time();
      bob_smt_volume -= op.amount_to_sell.amount.value;
      bob_reward_last_update = db->head_block_time();

      ops = get_last_operations( 1 );
      fill_order_op = ops[0].get< fill_order_operation >();

      BOOST_REQUIRE( fill_order_op.open_owner == "alice" );
      BOOST_REQUIRE( fill_order_op.open_orderid == 9 );
      BOOST_REQUIRE( fill_order_op.open_pays.amount.value == alice_smt.amount.value / 20 );
      BOOST_REQUIRE( fill_order_op.current_owner == "bob" );
      BOOST_REQUIRE( fill_order_op.current_orderid == 11 );
      BOOST_REQUIRE( fill_order_op.current_pays.amount.value == alice_smt.amount.value / 20 );

      reward = liquidity_idx.find( db->get_account( "alice" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "alice" ).id );
      BOOST_REQUIRE( reward->sbd_volume == alice_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == alice_steem_volume );
      BOOST_CHECK( reward->last_update == alice_reward_last_update );*/

      reward = liquidity_idx.find( db->get_account( "bob" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "bob" ).id );
      BOOST_REQUIRE( reward->sbd_volume == bob_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == bob_steem_volume );
      BOOST_CHECK( reward->last_update == bob_reward_last_update );*/

      reward = liquidity_idx.find( db->get_account( "sam" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "sam" ).id );
      BOOST_REQUIRE( reward->sbd_volume == sam_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == sam_steem_volume );
      BOOST_CHECK( reward->last_update == sam_reward_last_update );*/

      reward = liquidity_idx.find( db->get_account( "dave" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "dave" ).id );
      BOOST_REQUIRE( reward->sbd_volume == dave_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == dave_steem_volume );
      BOOST_CHECK( reward->last_update == dave_reward_last_update );*/

      transfer.to = "bob";
      transfer.from = "alice";
      transfer.amount = asset( alice_smt.amount / 5, any_smt_symbol );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( transfer );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      op.owner = "bob";
      op.orderid = 12;
      op.amount_to_sell = asset( 3 * ( alice_smt.amount / 40 ), any_smt_symbol );
      op.min_to_receive = asset( op.amount_to_sell.amount, STEEM_SYMBOL );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      generate_blocks( db->head_block_time() + STEEM_MIN_LIQUIDITY_REWARD_PERIOD_SEC_HF10, true );

      op.owner = "dave";
      op.orderid = 13;
      op.amount_to_sell = op.min_to_receive;
      op.min_to_receive.symbol = any_smt_symbol;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, dave_private_key );
      db->push_transaction( tx, 0 );

      bob_steem_volume += op.amount_to_sell.amount.value;
      bob_reward_last_update = db->head_block_time();
      dave_steem_volume -= op.amount_to_sell.amount.value;
      dave_reward_last_update = db->head_block_time();

      ops = get_last_operations( 1 );
      fill_order_op = ops[0].get< fill_order_operation >();

      BOOST_REQUIRE( fill_order_op.open_owner == "bob" );
      BOOST_REQUIRE( fill_order_op.open_orderid == 12 );
      BOOST_REQUIRE( fill_order_op.open_pays.amount.value == 3 * ( alice_smt.amount.value / 40 ) );
      BOOST_REQUIRE( fill_order_op.current_owner == "dave" );
      BOOST_REQUIRE( fill_order_op.current_orderid == 13 );
      BOOST_REQUIRE( fill_order_op.current_pays.amount.value == 3 * ( alice_smt.amount.value / 40 ) );

      reward = liquidity_idx.find( db->get_account( "alice" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "alice" ).id );
      BOOST_REQUIRE( reward->sbd_volume == alice_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == alice_steem_volume );
      BOOST_CHECK( reward->last_update == alice_reward_last_update );*/

      reward = liquidity_idx.find( db->get_account( "bob" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "bob" ).id );
      BOOST_REQUIRE( reward->sbd_volume == bob_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == bob_steem_volume );
      BOOST_CHECK( reward->last_update == bob_reward_last_update );*/

      reward = liquidity_idx.find( db->get_account( "sam" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "sam" ).id );
      BOOST_REQUIRE( reward->sbd_volume == sam_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == sam_steem_volume );
      BOOST_CHECK( reward->last_update == sam_reward_last_update );*/

      reward = liquidity_idx.find( db->get_account( "dave" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_REQUIRE( reward->owner == db->get_account( "dave" ).id );
      BOOST_REQUIRE( reward->sbd_volume == dave_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == dave_steem_volume );
      BOOST_CHECK( reward->last_update == dave_reward_last_update );*/

      auto alice_balance = db->get_account( "alice" ).balance;
      auto bob_balance = db->get_account( "bob" ).balance;
      auto sam_balance = db->get_account( "sam" ).balance;
      auto dave_balance = db->get_account( "dave" ).balance;

      BOOST_TEST_MESSAGE( "Generating Blocks to trigger liquidity rewards" );

      db->liquidity_rewards_enabled = true;
      generate_blocks( STEEM_LIQUIDITY_REWARD_BLOCKS - ( db->head_block_num() % STEEM_LIQUIDITY_REWARD_BLOCKS ) - 1 );

      BOOST_REQUIRE( db->head_block_num() % STEEM_LIQUIDITY_REWARD_BLOCKS == STEEM_LIQUIDITY_REWARD_BLOCKS - 1 );
      BOOST_REQUIRE( db->get_account( "alice" ).balance.amount.value == alice_balance.amount.value );
      BOOST_REQUIRE( db->get_account( "bob" ).balance.amount.value == bob_balance.amount.value );
      BOOST_REQUIRE( db->get_account( "sam" ).balance.amount.value == sam_balance.amount.value );
      BOOST_REQUIRE( db->get_account( "dave" ).balance.amount.value == dave_balance.amount.value );

      generate_block();

      //alice_balance += STEEM_MIN_LIQUIDITY_REWARD;

      BOOST_REQUIRE( db->get_account( "alice" ).balance.amount.value == alice_balance.amount.value );
      BOOST_REQUIRE( db->get_account( "bob" ).balance.amount.value == bob_balance.amount.value );
      BOOST_REQUIRE( db->get_account( "sam" ).balance.amount.value == sam_balance.amount.value );
      BOOST_REQUIRE( db->get_account( "dave" ).balance.amount.value == dave_balance.amount.value );

      ops = get_last_operations( 1 );

      STEEM_REQUIRE_THROW( ops[0].get< liquidity_reward_operation>(), fc::exception );
      //BOOST_REQUIRE( ops[0].get< liquidity_reward_operation>().payout.amount.value == STEEM_MIN_LIQUIDITY_REWARD.amount.value );

      generate_blocks( STEEM_LIQUIDITY_REWARD_BLOCKS );

      //bob_balance += STEEM_MIN_LIQUIDITY_REWARD;

      BOOST_REQUIRE( db->get_account( "alice" ).balance.amount.value == alice_balance.amount.value );
      BOOST_REQUIRE( db->get_account( "bob" ).balance.amount.value == bob_balance.amount.value );
      BOOST_REQUIRE( db->get_account( "sam" ).balance.amount.value == sam_balance.amount.value );
      BOOST_REQUIRE( db->get_account( "dave" ).balance.amount.value == dave_balance.amount.value );

      ops = get_last_operations( 1 );

      STEEM_REQUIRE_THROW( ops[0].get< liquidity_reward_operation>(), fc::exception );
      //BOOST_REQUIRE( ops[0].get< liquidity_reward_operation>().payout.amount.value == STEEM_MIN_LIQUIDITY_REWARD.amount.value );

      alice_steem_volume = 0;
      alice_smt_volume = 0;
      bob_steem_volume = 0;
      bob_smt_volume = 0;

      BOOST_TEST_MESSAGE( "Testing liquidity timeout" );

      generate_blocks( sam_reward_last_update + STEEM_LIQUIDITY_TIMEOUT_SEC - fc::seconds( STEEM_BLOCK_INTERVAL / 2 ) - STEEM_MIN_LIQUIDITY_REWARD_PERIOD_SEC , true );

      op.owner = "sam";
      op.orderid = 14;
      op.amount_to_sell = ASSET( "1.000 TESTS" );
      op.min_to_receive = ASSET( "1.000 TBD" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, sam_private_key );
      db->push_transaction( tx, 0 );

      generate_blocks( db->head_block_time() + ( STEEM_BLOCK_INTERVAL / 2 ) + STEEM_LIQUIDITY_TIMEOUT_SEC, true );

      reward = liquidity_idx.find( db->get_account( "sam" ).id );
      /*BOOST_REQUIRE( reward == liquidity_idx.end() );
      BOOST_REQUIRE( reward->owner == db->get_account( "sam" ).id );
      BOOST_REQUIRE( reward->sbd_volume == sam_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == sam_steem_volume );
      BOOST_CHECK( reward->last_update == sam_reward_last_update );*/

      generate_block();

      op.owner = "alice";
      op.orderid = 15;
      op.amount_to_sell.symbol = any_smt_symbol;
      op.min_to_receive.symbol = STEEM_SYMBOL;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      sam_smt_volume = ASSET( "1.000 TBD" ).amount.value;
      sam_steem_volume = 0;
      sam_reward_last_update = db->head_block_time();

      reward = liquidity_idx.find( db->get_account( "sam" ).id );
      /*BOOST_REQUIRE( reward == liquidity_idx.end() );
      BOOST_REQUIRE( reward->owner == db->get_account( "sam" ).id );
      BOOST_REQUIRE( reward->sbd_volume == sam_smt_volume );
      BOOST_REQUIRE( reward->steem_volume == sam_steem_volume );
      BOOST_CHECK( reward->last_update == sam_reward_last_update );*/

      validate_database();
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( smt_refunds )
{
   try
   {

      BOOST_TEST_MESSAGE( "Testing SMT contribution refunds" );
      ACTORS( (alice)(bob)(sam)(dave) )

      generate_block();

      auto bobs_balance  = asset( 1000000, STEEM_SYMBOL );
      auto sams_balance  = asset( 800000, STEEM_SYMBOL );
      auto daves_balance = asset( 600000, STEEM_SYMBOL );

      FUND( "bob", bobs_balance );
      FUND( "sam", sams_balance );
      FUND( "dave", daves_balance );

      generate_block();

      BOOST_TEST_MESSAGE( " --- SMT creation" );
      auto symbol = create_smt( "alice", alice_private_key, 3 );
      const auto& token = db->get< smt_token_object, by_symbol >( symbol );

      BOOST_TEST_MESSAGE( " --- SMT setup" );
      signed_transaction tx;
      smt_setup_operation setup_op;

      uint64_t contribution_window_blocks = 10;
      setup_op.control_account = "alice";
      setup_op.symbol = symbol;
      setup_op.contribution_begin_time = db->head_block_time() + STEEM_BLOCK_INTERVAL;
      setup_op.contribution_end_time = setup_op.contribution_begin_time + ( STEEM_BLOCK_INTERVAL * contribution_window_blocks );
      setup_op.steem_units_min      = 2400001;
      setup_op.steem_units_soft_cap = 2400001;
      setup_op.steem_units_hard_cap = 4000000;
      setup_op.max_supply = STEEM_MAX_SHARE_SUPPLY;
      setup_op.launch_time = setup_op.contribution_end_time + STEEM_BLOCK_INTERVAL;
      setup_op.initial_generation_policy = get_capped_generation_policy
      (
         get_generation_unit( { { "alice", 1 } }, { { "alice", 2 } } ), /* pre_soft_cap_unit */
         get_generation_unit( { { "alice", 1 } }, { { "alice", 2 } } ), /* post_soft_cap_unit */
         1,                                                             /* min_unit_ratio */
         2                                                              /* max_unit_ratio */
      );

      tx.operations.push_back( setup_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );
      tx.operations.clear();
      tx.signatures.clear();

      BOOST_REQUIRE( token.phase == smt_phase::setup_completed );

      generate_block();

      BOOST_REQUIRE( token.phase == smt_phase::ico );

      BOOST_TEST_MESSAGE( " --- SMT contributions" );
      uint32_t num_contributions = 0;
      for ( uint64_t i = 0; i < contribution_window_blocks; i++ )
      {
         smt_contribute_operation contrib_op;

         contrib_op.symbol = symbol;
         contrib_op.contribution_id = i;

         contrib_op.contributor = "bob";
         contrib_op.contribution = asset( bobs_balance.amount / contribution_window_blocks, STEEM_SYMBOL );


         tx.operations.push_back( contrib_op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, bob_private_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();
         num_contributions++;

         contrib_op.contributor = "sam";
         contrib_op.contribution = asset( sams_balance.amount / contribution_window_blocks, STEEM_SYMBOL );

         tx.operations.push_back( contrib_op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, sam_private_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();
         num_contributions++;

         contrib_op.contributor = "dave";
         contrib_op.contribution = asset( daves_balance.amount / contribution_window_blocks, STEEM_SYMBOL );

         tx.operations.push_back( contrib_op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, dave_private_key );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();
         num_contributions++;

         if ( i < contribution_window_blocks - 1 )
            generate_block();

         validate_database();
      }

      BOOST_REQUIRE( token.phase == smt_phase::ico );

      BOOST_TEST_MESSAGE( " --- Checking contributor balances" );

      BOOST_REQUIRE( db->get_balance( "bob", STEEM_SYMBOL ) == asset( 0, STEEM_SYMBOL ) );
      BOOST_REQUIRE( db->get_balance( "sam", STEEM_SYMBOL ) == asset( 0, STEEM_SYMBOL ) );
      BOOST_REQUIRE( db->get_balance( "dave", STEEM_SYMBOL ) == asset( 0, STEEM_SYMBOL ) );

      generate_block();

      BOOST_REQUIRE( token.phase == smt_phase::launch_failed );

      validate_database();

      BOOST_TEST_MESSAGE( " --- Starting the cascading refunds" );

      generate_blocks( num_contributions / 2 );

      validate_database();

      generate_blocks( num_contributions / 2 + 1 );

      BOOST_TEST_MESSAGE( " --- Checking contributor balances" );

      BOOST_REQUIRE( db->get_balance( "bob", STEEM_SYMBOL ) == bobs_balance );
      BOOST_REQUIRE( db->get_balance( "sam", STEEM_SYMBOL ) == sams_balance );
      BOOST_REQUIRE( db->get_balance( "dave", STEEM_SYMBOL ) == daves_balance );

      validate_database();

      auto& ico_idx = db->get_index< smt_ico_index, by_symbol >();
      BOOST_REQUIRE( ico_idx.find( symbol ) == ico_idx.end() );
      auto& contribution_idx = db->get_index< smt_contribution_index, by_symbol_id >();
      BOOST_REQUIRE( contribution_idx.find( boost::make_tuple( symbol, 0 ) ) == contribution_idx.end() );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_ico_payouts )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing SMT ICO payouts" );
      ACTORS( (creator)(alice)(bob)(charlie)(dan)(elaine)(fred)(george)(henry) )

      generate_block();

      auto alices_balance    = asset( 5000000, STEEM_SYMBOL );
      auto bobs_balance      = asset( 25000000, STEEM_SYMBOL );
      auto charlies_balance  = asset( 10000000, STEEM_SYMBOL );
      auto dans_balance      = asset( 25000000, STEEM_SYMBOL );
      auto elaines_balance   = asset( 60000000, STEEM_SYMBOL );
      auto freds_balance     = asset( 0, STEEM_SYMBOL );
      auto georges_balance   = asset( 0, STEEM_SYMBOL );
      auto henrys_balance    = asset( 0, STEEM_SYMBOL );

      std::map< std ::string, std::tuple< share_type, fc::ecc::private_key > > contributor_contributions {
         { "alice",   { alices_balance.amount,   alice_private_key   } },
         { "bob",     { bobs_balance.amount,     bob_private_key     } },
         { "charlie", { charlies_balance.amount, charlie_private_key } },
         { "dan",     { dans_balance.amount,     dan_private_key     } },
         { "elaine",  { elaines_balance.amount,  elaine_private_key  } },
         { "fred",    { freds_balance.amount,    fred_private_key    } },
         { "george",  { georges_balance.amount,  george_private_key  } },
         { "henry",   { henrys_balance.amount,   henry_private_key   } }
      };

      for ( auto& e : contributor_contributions )
      {
         FUND( e.first, asset( std::get< 0 >( e.second ), STEEM_SYMBOL ) );
      }

      generate_block();

      BOOST_TEST_MESSAGE( " --- SMT creation" );
      auto symbol = create_smt( "creator", creator_private_key, 3 );
      const auto& token = db->get< smt_token_object, by_symbol >( symbol );

      BOOST_TEST_MESSAGE( " --- SMT setup" );
      signed_transaction tx;
      smt_setup_operation setup_op;

      uint64_t contribution_window_blocks = 5;
      setup_op.control_account = "creator";
      setup_op.symbol = symbol;
      setup_op.contribution_begin_time = db->head_block_time() + STEEM_BLOCK_INTERVAL;
      setup_op.contribution_end_time = setup_op.contribution_begin_time + ( STEEM_BLOCK_INTERVAL * contribution_window_blocks );
      setup_op.steem_units_min      = 0;
      setup_op.steem_units_soft_cap = 100000000;
      setup_op.steem_units_hard_cap = 150000000;
      setup_op.max_supply = STEEM_MAX_SHARE_SUPPLY;
      setup_op.launch_time = setup_op.contribution_end_time + STEEM_BLOCK_INTERVAL;
      setup_op.initial_generation_policy = get_capped_generation_policy
      (
         get_generation_unit(
         {
            { "fred", 3 },
            { "george", 2 }
         },
         {
            { "$from", 7 },
            { "george", 1 },
            { "henry", 2 }
         } ), /* pre_soft_cap_unit */
         get_generation_unit(
         {
            { "fred", 3 },
            { "george", 2 }
         },
         {
            { "$from", 7 },
            { "george", 1 },
            { "henry", 2 }
         } ), /* post_soft_cap_unit */
         50,                                                            /* min_unit_ratio */
         100                                                            /* max_unit_ratio */
      );

      tx.operations.push_back( setup_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, creator_private_key );
      db->push_transaction( tx, 0 );
      tx.operations.clear();
      tx.signatures.clear();

      BOOST_REQUIRE( token.phase == smt_phase::setup_completed );

      generate_block();

      BOOST_REQUIRE( token.phase == smt_phase::ico );

      BOOST_TEST_MESSAGE( " --- SMT contributions" );

      uint32_t num_contributions = 0;
      for ( auto& e : contributor_contributions )
      {
         if ( std::get< 0 >( e.second ) == 0 )
            continue;

         smt_contribute_operation contrib_op;

         contrib_op.symbol = symbol;
         contrib_op.contribution_id = 0;
         contrib_op.contributor = e.first;
         contrib_op.contribution = asset( std::get< 0 >( e.second ), STEEM_SYMBOL );

         tx.operations.push_back( contrib_op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, std::get< 1 >( e.second ) );
         db->push_transaction( tx, 0 );
         tx.operations.clear();
         tx.signatures.clear();

         generate_block();
         num_contributions++;
      }

      validate_database();

      generate_block();

      BOOST_REQUIRE( token.phase == smt_phase::launch_success );

      BOOST_TEST_MESSAGE( " --- Starting the cascading payouts" );

      generate_blocks( num_contributions / 2 );

      validate_database();

      generate_blocks( num_contributions / 2 + 1 );

      BOOST_TEST_MESSAGE( " --- Checking contributor balances" );

      BOOST_REQUIRE( db->get_balance( "alice", STEEM_SYMBOL ).amount == 0 );
      BOOST_REQUIRE( db->get_balance( "bob", STEEM_SYMBOL ).amount == 0 );
      BOOST_REQUIRE( db->get_balance( "charlie", STEEM_SYMBOL ).amount == 0 );
      BOOST_REQUIRE( db->get_balance( "dan", STEEM_SYMBOL ).amount == 0 );
      BOOST_REQUIRE( db->get_balance( "elaine", STEEM_SYMBOL ).amount == 0 );
      BOOST_REQUIRE( db->get_balance( "fred", STEEM_SYMBOL ).amount == 75000000 );
      BOOST_REQUIRE( db->get_balance( "george", STEEM_SYMBOL ).amount == 50000000 );
      BOOST_REQUIRE( db->get_balance( "henry", STEEM_SYMBOL ).amount == 0 );

      BOOST_REQUIRE( db->get_balance( "alice", symbol ).amount == 420000000 );
      BOOST_REQUIRE( db->get_balance( "bob", symbol ).amount == 2100000000 );
      BOOST_REQUIRE( db->get_balance( "charlie", symbol ).amount == 840000000 );
      BOOST_REQUIRE( db->get_balance( "dan", symbol ).amount == 2100000000 );
      BOOST_REQUIRE( db->get_balance( "elaine", symbol ).amount == 5040000000 );
      BOOST_REQUIRE( db->get_balance( "fred", symbol ).amount == 0 );
      BOOST_REQUIRE( db->get_balance( "george", symbol ).amount == 1500000000 );
      BOOST_REQUIRE( db->get_balance( "henry", symbol ).amount == 3000000000 );

      validate_database();

      auto& ico_idx = db->get_index< smt_ico_index, by_symbol >();
      BOOST_REQUIRE( ico_idx.find( symbol ) == ico_idx.end() );
      auto& contribution_idx = db->get_index< smt_contribution_index, by_symbol_id >();
      BOOST_REQUIRE( contribution_idx.find( boost::make_tuple( symbol, 0 ) ) == contribution_idx.end() );
   }
   FC_LOG_AND_RETHROW()
}
BOOST_AUTO_TEST_SUITE_END()
#endif
