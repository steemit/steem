#include <fc/macros.hpp>

#if defined IS_TEST_NET

#include <boost/test/unit_test.hpp>

#include <steem/chain/steem_fwd.hpp>

#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/hardfork.hpp>
#include <steem/protocol/smt_util.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/database_exceptions.hpp>
#include <steem/chain/steem_objects.hpp>
#include <steem/chain/smt_objects.hpp>

#include <steem/chain/util/nai_generator.hpp>
#include <steem/chain/util/smt_token.hpp>

#include "../db_fixture/database_fixture.hpp"

#include <fc/uint128.hpp>

using namespace steem::chain;
using namespace steem::protocol;
using fc::string;
using fc::uint128_t;
using boost::container::flat_set;

BOOST_FIXTURE_TEST_SUITE( smt_operation_tests, smt_database_fixture )

BOOST_AUTO_TEST_CASE( smt_limit_order_create_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_limit_order_create_authorities" );

      ACTORS( (alice)(bob) )

      limit_order_create_operation op;

      //Create SMT and give some SMT to creator.
      signed_transaction tx;
      asset_symbol_type alice_symbol = create_smt( "alice", alice_private_key, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      op.owner = "alice";
      op.amount_to_sell = ASSET( "1.000 TESTS" );
      op.min_to_receive = asset( 1000, alice_symbol );
      op.expiration = db->head_block_time() + fc::seconds( STEEM_MAX_TIME_UNTIL_EXPIRATION );

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );

      BOOST_TEST_MESSAGE( "--- Test failure when no signature." );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test success with account signature" );
      sign( tx, alice_private_key );
      db->push_transaction( tx, database::skip_transaction_dupe_check );

      BOOST_TEST_MESSAGE( "--- Test failure with duplicate signature" );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure with additional incorrect signature" );
      tx.signatures.clear();
      sign( tx, alice_private_key );
      sign( tx, bob_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test failure with incorrect signature" );
      tx.signatures.clear();
      sign( tx, alice_post_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_limit_order_create2_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_limit_order_create2_authorities" );

      ACTORS( (alice)(bob) )

      limit_order_create2_operation op;

      //Create SMT and give some SMT to creator.
      signed_transaction tx;
      asset_symbol_type alice_symbol = create_smt( "alice", alice_private_key, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      op.owner = "alice";
      op.amount_to_sell = ASSET( "1.000 TESTS" );
      op.exchange_rate = price( ASSET( "1.000 TESTS" ), asset( 1000, alice_symbol ) );
      op.expiration = db->head_block_time() + fc::seconds( STEEM_MAX_LIMIT_ORDER_EXPIRATION );

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );

      BOOST_TEST_MESSAGE( "--- Test failure when no signature." );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test success with account signature" );
      sign( tx, alice_private_key );
      db->push_transaction( tx, database::skip_transaction_dupe_check );

      BOOST_TEST_MESSAGE( "--- Test failure with duplicate signature" );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure with additional incorrect signature" );
      tx.signatures.clear();
      sign( tx, alice_private_key );
      sign( tx, bob_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test failure with incorrect signature" );
      tx.signatures.clear();
      sign( tx, alice_post_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_limit_order_create_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: limit_order_create_apply" );

      ACTORS( (alice)(bob) )

      //Create SMT and give some SMT to creators.
      signed_transaction tx;
      asset_symbol_type alice_symbol = create_smt( "alice", alice_private_key, 3);

      const account_object& alice_account = db->get_account( "alice" );
      const account_object& bob_account = db->get_account( "bob" );

      asset alice_0 = asset( 0, alice_symbol );

      FUND( "bob", 1000000 );
      convert( "bob", ASSET("1000.000 TESTS" ) );
      generate_block();

      asset alice_smt_balance = asset( 1000000, alice_symbol );
      asset bob_smt_balance = asset( 1000000, alice_symbol );

      asset alice_balance = alice_account.balance;

      asset bob_balance = bob_account.balance;

      FUND( "alice", alice_smt_balance );
      FUND( "bob", bob_smt_balance );

      tx.operations.clear();
      tx.signatures.clear();

      const auto& limit_order_idx = db->get_index< limit_order_index >().indices().get< by_account >();

      BOOST_TEST_MESSAGE( "--- Test failure when account does not have required funds" );
      limit_order_create_operation op;

      op.owner = "bob";
      op.orderid = 1;
      op.amount_to_sell = ASSET( "10.000 TESTS" );
      op.min_to_receive = asset( 10000, alice_symbol );
      op.fill_or_kill = false;
      op.expiration = db->head_block_time() + fc::seconds( STEEM_MAX_LIMIT_ORDER_EXPIRATION );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, bob_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( limit_order_idx.find( boost::make_tuple( "bob", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( db->get_balance( bob_account, STEEM_SYMBOL ).amount.value == bob_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, alice_symbol ).amount.value == bob_smt_balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when amount to receive is 0" );

      op.owner = "alice";
      op.min_to_receive = alice_0;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( limit_order_idx.find( boost::make_tuple( "alice", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( db->get_balance( bob_account, STEEM_SYMBOL ).amount.value == bob_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, alice_symbol ).amount.value == bob_smt_balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when amount to sell is 0" );

      op.amount_to_sell = alice_0;
      op.min_to_receive = ASSET( "10.000 TESTS" ) ;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( limit_order_idx.find( boost::make_tuple( "alice", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( db->get_balance( bob_account, STEEM_SYMBOL ).amount.value == bob_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when expiration is too long" );
      op.amount_to_sell = ASSET( "10.000 TESTS" );
      op.min_to_receive = ASSET( "15.000 TBD" );
      op.expiration = db->head_block_time() + fc::seconds( STEEM_MAX_LIMIT_ORDER_EXPIRATION + 1 );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_TEST_MESSAGE( "--- Test success creating limit order that will not be filled" );

      op.expiration = db->head_block_time() + fc::seconds( STEEM_MAX_LIMIT_ORDER_EXPIRATION );
      op.amount_to_sell = ASSET( "10.000 TESTS" );
      op.min_to_receive = asset( 15000, alice_symbol );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      alice_balance -= ASSET( "10.000 TESTS" );

      auto limit_order = limit_order_idx.find( boost::make_tuple( "alice", op.orderid ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == op.owner );
      BOOST_REQUIRE( limit_order->orderid == op.orderid );
      BOOST_REQUIRE( limit_order->for_sale == op.amount_to_sell.amount );
      BOOST_REQUIRE( limit_order->sell_price == price( op.amount_to_sell / op.min_to_receive ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( alice_symbol, STEEM_SYMBOL ) );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, STEEM_SYMBOL ).amount.value == alice_balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure creating limit order with duplicate id" );

      op.amount_to_sell = ASSET( "20.000 TESTS" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      limit_order = limit_order_idx.find( boost::make_tuple( "alice", op.orderid ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == op.owner );
      BOOST_REQUIRE( limit_order->orderid == op.orderid );
      BOOST_REQUIRE( limit_order->for_sale == 10000 );
      BOOST_REQUIRE( limit_order->sell_price == price( ASSET( "10.000 TESTS" ), op.min_to_receive ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( alice_symbol, STEEM_SYMBOL ) );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, STEEM_SYMBOL ).amount.value == alice_balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test sucess killing an order that will not be filled" );

      op.orderid = 2;
      op.fill_or_kill = true;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( limit_order_idx.find( boost::make_tuple( "alice", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, STEEM_SYMBOL ).amount.value == alice_balance.amount.value );
      validate_database();

      // BOOST_TEST_MESSAGE( "--- Test having a partial match to limit order" );
      // // Alice has order for 15 SMT at a price of 2:3
      // // Fill 5 STEEM for 7.5 SMT

      op.owner = "bob";
      op.orderid = 1;
      op.amount_to_sell = asset (7500, alice_symbol );
      op.min_to_receive = ASSET( "5.000 TESTS" );
      op.fill_or_kill = false;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      bob_smt_balance -= asset (7500, alice_symbol );
      alice_smt_balance += asset (7500, alice_symbol );
      bob_balance += ASSET( "5.000 TESTS" );

      auto recent_ops = get_last_operations( 1 );
      auto fill_order_op = recent_ops[0].get< fill_order_operation >();

      limit_order = limit_order_idx.find( boost::make_tuple( "alice", 1 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == "alice" );
      BOOST_REQUIRE( limit_order->orderid == op.orderid );
      BOOST_REQUIRE( limit_order->for_sale == 5000 );
      BOOST_REQUIRE( limit_order->sell_price == price( ASSET( "10.000 TESTS" ), asset( 15000, alice_symbol ) ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( alice_symbol, STEEM_SYMBOL ) );
      BOOST_REQUIRE( limit_order_idx.find( boost::make_tuple( "bob", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, STEEM_SYMBOL ).amount.value == alice_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, alice_symbol ).amount.value == bob_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, STEEM_SYMBOL ).amount.value == bob_balance.amount.value );
      BOOST_REQUIRE( fill_order_op.open_owner == "alice" );
      BOOST_REQUIRE( fill_order_op.open_orderid == 1 );
      BOOST_REQUIRE( fill_order_op.open_pays.amount.value == ASSET( "5.000 TESTS").amount.value );
      BOOST_REQUIRE( fill_order_op.current_owner == "bob" );
      BOOST_REQUIRE( fill_order_op.current_orderid == 1 );
      BOOST_REQUIRE( fill_order_op.current_pays.amount.value == asset(7500, alice_symbol ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test filling an existing order fully, but the new order partially" );

      op.amount_to_sell = asset( 15000, alice_symbol );
      op.min_to_receive = ASSET( "10.000 TESTS" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      bob_smt_balance -= asset( 15000, alice_symbol );
      alice_smt_balance += asset( 7500, alice_symbol );
      bob_balance += ASSET( "5.000 TESTS" );

      limit_order = limit_order_idx.find( boost::make_tuple( "bob", 1 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == "bob" );
      BOOST_REQUIRE( limit_order->orderid == 1 );
      BOOST_REQUIRE( limit_order->for_sale.value == 7500 );
      BOOST_REQUIRE( limit_order->sell_price == price( asset( 15000, alice_symbol ), ASSET( "10.000 TESTS" ) ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( alice_symbol, STEEM_SYMBOL ) );
      BOOST_REQUIRE( limit_order_idx.find( boost::make_tuple( "alice", 1 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, STEEM_SYMBOL ).amount.value == alice_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, alice_symbol ).amount.value == bob_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, STEEM_SYMBOL ).amount.value == bob_balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test filling an existing order and new order fully" );

      op.owner = "alice";
      op.orderid = 3;
      op.amount_to_sell = ASSET( "5.000 TESTS" );
      op.min_to_receive = asset( 7500, alice_symbol );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      alice_balance -= ASSET( "5.000 TESTS" );
      alice_smt_balance += asset( 7500, alice_symbol );
      bob_balance += ASSET( "5.000 TESTS" );

      BOOST_REQUIRE( limit_order_idx.find( boost::make_tuple( "alice", 3 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( limit_order_idx.find( boost::make_tuple( "bob", 1 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, STEEM_SYMBOL ).amount.value == alice_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, alice_symbol ).amount.value == bob_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, STEEM_SYMBOL ).amount.value == bob_balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test filling limit order with better order when partial order is better." );

      op.owner = "alice";
      op.orderid = 4;
      op.amount_to_sell = ASSET( "10.000 TESTS" );
      op.min_to_receive = asset( 11000, alice_symbol );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      op.owner = "bob";
      op.orderid = 4;
      op.amount_to_sell = asset( 12000, alice_symbol );
      op.min_to_receive = ASSET( "10.000 TESTS" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      alice_balance -= ASSET( "10.000 TESTS" );
      alice_smt_balance += asset( 11000, alice_symbol );
      bob_smt_balance -= asset( 12000, alice_symbol );
      bob_balance += ASSET( "10.000 TESTS" );

      limit_order = limit_order_idx.find( boost::make_tuple( "bob", 4 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order_idx.find(boost::make_tuple( "alice", 4 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == "bob" );
      BOOST_REQUIRE( limit_order->orderid == 4 );
      BOOST_REQUIRE( limit_order->for_sale.value == 1000 );
      BOOST_REQUIRE( limit_order->sell_price == price( asset( 12000, alice_symbol ), ASSET( "10.000 TESTS" ) ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( alice_symbol, STEEM_SYMBOL ) );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, STEEM_SYMBOL ).amount.value == alice_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, alice_symbol ).amount.value == bob_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, STEEM_SYMBOL ).amount.value == bob_balance.amount.value );
      validate_database();

      limit_order_cancel_operation can;
      can.owner = "bob";
      can.orderid = 4;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( can );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Test filling limit order with better order when partial order is worse." );

      //auto gpo = db->get_dynamic_global_properties();
      //auto start_sbd = gpo.current_sbd_supply;

      op.owner = "alice";
      op.orderid = 5;
      op.amount_to_sell = ASSET( "20.000 TESTS" );
      op.min_to_receive = asset( 22000, alice_symbol );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      op.owner = "bob";
      op.orderid = 5;
      op.amount_to_sell = asset( 12000, alice_symbol );
      op.min_to_receive = ASSET( "10.000 TESTS" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      alice_balance -= ASSET( "20.000 TESTS" );
      alice_smt_balance += asset( 12000, alice_symbol );

      bob_smt_balance -= asset( 11000, alice_symbol );
      bob_balance += ASSET( "10.909 TESTS" );

      limit_order = limit_order_idx.find( boost::make_tuple( "alice", 5 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order_idx.find(boost::make_tuple( "bob", 5 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == "alice" );
      BOOST_REQUIRE( limit_order->orderid == 5 );
      BOOST_REQUIRE( limit_order->for_sale.value == 9091 );
      BOOST_REQUIRE( limit_order->sell_price == price( ASSET( "20.000 TESTS" ), asset( 22000, alice_symbol ) ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( alice_symbol, STEEM_SYMBOL ) );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, STEEM_SYMBOL ).amount.value == alice_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, alice_symbol ).amount.value == bob_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, STEEM_SYMBOL ).amount.value == bob_balance.amount.value );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_limit_order_cancel_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_limit_order_cancel_authorities" );

      ACTORS( (alice)(bob) )

      //Create SMT and give some SMT to creator.
      signed_transaction tx;
      asset_symbol_type alice_symbol = create_smt( "alice", alice_private_key, 3 );

      FUND( "alice", asset( 100000, alice_symbol ) );

      tx.operations.clear();
      tx.signatures.clear();

      limit_order_create_operation c;
      c.owner = "alice";
      c.orderid = 1;
      c.amount_to_sell = ASSET( "1.000 TESTS" );
      c.min_to_receive = asset( 1000, alice_symbol );
      c.expiration = db->head_block_time() + fc::seconds( STEEM_MAX_LIMIT_ORDER_EXPIRATION );

      tx.operations.push_back( c );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      limit_order_cancel_operation op;
      op.owner = "alice";
      op.orderid = 1;

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "--- Test failure when no signature." );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test success with account signature" );
      sign( tx, alice_private_key );
      db->push_transaction( tx, database::skip_transaction_dupe_check );

      BOOST_TEST_MESSAGE( "--- Test failure with duplicate signature" );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure with additional incorrect signature" );
      tx.signatures.clear();
      sign( tx, alice_private_key );
      sign( tx, bob_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test failure with incorrect signature" );
      tx.signatures.clear();
      sign( tx, alice_post_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_limit_order_cancel_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_limit_order_cancel_apply" );

      ACTORS( (alice) )

      //Create SMT and give some SMT to creator.
      signed_transaction tx;
      asset_symbol_type alice_symbol = create_smt( "alice", alice_private_key, 3 );

      const account_object& alice_account = db->get_account( "alice" );

      tx.operations.clear();
      tx.signatures.clear();

      asset alice_smt_balance = asset( 1000000, alice_symbol );
      asset alice_balance = alice_account.balance;

      FUND( "alice", alice_smt_balance );

      const auto& limit_order_idx = db->get_index< limit_order_index >().indices().get< by_account >();

      BOOST_TEST_MESSAGE( "--- Test cancel non-existent order" );

      limit_order_cancel_operation op;

      op.owner = "alice";
      op.orderid = 5;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_TEST_MESSAGE( "--- Test cancel order" );

      limit_order_create_operation create;
      create.owner = "alice";
      create.orderid = 5;
      create.amount_to_sell = ASSET( "5.000 TESTS" );
      create.min_to_receive = asset( 7500, alice_symbol );
      create.expiration = db->head_block_time() + fc::seconds( STEEM_MAX_LIMIT_ORDER_EXPIRATION );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( create );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( limit_order_idx.find( boost::make_tuple( "alice", 5 ) ) != limit_order_idx.end() );

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( limit_order_idx.find( boost::make_tuple( "alice", 5 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, STEEM_SYMBOL ).amount.value == alice_balance.amount.value );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_limit_order_create2_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: limit_order_create2_apply" );

      ACTORS( (alice)(bob) )

      //Create SMT and give some SMT to creators.
      signed_transaction tx;
      asset_symbol_type alice_symbol = create_smt( "alice", alice_private_key, 3);

      const account_object& alice_account = db->get_account( "alice" );
      const account_object& bob_account = db->get_account( "bob" );

      asset alice_0 = asset( 0, alice_symbol );

      FUND( "bob", 1000000 );
      convert( "bob", ASSET("1000.000 TESTS" ) );
      generate_block();

      asset alice_smt_balance = asset( 1000000, alice_symbol );
      asset bob_smt_balance = asset( 1000000, alice_symbol );

      asset alice_balance = alice_account.balance;

      asset bob_balance = bob_account.balance;

      FUND( "alice", alice_smt_balance );
      FUND( "bob", bob_smt_balance );

      tx.operations.clear();
      tx.signatures.clear();

      const auto& limit_order_idx = db->get_index< limit_order_index >().indices().get< by_account >();

      BOOST_TEST_MESSAGE( "--- Test failure when account does not have required funds" );
      limit_order_create2_operation op;

      op.owner = "bob";
      op.orderid = 1;
      op.amount_to_sell = ASSET( "10.000 TESTS" );
      op.exchange_rate = price( ASSET( "1.000 TESTS" ), asset( 1000, alice_symbol ) );
      op.fill_or_kill = false;
      op.expiration = db->head_block_time() + fc::seconds( STEEM_MAX_LIMIT_ORDER_EXPIRATION );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, bob_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( limit_order_idx.find( boost::make_tuple( "bob", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( db->get_balance( bob_account, STEEM_SYMBOL ).amount.value == bob_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, alice_symbol ).amount.value == bob_smt_balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when amount to receive is 0" );

      op.owner = "alice";
      op.exchange_rate.base = ASSET( "0.000 TESTS" );
      op.exchange_rate.quote = alice_0;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( limit_order_idx.find( boost::make_tuple( "alice", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( db->get_balance( bob_account, STEEM_SYMBOL ).amount.value == bob_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, alice_symbol ).amount.value == bob_smt_balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when amount to sell is 0" );

      op.amount_to_sell = alice_0;
      op.exchange_rate = price( ASSET( "1.000 TESTS" ), asset( 1000, alice_symbol ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( limit_order_idx.find( boost::make_tuple( "alice", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( db->get_balance( bob_account, STEEM_SYMBOL ).amount.value == bob_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when expiration is too long" );
      op.amount_to_sell = ASSET( "10.000 TESTS" );
      op.exchange_rate = price( ASSET( "2.000 TESTS" ), ASSET( "3.000 TBD" ) );
      op.expiration = db->head_block_time() + fc::seconds( STEEM_MAX_LIMIT_ORDER_EXPIRATION + 1 );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_TEST_MESSAGE( "--- Test success creating limit order that will not be filled" );

      op.expiration = db->head_block_time() + fc::seconds( STEEM_MAX_LIMIT_ORDER_EXPIRATION );
      op.amount_to_sell = ASSET( "10.000 TESTS" );
      op.exchange_rate = price( ASSET( "2.000 TESTS" ), asset( 3000, alice_symbol ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      alice_balance -= ASSET( "10.000 TESTS" );

      auto limit_order = limit_order_idx.find( boost::make_tuple( "alice", op.orderid ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == op.owner );
      BOOST_REQUIRE( limit_order->orderid == op.orderid );
      BOOST_REQUIRE( limit_order->for_sale == op.amount_to_sell.amount );
      BOOST_REQUIRE( limit_order->sell_price == op.exchange_rate );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( alice_symbol, STEEM_SYMBOL ) );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, STEEM_SYMBOL ).amount.value == alice_balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure creating limit order with duplicate id" );

      op.amount_to_sell = ASSET( "20.000 TESTS" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      limit_order = limit_order_idx.find( boost::make_tuple( "alice", op.orderid ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == op.owner );
      BOOST_REQUIRE( limit_order->orderid == op.orderid );
      BOOST_REQUIRE( limit_order->for_sale == 10000 );
      BOOST_REQUIRE( limit_order->sell_price == op.exchange_rate );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( alice_symbol, STEEM_SYMBOL ) );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, STEEM_SYMBOL ).amount.value == alice_balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test sucess killing an order that will not be filled" );

      op.orderid = 2;
      op.fill_or_kill = true;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( limit_order_idx.find( boost::make_tuple( "alice", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, STEEM_SYMBOL ).amount.value == alice_balance.amount.value );
      validate_database();

      // BOOST_TEST_MESSAGE( "--- Test having a partial match to limit order" );
      // // Alice has order for 15 SMT at a price of 2:3
      // // Fill 5 STEEM for 7.5 SMT

      op.owner = "bob";
      op.orderid = 1;
      op.amount_to_sell = asset( 7500, alice_symbol );
      op.exchange_rate = price( asset( 3000, alice_symbol ), ASSET( "2.000 TESTS" ) );
      op.fill_or_kill = false;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      bob_smt_balance -= asset (7500, alice_symbol );
      alice_smt_balance += asset (7500, alice_symbol );
      bob_balance += ASSET( "5.000 TESTS" );

      auto recent_ops = get_last_operations( 1 );
      auto fill_order_op = recent_ops[0].get< fill_order_operation >();

      limit_order = limit_order_idx.find( boost::make_tuple( "alice", 1 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == "alice" );
      BOOST_REQUIRE( limit_order->orderid == op.orderid );
      BOOST_REQUIRE( limit_order->for_sale == 5000 );
      BOOST_REQUIRE( limit_order->sell_price == price( ASSET( "2.000 TESTS" ), asset( 3000, alice_symbol ) ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( alice_symbol, STEEM_SYMBOL ) );
      BOOST_REQUIRE( limit_order_idx.find( boost::make_tuple( "bob", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, STEEM_SYMBOL ).amount.value == alice_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, alice_symbol ).amount.value == bob_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, STEEM_SYMBOL ).amount.value == bob_balance.amount.value );
      BOOST_REQUIRE( fill_order_op.open_owner == "alice" );
      BOOST_REQUIRE( fill_order_op.open_orderid == 1 );
      BOOST_REQUIRE( fill_order_op.open_pays.amount.value == ASSET( "5.000 TESTS").amount.value );
      BOOST_REQUIRE( fill_order_op.current_owner == "bob" );
      BOOST_REQUIRE( fill_order_op.current_orderid == 1 );
      BOOST_REQUIRE( fill_order_op.current_pays.amount.value == asset(7500, alice_symbol ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test filling an existing order fully, but the new order partially" );

      op.amount_to_sell = asset( 15000, alice_symbol );
      op.exchange_rate = price( asset( 3000, alice_symbol ), ASSET( "2.000 TESTS" ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      bob_smt_balance -= asset( 15000, alice_symbol );
      alice_smt_balance += asset( 7500, alice_symbol );
      bob_balance += ASSET( "5.000 TESTS" );

      limit_order = limit_order_idx.find( boost::make_tuple( "bob", 1 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == "bob" );
      BOOST_REQUIRE( limit_order->orderid == 1 );
      BOOST_REQUIRE( limit_order->for_sale.value == 7500 );
      BOOST_REQUIRE( limit_order->sell_price == price( asset( 3000, alice_symbol ), ASSET( "2.000 TESTS" ) ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( alice_symbol, STEEM_SYMBOL ) );
      BOOST_REQUIRE( limit_order_idx.find( boost::make_tuple( "alice", 1 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, STEEM_SYMBOL ).amount.value == alice_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, alice_symbol ).amount.value == bob_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, STEEM_SYMBOL ).amount.value == bob_balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test filling an existing order and new order fully" );

      op.owner = "alice";
      op.orderid = 3;
      op.amount_to_sell = ASSET( "5.000 TESTS" );
      op.exchange_rate = price( ASSET( "2.000 TESTS" ), asset( 3000, alice_symbol ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      alice_balance -= ASSET( "5.000 TESTS" );
      alice_smt_balance += asset( 7500, alice_symbol );
      bob_balance += ASSET( "5.000 TESTS" );

      BOOST_REQUIRE( limit_order_idx.find( boost::make_tuple( "alice", 3 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( limit_order_idx.find( boost::make_tuple( "bob", 1 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, STEEM_SYMBOL ).amount.value == alice_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, alice_symbol ).amount.value == bob_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, STEEM_SYMBOL ).amount.value == bob_balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test filling limit order with better order when partial order is better." );

      op.owner = "alice";
      op.orderid = 4;
      op.amount_to_sell = ASSET( "10.000 TESTS" );
      op.exchange_rate = price( ASSET( "1.000 TESTS" ), asset( 1100, alice_symbol ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      op.owner = "bob";
      op.orderid = 4;
      op.amount_to_sell = asset( 12000, alice_symbol );
      op.exchange_rate = price( asset( 1200, alice_symbol ), ASSET( "1.000 TESTS" ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      alice_balance -= ASSET( "10.000 TESTS" );
      alice_smt_balance += asset( 11000, alice_symbol );
      bob_smt_balance -= asset( 12000, alice_symbol );
      bob_balance += ASSET( "10.000 TESTS" );

      limit_order = limit_order_idx.find( boost::make_tuple( "bob", 4 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order_idx.find(boost::make_tuple( "alice", 4 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == "bob" );
      BOOST_REQUIRE( limit_order->orderid == 4 );
      BOOST_REQUIRE( limit_order->for_sale.value == 1000 );
      BOOST_REQUIRE( limit_order->sell_price == op.exchange_rate );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( alice_symbol, STEEM_SYMBOL ) );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, STEEM_SYMBOL ).amount.value == alice_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, alice_symbol ).amount.value == bob_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, STEEM_SYMBOL ).amount.value == bob_balance.amount.value );
      validate_database();

      limit_order_cancel_operation can;
      can.owner = "bob";
      can.orderid = 4;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( can );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Test filling limit order with better order when partial order is worse." );

      //auto gpo = db->get_dynamic_global_properties();
      //auto start_sbd = gpo.current_sbd_supply;

      op.owner = "alice";
      op.orderid = 5;
      op.amount_to_sell = ASSET( "20.000 TESTS" );
      op.exchange_rate = price( ASSET( "1.000 TESTS" ), asset( 1100, alice_symbol ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      op.owner = "bob";
      op.orderid = 5;
      op.amount_to_sell = asset( 12000, alice_symbol );
      op.exchange_rate = price( asset( 1200, alice_symbol ), ASSET( "1.000 TESTS" ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      alice_balance -= ASSET( "20.000 TESTS" );
      alice_smt_balance += asset( 12000, alice_symbol );

      bob_smt_balance -= asset( 11000, alice_symbol );
      bob_balance += ASSET( "10.909 TESTS" );

      limit_order = limit_order_idx.find( boost::make_tuple( "alice", 5 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order_idx.find(boost::make_tuple( "bob", 5 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == "alice" );
      BOOST_REQUIRE( limit_order->orderid == 5 );
      BOOST_REQUIRE( limit_order->for_sale.value == 9091 );
      BOOST_REQUIRE( limit_order->sell_price == price( ASSET( "1.000 TESTS" ), asset( 1100, alice_symbol ) ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( alice_symbol, STEEM_SYMBOL ) );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, STEEM_SYMBOL ).amount.value == alice_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, alice_symbol ).amount.value == bob_smt_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( bob_account, STEEM_SYMBOL ).amount.value == bob_balance.amount.value );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( claim_reward_balance2_validate )
{
   try
   {
      claim_reward_balance2_operation op;
      op.account = "alice";

      ACTORS( (alice) )

      generate_block();

      // Create SMT(s) and continue.
      auto smts = create_smt_3("alice", alice_private_key);
      const auto& smt1 = smts[0];
      const auto& smt2 = smts[1];
      const auto& smt3 = smts[2];

      BOOST_TEST_MESSAGE( "Testing empty rewards" );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "Testing ineffective rewards" );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      // Manually inserted.
      op.reward_tokens.push_back( ASSET( "0.000 TESTS" ) );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.reward_tokens.clear();
      op.reward_tokens.push_back( ASSET( "0.000 TBD" ) );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.reward_tokens.clear();
      op.reward_tokens.push_back( ASSET( "0.000000 VESTS" ) );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.reward_tokens.clear();
      op.reward_tokens.push_back( asset( 0, smt1 ) );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.reward_tokens.clear();
      op.reward_tokens.push_back( asset( 0, smt2 ) );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.reward_tokens.clear();
      op.reward_tokens.push_back( asset( 0, smt3 ) );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.reward_tokens.clear();

      BOOST_TEST_MESSAGE( "Testing single reward claims" );
      op.reward_tokens.push_back( ASSET( "1.000 TESTS" ) );
      op.validate();
      op.reward_tokens.clear();

      op.reward_tokens.push_back( ASSET( "1.000 TBD" ) );
      op.validate();
      op.reward_tokens.clear();

      op.reward_tokens.push_back( ASSET( "1.000000 VESTS" ) );
      op.validate();
      op.reward_tokens.clear();

      op.reward_tokens.push_back( asset( 1, smt1 ) );
      op.validate();
      op.reward_tokens.clear();

      op.reward_tokens.push_back( asset( 1, smt2 ) );
      op.validate();
      op.reward_tokens.clear();

      op.reward_tokens.push_back( asset( 1, smt3 ) );
      op.validate();
      op.reward_tokens.clear();

      BOOST_TEST_MESSAGE( "Testing multiple rewards" );
      op.reward_tokens.push_back( ASSET( "1.000 TBD" ) );
      op.reward_tokens.push_back( ASSET( "1.000 TESTS" ) );
      op.reward_tokens.push_back( ASSET( "1.000000 VESTS" ) );
      op.reward_tokens.push_back( asset( 1, smt1 ) );
      op.reward_tokens.push_back( asset( 1, smt2 ) );
      op.reward_tokens.push_back( asset( 1, smt3 ) );
      op.validate();
      op.reward_tokens.clear();

      BOOST_TEST_MESSAGE( "Testing invalid rewards" );
      op.reward_tokens.push_back( ASSET( "-1.000 TESTS" ) );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.reward_tokens.clear();
      op.reward_tokens.push_back( ASSET( "-1.000 TBD" ) );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.reward_tokens.clear();
      op.reward_tokens.push_back( ASSET( "-1.000000 VESTS" ) );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.reward_tokens.clear();
      op.reward_tokens.push_back( asset( -1, smt1 ) );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.reward_tokens.clear();
      op.reward_tokens.push_back( asset( -1, smt2 ) );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.reward_tokens.clear();
      op.reward_tokens.push_back( asset( -1, smt3 ) );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.reward_tokens.clear();

      BOOST_TEST_MESSAGE( "Testing duplicated reward tokens." );
      op.reward_tokens.push_back( asset( 1, smt3 ) );
      op.reward_tokens.push_back( asset( 1, smt3 ) );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.reward_tokens.clear();

      BOOST_TEST_MESSAGE( "Testing inconsistencies of manually inserted reward tokens." );
      op.reward_tokens.push_back( ASSET( "1.000 TESTS" ) );
      op.reward_tokens.push_back( ASSET( "1.000 TBD" ) );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.reward_tokens.push_back( asset( 1, smt3 ) );
      op.reward_tokens.push_back( asset( 1, smt1 ) );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.reward_tokens.clear();
      op.reward_tokens.push_back( asset( 1, smt1 ) );
      op.reward_tokens.push_back( asset( -1, smt3 ) );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( claim_reward_balance2_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: decline_voting_rights_authorities" );

      claim_reward_balance2_operation op;
      op.account = "alice";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "alice" );
      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( claim_reward_balance2_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: claim_reward_balance2_apply" );
      BOOST_TEST_MESSAGE( "--- Setting up test state" );

      ACTORS( (alice) )
      generate_block();

      auto smts = create_smt_3( "alice", alice_private_key );
      const auto& smt1 = smts[0];
      const auto& smt2 = smts[1];
      const auto& smt3 = smts[2];

      FUND_SMT_REWARDS( "alice", asset( 10*std::pow(10, smt1.decimals()), smt1 ) );
      FUND_SMT_REWARDS( "alice", asset( 10*std::pow(10, smt2.decimals()), smt2 ) );
      FUND_SMT_REWARDS( "alice", asset( 10*std::pow(10, smt3.decimals()), smt3 ) );

      db_plugin->debug_update( []( database& db )
      {
         db.modify( db.get_account( "alice" ), []( account_object& a )
         {
            a.reward_sbd_balance = ASSET( "10.000 TBD" );
            a.reward_steem_balance = ASSET( "10.000 TESTS" );
            a.reward_vesting_balance = ASSET( "10.000000 VESTS" );
            a.reward_vesting_steem = ASSET( "10.000 TESTS" );
         });

         db.modify( db.get_dynamic_global_properties(), []( dynamic_global_property_object& gpo )
         {
            gpo.current_sbd_supply += ASSET( "10.000 TBD" );
            gpo.current_supply += ASSET( "20.000 TESTS" );
            gpo.virtual_supply += ASSET( "20.000 TESTS" );
            gpo.pending_rewarded_vesting_shares += ASSET( "10.000000 VESTS" );
            gpo.pending_rewarded_vesting_steem += ASSET( "10.000 TESTS" );
         });
      });

      generate_block();
      validate_database();

      auto alice_steem = db->get_account( "alice" ).balance;
      auto alice_sbd = db->get_account( "alice" ).sbd_balance;
      auto alice_vests = db->get_account( "alice" ).vesting_shares;
      auto alice_smt1 = db->get_balance( "alice", smt1 );
      auto alice_smt2 = db->get_balance( "alice", smt2 );
      auto alice_smt3 = db->get_balance( "alice", smt3 );

      claim_reward_balance2_operation op;
      op.account = "alice";

      BOOST_TEST_MESSAGE( "--- Attempting to claim more than exists in the reward balance." );
      // Legacy symbols
      op.reward_tokens.push_back( ASSET( "0.000 TBD" ) );
      op.reward_tokens.push_back( ASSET( "20.000 TESTS" ) );
      op.reward_tokens.push_back( ASSET( "0.000000 VESTS" ) );
      FAIL_WITH_OP(op, alice_private_key, fc::assert_exception);
      op.reward_tokens.clear();
      // SMTs
      op.reward_tokens.push_back( asset( 0, smt1 ) );
      op.reward_tokens.push_back( asset( 0, smt2 ) );
      op.reward_tokens.push_back( asset( 20*std::pow(10, smt3.decimals()), smt3 ) );
      FAIL_WITH_OP(op, alice_private_key, fc::assert_exception);
      op.reward_tokens.clear();

      BOOST_TEST_MESSAGE( "--- Claiming a partial reward balance" );
      // Legacy symbols
      asset partial_vests = ASSET( "5.000000 VESTS" );
      op.reward_tokens.push_back( ASSET( "0.000 TBD" ) );
      op.reward_tokens.push_back( ASSET( "0.000 TESTS" ) );
      op.reward_tokens.push_back( partial_vests );
      PUSH_OP(op, alice_private_key);
      BOOST_REQUIRE( db->get_account( "alice" ).balance == alice_steem + ASSET( "0.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).reward_steem_balance == ASSET( "10.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).sbd_balance == alice_sbd + ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).reward_sbd_balance == ASSET( "10.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).vesting_shares == alice_vests + partial_vests );
      BOOST_REQUIRE( db->get_account( "alice" ).reward_vesting_balance == ASSET( "5.000000 VESTS" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).reward_vesting_steem == ASSET( "5.000 TESTS" ) );
      validate_database();
      alice_vests += partial_vests;
      op.reward_tokens.clear();
      // SMTs
      asset partial_smt2 = asset( 5*std::pow(10, smt2.decimals()), smt2 );
      op.reward_tokens.push_back( asset( 0, smt1 ) );
      op.reward_tokens.push_back( partial_smt2 );
      op.reward_tokens.push_back( asset( 0, smt3 ) );
      PUSH_OP(op, alice_private_key);
      BOOST_REQUIRE( db->get_balance( "alice", smt1 ) == alice_smt1 + asset( 0, smt1 ) );
      BOOST_REQUIRE( db->get_balance( "alice", smt2 ) == alice_smt2 + partial_smt2 );
      BOOST_REQUIRE( db->get_balance( "alice", smt3 ) == alice_smt3 + asset( 0, smt3 ) );
      validate_database();
      alice_smt2 += partial_smt2;
      op.reward_tokens.clear();

      BOOST_TEST_MESSAGE( "--- Claiming the full reward balance" );
      // Legacy symbols
      asset full_steem = ASSET( "10.000 TESTS" );
      asset full_sbd = ASSET( "10.000 TBD" );
      op.reward_tokens.push_back( full_sbd );
      op.reward_tokens.push_back( full_steem );
      op.reward_tokens.push_back( partial_vests );
      PUSH_OP(op, alice_private_key);
      BOOST_REQUIRE( db->get_account( "alice" ).balance == alice_steem + full_steem );
      BOOST_REQUIRE( db->get_account( "alice" ).reward_steem_balance == ASSET( "0.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).sbd_balance == alice_sbd + full_sbd );
      BOOST_REQUIRE( db->get_account( "alice" ).reward_sbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).vesting_shares == alice_vests + partial_vests );
      BOOST_REQUIRE( db->get_account( "alice" ).reward_vesting_balance == ASSET( "0.000000 VESTS" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).reward_vesting_steem == ASSET( "0.000 TESTS" ) );
      validate_database();
      op.reward_tokens.clear();
      // SMTs
      asset full_smt1 = asset( 10*std::pow(10, smt1.decimals()), smt1 );
      asset full_smt3 = asset( 10*std::pow(10, smt3.decimals()), smt3 );
      op.reward_tokens.push_back( full_smt1 );
      op.reward_tokens.push_back( partial_smt2 );
      op.reward_tokens.push_back( full_smt3 );
      PUSH_OP(op, alice_private_key);
      BOOST_REQUIRE( db->get_balance( "alice", smt1 ) == alice_smt1 + full_smt1 );
      BOOST_REQUIRE( db->get_balance( "alice", smt2 ) == alice_smt2 + partial_smt2 );
      BOOST_REQUIRE( db->get_balance( "alice", smt3 ) == alice_smt3 + full_smt3 );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_transfer_to_vesting_validate )
{
   BOOST_TEST_MESSAGE( "Testing: transfer_to_vesting_validate with SMT" );

   ACTORS( (alice) )
   generate_block();

   auto token = create_smt( "alice", alice_private_key, 3 );

   FUND( "alice", asset( 100, token ) );

   transfer_to_vesting_operation op;
   op.from = "alice";
   op.amount = asset( 20, token );
   op.validate();

   BOOST_TEST_MESSAGE( " -- Fail on invalid 'from' account name" );
   op.from = "@@@@@";
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.from = "alice";

   BOOST_TEST_MESSAGE( " -- Fail on invalid 'to' account name" );
   op.to = "@@@@@";
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.to = "";

   BOOST_TEST_MESSAGE( " -- Fail on vesting symbol (instead of liquid)" );
   op.amount = asset( 20, token.get_paired_symbol() );
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.amount = asset( 20, token );

   BOOST_TEST_MESSAGE( " -- Fail on 0 amount" );
   op.amount = asset( 0, token );
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.amount = asset( 20, token );

   BOOST_TEST_MESSAGE( " -- Fail on negative amount" );
   op.amount = asset( -20, token );
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.amount = asset( 20, token );

   op.validate();
}

BOOST_AUTO_TEST_CASE( smt_transfer_to_vesting_apply )
{
   BOOST_TEST_MESSAGE( "Testing: smt_transfer_to_vesting_apply" );

   ACTORS( (alice)(bob) )
   generate_block();

   auto symbol = create_smt( "alice", alice_private_key, 3 );

   FUND( "alice", asset( 10000, symbol ) );

   BOOST_TEST_MESSAGE( " -- Checking initial balances" );
   BOOST_REQUIRE( db->get_balance( "alice", symbol ).amount == 10000 );
   BOOST_REQUIRE( db->get_balance( "alice", symbol.get_paired_symbol() ).amount == 0 );

   const auto& token = db->get< smt_token_object, by_symbol >( symbol );

   auto smt_shares      = asset( token.total_vesting_shares, symbol.get_paired_symbol() );
   auto smt_vests       = asset( token.total_vesting_fund_smt, symbol );
   auto smt_share_price = token.get_vesting_share_price();

   BOOST_TEST_MESSAGE( " -- Self transfer to vesting (alice)" );
   transfer_to_vesting_operation op;
   op.from = "alice";
   op.to = "";
   op.amount = asset( 7500, symbol );
   PUSH_OP( op, alice_private_key );

   auto new_vest = op.amount * smt_share_price;
   smt_shares += new_vest;
   smt_vests += op.amount;
   auto alice_smt_shares = new_vest;

   BOOST_TEST_MESSAGE( " -- Checking balances" );
   BOOST_REQUIRE( db->get_balance( "alice", symbol ) == asset( 2500, symbol ) );
   BOOST_REQUIRE( db->get_balance( "alice", symbol.get_paired_symbol() ) == alice_smt_shares );
   BOOST_REQUIRE( token.total_vesting_fund_smt.value == smt_vests.amount.value );
   BOOST_REQUIRE( token.total_vesting_shares.value == smt_shares.amount.value );

   validate_database();

   smt_share_price = token.get_vesting_share_price();

   BOOST_TEST_MESSAGE( " -- Transfer from alice to bob ( liquid -> vests)" );
   op.to = "bob";
   op.amount = asset( 2000, symbol );
   PUSH_OP( op, alice_private_key );

   new_vest = op.amount * smt_share_price;
   smt_shares += new_vest;
   smt_vests += op.amount;
   auto bob_smt_shares = new_vest;

   BOOST_TEST_MESSAGE( " -- Checking balances" );
   BOOST_REQUIRE( db->get_balance( "alice", symbol ) == asset( 500, symbol ) );
   BOOST_REQUIRE( db->get_balance( "alice", symbol.get_paired_symbol() ) == alice_smt_shares );
   BOOST_REQUIRE( db->get_balance( "bob", symbol ) == asset( 0, symbol ) );
   BOOST_REQUIRE( db->get_balance( "bob", symbol.get_paired_symbol() ) == bob_smt_shares );
   BOOST_REQUIRE( token.total_vesting_fund_smt.value == smt_vests.amount.value );
   BOOST_REQUIRE( token.total_vesting_shares.value == smt_shares.amount.value );

   BOOST_TEST_MESSAGE( " -- Transfer from alice to bob ( liquid -> vests ) insufficient funds" );
   op.amount = asset( 501, symbol );
   FAIL_WITH_OP( op, alice_private_key, fc::assert_exception );

   BOOST_TEST_MESSAGE( " -- Checking balances" );
   BOOST_REQUIRE( db->get_balance( "alice", symbol ) == asset( 500, symbol ) );
   BOOST_REQUIRE( db->get_balance( "alice", symbol.get_paired_symbol() ) == alice_smt_shares );
   BOOST_REQUIRE( db->get_balance( "bob", symbol ) == asset( 0, symbol ) );
   BOOST_REQUIRE( db->get_balance( "bob", symbol.get_paired_symbol() ) == bob_smt_shares );
   BOOST_REQUIRE( token.total_vesting_fund_smt.value == smt_vests.amount.value );
   BOOST_REQUIRE( token.total_vesting_shares.value == smt_shares.amount.value );

   validate_database();
}

BOOST_AUTO_TEST_CASE( smt_withdraw_vesting_validate )
{
   BOOST_TEST_MESSAGE( "Testing: withdraw_vesting_validate" );

   ACTORS( (alice) )
   generate_block();

   auto symbol = create_smt( "alice", alice_private_key, 3 );

   withdraw_vesting_operation op;
   op.account = "alice";
   op.vesting_shares = asset( 10, symbol.get_paired_symbol() );
   op.validate();

   BOOST_TEST_MESSAGE( " -- Testing invalid account name" );
   op.account = "@@@@@";
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.account = "alice";

   BOOST_TEST_MESSAGE( " -- Testing withdrawal of non-vest symbol" );
   op.vesting_shares = asset( 10, symbol );
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.vesting_shares = asset( 10, symbol.get_paired_symbol() );

   op.validate();
}

BOOST_AUTO_TEST_CASE( smt_withdraw_vesting_apply )
{
   BOOST_TEST_MESSAGE( "Testing: smt_withdraw_vesting_apply" );

   ACTORS( (alice)(bob)(charlie) )
   generate_block();

   auto symbol = create_smt( "charlie", charlie_private_key, 3 );

   FUND( STEEM_INIT_MINER_NAME, asset( 10000, symbol ) );
   vest( STEEM_INIT_MINER_NAME, "alice", asset( 10000, symbol ) );

   BOOST_TEST_MESSAGE( "--- Test failure withdrawing negative SMT VESTS" );

   withdraw_vesting_operation op;
   op.account = "alice";
   op.vesting_shares = asset( -1, symbol.get_paired_symbol() );

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, alice_private_key );
   STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

   BOOST_TEST_MESSAGE( "--- Test withdraw of existing SMT VESTS" );
   op.vesting_shares = asset( db->get_balance( "alice", symbol.get_paired_symbol() ).amount / 2, symbol.get_paired_symbol() );

   auto old_vesting_shares = db->get_balance( "alice", symbol.get_paired_symbol() );

   tx.clear();
   tx.operations.push_back( op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, alice_private_key );
   db->push_transaction( tx, 0 );

   auto key = boost::make_tuple( "alice", symbol );
   const auto* balance_obj = db->find< account_regular_balance_object, by_owner_liquid_symbol >( key );

   BOOST_REQUIRE( balance_obj != nullptr );
   BOOST_REQUIRE( db->get_balance( "alice", symbol.get_paired_symbol() ).amount.value == old_vesting_shares.amount.value );
   BOOST_REQUIRE( balance_obj->vesting_withdraw_rate.amount.value == ( old_vesting_shares.amount / ( STEEM_VESTING_WITHDRAW_INTERVALS * 2 ) ).value + 1 );
   BOOST_REQUIRE( balance_obj->to_withdraw.value == op.vesting_shares.amount.value );
   BOOST_REQUIRE( balance_obj->next_vesting_withdrawal == db->head_block_time() + SMT_VESTING_WITHDRAW_INTERVAL_SECONDS );
   validate_database();

   BOOST_TEST_MESSAGE( "--- Test changing vesting withdrawal" );
   tx.operations.clear();
   tx.signatures.clear();

   op.vesting_shares = asset( balance_obj->vesting_shares.amount / 3, symbol.get_paired_symbol() );
   tx.operations.push_back( op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, alice_private_key );
   db->push_transaction( tx, 0 );

   BOOST_REQUIRE( balance_obj->vesting_shares.amount.value == old_vesting_shares.amount.value );
   BOOST_REQUIRE( balance_obj->vesting_withdraw_rate.amount.value == ( old_vesting_shares.amount / ( STEEM_VESTING_WITHDRAW_INTERVALS * 3 ) ).value + 1 );
   BOOST_REQUIRE( balance_obj->to_withdraw.value == op.vesting_shares.amount.value );
   BOOST_REQUIRE( balance_obj->next_vesting_withdrawal == db->head_block_time() + SMT_VESTING_WITHDRAW_INTERVAL_SECONDS );
   validate_database();

   BOOST_TEST_MESSAGE( "--- Test withdrawing more vests than available" );
   tx.operations.clear();
   tx.signatures.clear();

   op.vesting_shares = asset( balance_obj->vesting_shares.amount * 2, symbol.get_paired_symbol() );
   tx.operations.push_back( op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, alice_private_key );
   STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

   BOOST_REQUIRE( balance_obj->vesting_shares.amount.value == old_vesting_shares.amount.value );
   BOOST_REQUIRE( balance_obj->vesting_withdraw_rate.amount.value == ( old_vesting_shares.amount / ( STEEM_VESTING_WITHDRAW_INTERVALS * 3 ) ).value + 1 );
   BOOST_REQUIRE( balance_obj->next_vesting_withdrawal == db->head_block_time() + SMT_VESTING_WITHDRAW_INTERVAL_SECONDS );
   validate_database();

   BOOST_TEST_MESSAGE( "--- Test withdrawing 0 to reset vesting withdraw" );
   tx.operations.clear();
   tx.signatures.clear();

   op.vesting_shares = asset( 0, symbol.get_paired_symbol() );
   tx.operations.push_back( op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, alice_private_key );
   db->push_transaction( tx, 0 );

   BOOST_REQUIRE( balance_obj->vesting_shares.amount.value == old_vesting_shares.amount.value );
   BOOST_REQUIRE( balance_obj->vesting_withdraw_rate.amount.value == 0 );
   BOOST_REQUIRE( balance_obj->to_withdraw.value == 0 );
   BOOST_REQUIRE( balance_obj->next_vesting_withdrawal == fc::time_point_sec::maximum() );
   validate_database();

   BOOST_TEST_MESSAGE( "--- Test withdrawing with no balance" );
   op.account = "bob";
   op.vesting_shares = db->get_balance( "bob", symbol.get_paired_symbol() );
   tx.clear();
   tx.operations.push_back( op );
   sign( tx, bob_private_key );
   STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
}

BOOST_AUTO_TEST_CASE( smt_create_validate )
{
   try
   {
      ACTORS( (alice) );

      BOOST_TEST_MESSAGE( " -- A valid smt_create_operation" );
      smt_create_operation op;
      op.control_account = "alice";
      op.smt_creation_fee = db->get_dynamic_global_properties().smt_creation_fee;
      op.symbol = get_new_smt_symbol( 3, db );
      op.precision = op.symbol.decimals();
      op.validate();

      BOOST_TEST_MESSAGE( " -- Test invalid control account name" );
      op.control_account = "@@@@@";
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.control_account = "alice";

      // Test invalid creation fees.
      BOOST_TEST_MESSAGE( " -- Invalid negative creation fee" );
      op.smt_creation_fee.amount = -op.smt_creation_fee.amount;
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( " -- Valid maximum SMT creation fee (STEEM_MAX_SHARE_SUPPLY)" );
      op.smt_creation_fee.amount = STEEM_MAX_SHARE_SUPPLY;
      op.validate();

      BOOST_TEST_MESSAGE( " -- Invalid SMT creation fee (MAX_SHARE_SUPPLY + 1)" );
      op.smt_creation_fee.amount++;
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( " -- Invalid currency for SMT creation fee (VESTS)" );
      op.smt_creation_fee = ASSET( "1.000000 VESTS" );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.smt_creation_fee = db->get_dynamic_global_properties().smt_creation_fee;

      BOOST_TEST_MESSAGE( " -- Invalid SMT creation fee: differing decimals" );
      op.precision = 0;
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.precision = op.symbol.decimals();

      // Test symbol
      BOOST_TEST_MESSAGE( " -- Invalid SMT creation symbol: vesting symbol used instead of liquid one" );
      op.symbol = op.symbol.get_paired_symbol();
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( " -- Invalid SMT creation symbol: STEEM cannot be an SMT" );
      op.symbol = STEEM_SYMBOL;
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( " -- Invalid SMT creation symbol: SBD cannot be an SMT" );
      op.symbol = SBD_SYMBOL;
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( " -- Invalid SMT creation symbol: VESTS cannot be an SMT" );
      op.symbol = VESTS_SYMBOL;
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      // If this fails, it could indicate a test above has failed for the wrong reasons
      op.symbol = get_new_smt_symbol( 3, db );
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_create_authorities )
{
   try
   {
      SMT_SYMBOL( alice, 3, db );

      smt_create_operation op;
      op.control_account = "alice";
      op.symbol = alice_symbol;
      op.smt_creation_fee = db->get_dynamic_global_properties().smt_creation_fee;

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "alice" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_create_duplicate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_create_duplicate" );

      ACTORS( (alice) )
      asset_symbol_type alice_symbol = create_smt( "alice", alice_private_key, 0 );

      // We add the NAI back to the pool to ensure the test does not fail because the NAI is not in the pool
      db->modify( db->get< nai_pool_object >(), [&] ( nai_pool_object& obj )
      {
         obj.nais[ 0 ] = alice_symbol;
      } );

      // Fail on duplicate SMT lookup
      STEEM_REQUIRE_THROW( create_smt_with_nai( "alice", alice_private_key, alice_symbol.to_nai(), alice_symbol.decimals() ), fc::assert_exception)
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( smt_create_duplicate_differing_decimals )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_create_duplicate_differing_decimals" );

      ACTORS( (alice) )
      asset_symbol_type alice_symbol = create_smt( "alice", alice_private_key, 3 /* Decimals */ );

      // We add the NAI back to the pool to ensure the test does not fail because the NAI is not in the pool
      db->modify( db->get< nai_pool_object >(), [&] ( nai_pool_object& obj )
      {
         obj.nais[ 0 ] = asset_symbol_type::from_nai( alice_symbol.to_nai(), 0 );
      } );

      // Fail on duplicate SMT lookup
      STEEM_REQUIRE_THROW( create_smt_with_nai( "alice", alice_private_key, alice_symbol.to_nai(), 2 /* Decimals */ ), fc::assert_exception)
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( smt_create_duplicate_different_users )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_create_duplicate_different_users" );

      ACTORS( (alice)(bob) )
      asset_symbol_type alice_symbol = create_smt( "alice", alice_private_key, 0 );

      // We add the NAI back to the pool to ensure the test does not fail because the NAI is not in the pool
      db->modify( db->get< nai_pool_object >(), [&] ( nai_pool_object& obj )
      {
         obj.nais[ 0 ] = alice_symbol;
      } );

      // Fail on duplicate SMT lookup
      STEEM_REQUIRE_THROW( create_smt_with_nai( "bob", bob_private_key, alice_symbol.to_nai(), alice_symbol.decimals() ), fc::assert_exception)
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( smt_create_with_steem_funds )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_create_with_steem_funds" );

      // This test expects 1.000 TBD smt_creation_fee
      db->modify( db->get_dynamic_global_properties(), [&] ( dynamic_global_property_object& dgpo )
      {
         dgpo.smt_creation_fee = asset( 1000, SBD_SYMBOL );
      } );

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );

      ACTORS( (alice) )

      generate_block();

      FUND( "alice", ASSET( "0.999 TESTS" ) );

      smt_create_operation op;
      op.control_account = "alice";
      op.smt_creation_fee = ASSET( "1.000 TESTS" );
      op.symbol = get_new_smt_symbol( 3, db );
      op.precision = op.symbol.decimals();
      op.validate();

      // Fail insufficient funds
      FAIL_WITH_OP( op, alice_private_key, fc::assert_exception );

      BOOST_REQUIRE( util::smt::find_token( *db, op.symbol, true ) == nullptr );

      FUND( "alice", ASSET( "0.001 TESTS" ) );

      PUSH_OP( op, alice_private_key );

      BOOST_REQUIRE( util::smt::find_token( *db, op.symbol, true ) != nullptr );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( smt_create_with_sbd_funds )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_create_with_sbd_funds" );

      // This test expects 1.000 TBD smt_creation_fee
      db->modify( db->get_dynamic_global_properties(), [&] ( dynamic_global_property_object& dgpo )
      {
         dgpo.smt_creation_fee = asset( 1000, SBD_SYMBOL );
      } );

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );

      ACTORS( (alice) )

      generate_block();

      FUND( "alice", ASSET( "0.999 TBD" ) );

      smt_create_operation op;
      op.control_account = "alice";
      op.smt_creation_fee = ASSET( "1.000 TBD" );
      op.symbol = get_new_smt_symbol( 3, db );
      op.precision = op.symbol.decimals();
      op.validate();

      // Fail insufficient funds
      FAIL_WITH_OP( op, alice_private_key, fc::assert_exception );

      BOOST_REQUIRE( util::smt::find_token( *db, op.symbol, true ) == nullptr );

      FUND( "alice", ASSET( "0.001 TBD" ) );

      PUSH_OP( op, alice_private_key );

      BOOST_REQUIRE( util::smt::find_token( *db, op.symbol, true ) != nullptr );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( smt_create_with_invalid_nai )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_create_with_invalid_nai" );

      ACTORS( (alice) )

      uint32_t seed = 0;
      asset_symbol_type ast;
      uint32_t collisions = 0;
      do
      {
         BOOST_REQUIRE( collisions < SMT_MAX_NAI_GENERATION_TRIES );
         collisions++;

         ast = util::nai_generator::generate( seed++ );
      }
      while ( db->get< nai_pool_object >().contains( ast ) || util::smt::find_token( *db, ast, true ) != nullptr );

      // Fail on NAI pool not containing this NAI
      STEEM_REQUIRE_THROW( create_smt_with_nai( "alice", alice_private_key, ast.to_nai(), ast.decimals() ), fc::assert_exception)
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( smt_creation_fee_test )
{
   try
   {
      ACTORS( (alice) );
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "2.000 TESTS" ) ) );

      // This ensures that our actual smt_creation_fee is sane in production (either STEEM or SBD)
      const dynamic_global_property_object& dgpo = db->get_dynamic_global_properties();
      FC_ASSERT( dgpo.smt_creation_fee.symbol == STEEM_SYMBOL || dgpo.smt_creation_fee.symbol == SBD_SYMBOL,
                "Unexpected symbol for the SMT creation fee on the dynamic global properties object: ${s}", ("s", dgpo.smt_creation_fee.symbol) );

      FC_ASSERT( dgpo.smt_creation_fee.amount > 0, "Expected positive smt_creation_fee." );

      for ( int i = 0; i < 2; i++ )
      {
         FUND( "alice", ASSET( "2.000 TESTS" ) );
         FUND( "alice", ASSET( "1.000 TBD" ) );

         // These values should be equivilant as per our price feed and all tests here should work either way
         if ( !i ) // First pass
            db->modify( dgpo, [&] ( dynamic_global_property_object& dgpo )
            {
               dgpo.smt_creation_fee = asset( 2000, STEEM_SYMBOL );
            } );
         else // Second pass
            db->modify( dgpo, [&] ( dynamic_global_property_object& dgpo )
            {
               dgpo.smt_creation_fee = asset( 1000, SBD_SYMBOL );
            } );

         BOOST_TEST_MESSAGE( " -- Invalid creation fee, 0.001 TESTS short" );
         smt_create_operation fail_op;
         fail_op.control_account = "alice";
         fail_op.smt_creation_fee = ASSET( "1.999 TESTS" );
         fail_op.symbol = get_new_smt_symbol( 3, db );
         fail_op.precision = fail_op.symbol.decimals();
         fail_op.validate();

         // Fail because we are 0.001 TESTS short of the fee
         FAIL_WITH_OP( fail_op, alice_private_key, fc::assert_exception );

         BOOST_TEST_MESSAGE( " -- Invalid creation fee, 0.001 TBD short" );
         smt_create_operation fail_op2;
         fail_op2.control_account = "alice";
         fail_op2.smt_creation_fee = ASSET( "0.999 TBD" );
         fail_op2.symbol = get_new_smt_symbol( 3, db );
         fail_op2.precision = fail_op2.symbol.decimals();
         fail_op2.validate();

         // Fail because we are 0.001 TBD short of the fee
         FAIL_WITH_OP( fail_op2, alice_private_key, fc::assert_exception );

         BOOST_TEST_MESSAGE( " -- Valid creation fee, using STEEM" );
         // We should be able to pay with STEEM
         smt_create_operation op;
         op.control_account = "alice";
         op.smt_creation_fee = ASSET( "2.000 TESTS" );
         op.symbol = get_new_smt_symbol( 3, db );
         op.precision = op.symbol.decimals();
         op.validate();

         // Succeed because we have paid the equivilant of 1 TBD or 2 TESTS
         PUSH_OP( op, alice_private_key );

         BOOST_TEST_MESSAGE( " -- Valid creation fee, using SBD" );
         // We should be able to pay with SBD
         smt_create_operation op2;
         op2.control_account = "alice";
         op2.smt_creation_fee = ASSET( "1.000 TBD" );
         op2.symbol = get_new_smt_symbol( 3, db );
         op2.precision = op.symbol.decimals();
         op2.validate();

         // Succeed because we have paid the equivilant of 1 TBD or 2 TESTS
         PUSH_OP( op2, alice_private_key );
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_create_reset )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing smt_create_operation reset" );

      ACTORS( (alice) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      fund( "alice", ASSET( "100.000 TESTS" ) );

       SMT_SYMBOL( alice, 3, db )

      db_plugin->debug_update( [=]( database& db )
      {
         db.create< smt_token_object >( [&]( smt_token_object& o )
         {
            o.control_account = "alice";
            o.liquid_symbol = alice_symbol;
         });
      });

      generate_block();

      signed_transaction tx;
      smt_setup_emissions_operation op1;
      op1.control_account = "alice";
      op1.symbol = alice_symbol;
      op1.emissions_unit.token_unit[ SMT_DESTINATION_MARKET_MAKER ] = 10;
      op1.schedule_time = db->head_block_time() + fc::days(30);
      op1.interval_seconds = SMT_EMISSION_MIN_INTERVAL_SECONDS;
      op1.interval_count = 1;
      op1.lep_abs_amount = asset( 0, alice_symbol );
      op1.rep_abs_amount = asset( 0, alice_symbol );
      op1.lep_rel_amount_numerator = 1;
      op1.rep_rel_amount_numerator = 0;

      smt_setup_emissions_operation op2;
      op2.control_account = "alice";
      op2.symbol = alice_symbol;
      op2.emissions_unit.token_unit[ SMT_DESTINATION_MARKET_MAKER ] = 10;
      op2.schedule_time = op1.schedule_time + fc::days( 365 );
      op2.interval_seconds = SMT_EMISSION_MIN_INTERVAL_SECONDS;
      op2.interval_count = 10;
      op2.lep_abs_amount = asset( 0, alice_symbol );
      op2.rep_abs_amount = asset( 0, alice_symbol );
      op2.lep_rel_amount_numerator = 1;
      op2.rep_rel_amount_numerator = 0;

      smt_set_runtime_parameters_operation op3;
      smt_param_windows_v1 windows;
      windows.cashout_window_seconds = 86400 * 4;
      windows.reverse_auction_window_seconds = 60 * 5;
      smt_param_vote_regeneration_period_seconds_v1 vote_regen;
      vote_regen.vote_regeneration_period_seconds = 86400 * 6;
      vote_regen.votes_per_regeneration_period = 600;
      smt_param_rewards_v1 rewards;
      rewards.content_constant = uint128_t( uint64_t( 1000000000000ull ) );
      rewards.percent_curation_rewards = 15 * STEEM_1_PERCENT;
      rewards.author_reward_curve = curve_id::quadratic;
      rewards.curation_reward_curve = curve_id::linear;
      smt_param_allow_downvotes downvotes;
      downvotes.value = false;
      op3.runtime_parameters.insert( windows );
      op3.runtime_parameters.insert( vote_regen );
      op3.runtime_parameters.insert( rewards );
      op3.runtime_parameters.insert( downvotes );
      op3.control_account = "alice";
      op3.symbol = alice_symbol;

      smt_set_setup_parameters_operation op4;
      smt_param_allow_voting voting;
      voting.value = false;
      op4.setup_parameters.insert( voting );
      op4.control_account = "alice";
      op4.symbol = alice_symbol;

      tx.operations.push_back( op1 );
      tx.operations.push_back( op2 );
      tx.operations.push_back( op3 );
      tx.operations.push_back( op4 );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Failure when specifying fee" );
      auto alice_prec_4 = asset_symbol_type::from_nai( alice_symbol.to_nai(), 4 );
      smt_create_operation op;
      op.control_account = "alice";
      op.symbol = alice_prec_4;
      op.smt_creation_fee = ASSET( "1.000 TESTS" );
      op.precision = 4;
      tx.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Failure resetting SMT with token emissions" );
      op.smt_creation_fee = ASSET( "0.000 TBD" );
      tx.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Failure deleting token emissions in wrong order" );
      op1.remove = true;
      op2.remove = true;
      tx.clear();
      tx.operations.push_back( op1 );
      tx.operations.push_back( op2 );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Success deleting token emissions" );
      tx.clear();
      tx.operations.push_back( op2 );
      tx.operations.push_back( op1 );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Success resetting SMT" );
      op.smt_creation_fee = ASSET( "0.000 TBD" );
      tx.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      auto token = db->get< smt_token_object, by_symbol >( alice_prec_4 );

      BOOST_REQUIRE( token.liquid_symbol == op.symbol );
      BOOST_REQUIRE( token.control_account == "alice" );
      BOOST_REQUIRE( token.allow_voting == true );
      BOOST_REQUIRE( token.cashout_window_seconds == STEEM_CASHOUT_WINDOW_SECONDS );
      BOOST_REQUIRE( token.reverse_auction_window_seconds == STEEM_REVERSE_AUCTION_WINDOW_SECONDS_HF20 );
      BOOST_REQUIRE( token.vote_regeneration_period_seconds == STEEM_VOTING_MANA_REGENERATION_SECONDS );
      BOOST_REQUIRE( token.votes_per_regeneration_period == SMT_DEFAULT_VOTES_PER_REGEN_PERIOD );
      BOOST_REQUIRE( token.content_constant == STEEM_CONTENT_CONSTANT_HF0 );
      BOOST_REQUIRE( token.percent_curation_rewards == SMT_DEFAULT_PERCENT_CURATION_REWARDS );
      BOOST_REQUIRE( token.author_reward_curve == curve_id::linear );
      BOOST_REQUIRE( token.curation_reward_curve == curve_id::square_root );
      BOOST_REQUIRE( token.allow_downvotes == true );

      const auto& emissions_idx = db->get_index< smt_token_emissions_index, by_id >();
      BOOST_REQUIRE( emissions_idx.begin() == emissions_idx.end() );

      generate_block();

      BOOST_TEST_MESSAGE( "--- Failure resetting a token that has completed setup" );

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get< smt_token_object, by_symbol >( alice_prec_4 ), [&]( smt_token_object& o )
         {
            o.phase = smt_phase::setup_completed;
         });
      });

      tx.set_expiration( db->head_block_time() + STEEM_BLOCK_INTERVAL * 10 );
      tx.signatures.clear();
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_nai_pool_removal )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_nai_pool_removal" );

      ACTORS( (alice) )
      asset_symbol_type alice_symbol = create_smt( "alice", alice_private_key, 0 );

      // Ensure the NAI does not exist in the pool after being registered
      BOOST_REQUIRE( !db->get< nai_pool_object >().contains( alice_symbol ) );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( smt_nai_pool_count )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_nai_pool_count" );
      const auto &npo = db->get< nai_pool_object >();

      // We should begin with a full NAI pool
      BOOST_REQUIRE( npo.num_available_nais == SMT_MAX_NAI_POOL_COUNT );

      ACTORS( (alice) )

      fund( "alice", 10 * 1000 * 1000 );
      this->generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      convert( "alice", ASSET( "10000.000 TESTS" ) );

      // Drain the NAI pool one at a time
      for ( unsigned int i = 1; i <= SMT_MAX_NAI_POOL_COUNT; i++ )
      {
         smt_create_operation op;
         signed_transaction tx;

         op.symbol = get_new_smt_symbol( 0, this->db );
         op.precision = op.symbol.decimals();
         op.smt_creation_fee = db->get_dynamic_global_properties().smt_creation_fee;
         op.control_account = "alice";

         tx.operations.push_back( op );
         tx.set_expiration( this->db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         tx.sign( alice_private_key, this->db->get_chain_id(), fc::ecc::bip_0062 );

         this->db->push_transaction( tx, 0 );

         BOOST_REQUIRE( npo.num_available_nais == SMT_MAX_NAI_POOL_COUNT - i );
         BOOST_REQUIRE( npo.nais[ npo.num_available_nais ] == asset_symbol_type() );
      }

      // At this point, there should be no available NAIs
      STEEM_REQUIRE_THROW( get_new_smt_symbol( 0, this->db ), fc::assert_exception );

      this->generate_block();

      // We should end with a full NAI pool after block generation
      BOOST_REQUIRE( npo.num_available_nais == SMT_MAX_NAI_POOL_COUNT );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( smt_setup_emissions_validate )
{
   try
   {
      ACTORS( (alice)(bob) );
      generate_block();

      asset_symbol_type alice_symbol = create_smt( "alice", alice_private_key, 3 );

      smt_setup_emissions_operation op;
      op.control_account = "alice";
      op.symbol = alice_symbol;
      op.emissions_unit.token_unit[ SMT_DESTINATION_MARKET_MAKER ] = 10;
      op.interval_seconds = SMT_EMISSION_MIN_INTERVAL_SECONDS;
      op.interval_count = 1;

      fc::time_point_sec schedule_time = fc::time_point::now();
      fc::time_point_sec schedule_end_time = schedule_time + fc::seconds( op.interval_seconds * op.interval_count );

      op.schedule_time = schedule_time;
      op.lep_abs_amount = asset( 0, alice_symbol );
      op.rep_abs_amount = asset( 0, alice_symbol );
      op.lep_rel_amount_numerator = 1;
      op.rep_rel_amount_numerator = 0;
      op.lep_time = schedule_time;
      op.rep_time = schedule_end_time;
      op.validate();

      BOOST_TEST_MESSAGE( " -- Invalid token symbol" );
      op.symbol = STEEM_SYMBOL;
      op.lep_abs_amount = asset( 0, STEEM_SYMBOL );
      op.rep_abs_amount = asset( 0, STEEM_SYMBOL );
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );
      op.symbol = alice_symbol;
      op.lep_abs_amount = asset( 0, alice_symbol );
      op.rep_abs_amount = asset( 0, alice_symbol );

      BOOST_TEST_MESSAGE( " -- Mismatching right endpoint token" );
      op.rep_abs_amount = asset( 0, STEEM_SYMBOL );
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );
      op.rep_abs_amount = asset( 0, alice_symbol );

      BOOST_TEST_MESSAGE( " -- Mismatching left endpoint token" );
      op.lep_abs_amount = asset( 0, STEEM_SYMBOL );
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );
      op.lep_abs_amount = asset( 0, alice_symbol );

      BOOST_TEST_MESSAGE( " -- No emissions" );
      op.lep_rel_amount_numerator = 0;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );
      op.lep_rel_amount_numerator = 1;

      BOOST_TEST_MESSAGE( " -- Invalid control account name" );
      op.control_account = "@@@@";
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );
      op.control_account = "alice";

      BOOST_TEST_MESSAGE(" -- Empty emission unit" );
      op.emissions_unit.token_unit.clear();
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( " -- Invalid emission unit token unit account" );
      op.emissions_unit.token_unit[ "@@@@" ] = 10;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );
      op.emissions_unit.token_unit.clear();
      op.emissions_unit.token_unit[ SMT_DESTINATION_REWARDS ] = 1;
      op.emissions_unit.token_unit[ SMT_DESTINATION_MARKET_MAKER ] = 1;
      op.emissions_unit.token_unit[ SMT_DESTINATION_VESTING ] = 1;

      BOOST_TEST_MESSAGE( " -- Invalid schedule time" );
      op.schedule_time = STEEM_GENESIS_TIME;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );
      op.schedule_time = fc::time_point::now();

      BOOST_TEST_MESSAGE( " -- 0 interval count" );
      op.interval_count = 0;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );
      op.interval_count = 1;

      BOOST_TEST_MESSAGE( " -- Interval seconds too low" );
      op.interval_seconds = SMT_EMISSION_MIN_INTERVAL_SECONDS - 1;
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );
      op.interval_seconds = SMT_EMISSION_MIN_INTERVAL_SECONDS;

      BOOST_TEST_MESSAGE( " -- Negative asset left endpoint" );
      op.lep_abs_amount = asset( -1, alice_symbol );
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );
      op.lep_abs_amount = asset( 0 , alice_symbol );

      BOOST_TEST_MESSAGE( " -- Negative asset right endpoint" );
      op.rep_abs_amount = asset( -1 , alice_symbol );
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );
      op.rep_abs_amount = asset( 0 , alice_symbol );

      BOOST_TEST_MESSAGE( " -- Left endpoint time cannot be before schedule time" );
      op.lep_time -= fc::seconds( 1 );
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );
      op.lep_time += fc::seconds( 1 );

      BOOST_TEST_MESSAGE( " -- Right endpoint time cannot be after schedule end time" );
      op.rep_time += fc::seconds( 1 );
      STEEM_REQUIRE_THROW( op.validate(), fc::exception );
      op.rep_time -= fc::seconds( 1 );

      BOOST_TEST_MESSAGE( " -- Left endpoint time and right endpoint time can be anything if they're equal" );
      fc::time_point_sec tp = fc::time_point_sec( 0 );
      op.lep_time = tp;
      op.rep_time = tp;

      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_setup_emissions_authorities )
{
   try
   {
      SMT_SYMBOL( alice, 3, db );

      smt_setup_emissions_operation op;
      op.control_account = "alice";
      fc::time_point now = fc::time_point::now();
      op.schedule_time = now;
      op.emissions_unit.token_unit[ "alice" ] = 10;
      op.lep_abs_amount = op.rep_abs_amount = asset( 1000, alice_symbol );

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "alice" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_setup_emissions_apply )
{
   try
   {
      ACTORS( (alice)(bob) )

      generate_block();

      asset_symbol_type alice_symbol = create_smt( "alice", alice_private_key, 3 );

      fc::time_point_sec emissions1_schedule_time = fc::time_point::now();

      smt_setup_emissions_operation op;
      op.control_account = "alice";
      op.symbol = alice_symbol;
      op.emissions_unit.token_unit[ SMT_DESTINATION_MARKET_MAKER ] = 10;
      op.schedule_time = emissions1_schedule_time;
      op.interval_seconds = SMT_EMISSION_MIN_INTERVAL_SECONDS;
      op.interval_count = 1;
      op.lep_abs_amount = asset( 0, alice_symbol );
      op.rep_abs_amount = asset( 0, alice_symbol );
      op.lep_rel_amount_numerator = 1;
      op.rep_rel_amount_numerator = 0;;
      op.validate();

      BOOST_TEST_MESSAGE( " -- Control account does not own SMT" );
      op.control_account = "bob";
      FAIL_WITH_OP( op, bob_private_key, fc::assert_exception );
      op.control_account = "alice";

      BOOST_TEST_MESSAGE( " -- SMT setup phase has already completed" );
      auto smt_token = util::smt::find_token( *db, alice_symbol );
      db->modify( *smt_token, [&] ( smt_token_object& obj )
      {
         obj.phase = smt_phase::setup_completed;
      } );
      FAIL_WITH_OP( op, alice_private_key, fc::assert_exception );
      db->modify( *smt_token, [&] ( smt_token_object& obj )
      {
         obj.phase = smt_phase::setup;
      } );
      PUSH_OP( op, alice_private_key );

      BOOST_TEST_MESSAGE( " -- Emissions range is overlapping" );
      smt_setup_emissions_operation op2;
      op2.control_account = "alice";
      op2.symbol = alice_symbol;
      op2.emissions_unit.token_unit[ SMT_DESTINATION_MARKET_MAKER ] = 10;
      op2.schedule_time = emissions1_schedule_time + fc::seconds( SMT_EMISSION_MIN_INTERVAL_SECONDS );
      op2.interval_seconds = SMT_EMISSION_MIN_INTERVAL_SECONDS;
      op2.interval_count = 5;
      op2.lep_abs_amount = asset( 1200, alice_symbol );
      op2.rep_abs_amount = asset( 1000, alice_symbol );
      op2.lep_rel_amount_numerator = 1;
      op2.rep_rel_amount_numerator = 2;;
      op2.validate();

      FAIL_WITH_OP( op2, alice_private_key, fc::assert_exception );

      op2.schedule_time += fc::seconds( 1 );
      PUSH_OP( op2, alice_private_key );

      BOOST_TEST_MESSAGE( " -- Emissions must be created in chronological order" );
      smt_setup_emissions_operation op3;
      op3.control_account = "alice";
      op3.symbol = alice_symbol;
      op3.emissions_unit.token_unit[ SMT_DESTINATION_MARKET_MAKER ] = 10;
      op3.schedule_time = emissions1_schedule_time - fc::seconds( SMT_EMISSION_MIN_INTERVAL_SECONDS + 1 );
      op3.interval_seconds = SMT_EMISSION_MIN_INTERVAL_SECONDS;
      op3.interval_count = SMT_EMIT_INDEFINITELY;
      op3.lep_abs_amount = asset( 0, alice_symbol );
      op3.rep_abs_amount = asset( 1000, alice_symbol );
      op3.lep_rel_amount_numerator = 0;
      op3.rep_rel_amount_numerator = 0;;
      op3.validate();
      FAIL_WITH_OP( op3, alice_private_key, fc::assert_exception );

      op3.schedule_time = op2.schedule_time + fc::seconds( uint64_t( op2.interval_seconds ) * uint64_t( op2.interval_count ) );
      FAIL_WITH_OP( op3, alice_private_key, fc::assert_exception );

      op3.schedule_time += fc::seconds( 1 );
      PUSH_OP( op3, alice_private_key );

      BOOST_TEST_MESSAGE( " -- No more emissions permitted" );
      smt_setup_emissions_operation op4;
      op4.control_account = "alice";
      op4.symbol = alice_symbol;
      op4.emissions_unit.token_unit[ SMT_DESTINATION_MARKET_MAKER ] = 10;
      op4.schedule_time = op3.schedule_time + fc::days( 365 );
      op4.interval_seconds = SMT_EMISSION_MIN_INTERVAL_SECONDS;
      op4.interval_count = 10;
      op4.lep_abs_amount = asset( 0, alice_symbol );
      op4.rep_abs_amount = asset( 0, alice_symbol );
      op4.lep_rel_amount_numerator = 1;
      op4.rep_rel_amount_numerator = 0;;
      op4.validate();

      FAIL_WITH_OP( op4, alice_private_key, fc::assert_exception );

      op4.schedule_time = fc::time_point_sec::maximum();
      FAIL_WITH_OP( op4, alice_private_key, fc::assert_exception );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( set_setup_parameters_validate )
{
   try
   {
      ACTORS( (alice) )
      generate_block();

      asset_symbol_type alice_symbol = create_smt( "alice", alice_private_key, 3 );

      smt_set_setup_parameters_operation op;
      op.control_account = "alice";
      op.symbol = alice_symbol;
      op.setup_parameters.emplace( smt_param_allow_voting { .value = true } );

      op.validate();

      op.symbol = STEEM_SYMBOL;
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception ); // Invalid symbol
      op.symbol = VESTS_SYMBOL;
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception ); // Invalid symbol
      op.symbol = SBD_SYMBOL;
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception ); // Invalid symbol
      op.symbol = alice_symbol;

      op.control_account = "####";
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception ); // Invalid account name
      op.control_account = "alice";

      op.setup_parameters.clear();
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception ); // Empty setup parameters
      op.setup_parameters.emplace( smt_param_allow_voting { .value = true } );

      op.validate();

      op.setup_parameters.clear();
      op.setup_parameters.emplace( smt_param_allow_voting { .value = false } );
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( set_setup_parameters_authorities )
{
   try
   {
      smt_set_setup_parameters_operation op;
      op.control_account = "alice";

      flat_set<account_name_type> auths;
      flat_set<account_name_type> expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "alice" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( set_setup_parameters_apply )
{
   try
   {
      ACTORS( (alice)(bob) )

      generate_block();

      FUND( "alice", 5000000 );
      FUND( "bob", 5000000 );

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      convert( "alice", ASSET( "5000.000 TESTS" ) );
      convert( "bob", ASSET( "5000.000 TESTS" ) );

      auto alice_symbol = create_smt( "alice", alice_private_key, 3 );
      auto bob_symbol = create_smt( "bob", bob_private_key, 3 );

      auto alice_token = util::smt::find_token( *db, alice_symbol );
      auto bob_token = util::smt::find_token( *db, bob_symbol );

      BOOST_REQUIRE( alice_token->allow_voting == true );

      smt_set_setup_parameters_operation op;
      op.control_account = "alice";
      op.symbol = alice_symbol;

      BOOST_TEST_MESSAGE( " -- Succeed set voting to false" );
      op.setup_parameters.emplace( smt_param_allow_voting { .value = false } );
      PUSH_OP( op, alice_private_key );

      BOOST_REQUIRE( alice_token->allow_voting == false );

      BOOST_TEST_MESSAGE( " -- Succeed set voting to true" );
      op.setup_parameters.clear();
      op.setup_parameters.emplace( smt_param_allow_voting { .value = true } );
      PUSH_OP( op, alice_private_key );

      BOOST_REQUIRE( alice_token->allow_voting == true );

      BOOST_TEST_MESSAGE( "--- Failure with wrong control account" );
      op.symbol = bob_symbol;

      FAIL_WITH_OP( op, alice_private_key, fc::assert_exception );

      BOOST_TEST_MESSAGE( " -- Succeed set voting to true" );
      op.control_account = "bob";
      op.setup_parameters.clear();
      op.setup_parameters.emplace( smt_param_allow_voting { .value = true } );

      PUSH_OP( op, bob_private_key );
      BOOST_REQUIRE( bob_token->allow_voting == true );


      BOOST_TEST_MESSAGE( " -- Failure with mismatching precision" );
      op.symbol.asset_num++;
      FAIL_WITH_OP( op, bob_private_key, fc::assert_exception );

      BOOST_TEST_MESSAGE( " -- Failure with non-existent asset symbol" );
      op.symbol = this->get_new_smt_symbol( 1, db );
      FAIL_WITH_OP( op, bob_private_key, fc::assert_exception );

      BOOST_TEST_MESSAGE( " -- Succeed set voting to false" );
      op.symbol = bob_symbol;
      op.setup_parameters.clear();
      op.setup_parameters.emplace( smt_param_allow_voting { .value = false } );
      PUSH_OP( op, bob_private_key );
      BOOST_REQUIRE( bob_token->allow_voting == false );


      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_set_runtime_parameters_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_set_runtime_parameters_validate" );

      smt_set_runtime_parameters_operation op;

      auto new_symbol = get_new_smt_symbol( 3, db );

      op.symbol = new_symbol;
      op.control_account = "alice";
      op.runtime_parameters.insert( smt_param_allow_downvotes() );

      // If this fails, it could indicate a test above has failed for the wrong reasons
      op.validate();

      BOOST_TEST_MESSAGE( "--- Test invalid control account name" );
      op.control_account = "@@@@@";
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.control_account = "alice";

      // Test symbol
      BOOST_TEST_MESSAGE( "--- Invalid SMT creation symbol: vesting symbol used instead of liquid one" );
      op.symbol = op.symbol.get_paired_symbol();
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Invalid SMT creation symbol: STEEM cannot be an SMT" );
      op.symbol = STEEM_SYMBOL;
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Invalid SMT creation symbol: SBD cannot be an SMT" );
      op.symbol = SBD_SYMBOL;
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Invalid SMT creation symbol: VESTS cannot be an SMT" );
      op.symbol = VESTS_SYMBOL;
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.symbol = new_symbol;

      BOOST_TEST_MESSAGE( "--- Failure when no parameters are set" );
      op.runtime_parameters.clear();
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      /*
       * Inequality to test:
       *
       * 0 <= reverse_auction_window_seconds + SMT_UPVOTE_LOCKOUT < cashout_window_seconds
       * <= SMT_VESTING_WITHDRAW_INTERVAL_SECONDS
       */

      BOOST_TEST_MESSAGE( "--- Failure when cashout_window_second is equal to SMT_UPVOTE_LOCKOUT" );
      smt_param_windows_v1 windows;
      windows.reverse_auction_window_seconds = 0;
      windows.cashout_window_seconds = SMT_UPVOTE_LOCKOUT;
      op.runtime_parameters.insert( windows );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Success when cashout_window_seconds is above SMT_UPVOTE_LOCKOUT" );
      windows.cashout_window_seconds++;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( windows );
      op.validate();

      BOOST_TEST_MESSAGE( "--- Failure when cashout_window_seconds is equal to reverse_auction_window_seconds + SMT_UPVOTE_LOCKOUT" );
      windows.reverse_auction_window_seconds++;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( windows );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Failure when cashout_window_seconds is greater than SMT_VESTING_WITHDRAW_INTERVAL_SECONDS" );
      windows.cashout_window_seconds = SMT_VESTING_WITHDRAW_INTERVAL_SECONDS + 1;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( windows );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Success when cashout_window_seconds is equal to SMT_VESTING_WITHDRAW_INTERVAL_SECONDS" );
      windows.cashout_window_seconds--;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( windows );
      op.validate();

      BOOST_TEST_MESSAGE( "--- Success when reverse_auction_window_seconds + SMT_UPVOTE_LOCKOUT is one less than cashout_window_seconds" );
      windows.reverse_auction_window_seconds = windows.cashout_window_seconds - SMT_UPVOTE_LOCKOUT - 1;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( windows );
      op.validate();

      BOOST_TEST_MESSAGE( "--- Failure when reverse_auction_window_seconds + SMT_UPVOTE_LOCKOUT is equal to cashout_window_seconds" );
      windows.cashout_window_seconds--;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( windows );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      /*
       * Conditions to test:
       *
       * 0 < vote_regeneration_seconds < SMT_VESTING_WITHDRAW_INTERVAL_SECONDS
       *
       * votes_per_regeneration_period * 86400 / vote_regeneration_period
       * <= SMT_MAX_NOMINAL_VOTES_PER_DAY
       *
       * 0 < votes_per_regeneration_period <= SMT_MAX_VOTES_PER_REGENERATION
       */
      uint32_t practical_regen_seconds_lower_bound = 86400 / SMT_MAX_NOMINAL_VOTES_PER_DAY;

      BOOST_TEST_MESSAGE( "--- Failure when vote_regeneration_period_seconds is 0" );
      smt_param_vote_regeneration_period_seconds_v1 vote_regen;
      vote_regen.vote_regeneration_period_seconds = 0;
      vote_regen.votes_per_regeneration_period = 1;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( vote_regen );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Success when vote_regeneration_period_seconds is greater than 0" );
      // Any value less than 86 will violate the nominal votes per day check. 86 is a practical minimum as a consequence.
      vote_regen.vote_regeneration_period_seconds = practical_regen_seconds_lower_bound + 1;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( vote_regen );
      op.validate();

      BOOST_TEST_MESSAGE( "--- Failure when vote_regeneration_period_seconds is greater SMT_VESTING_WITHDRAW_INTERVAL_SECONDS" );
      vote_regen.vote_regeneration_period_seconds = SMT_VESTING_WITHDRAW_INTERVAL_SECONDS + 1;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( vote_regen );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Success when vote_regeneration_period_seconds is equal to SMT_VESTING_WITHDRAW_INTERVAL_SECONDS" );
      vote_regen.vote_regeneration_period_seconds--;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( vote_regen );
      op.validate();

      BOOST_TEST_MESSAGE( "--- Test various \"nominal votes per day\" scenarios" );
      BOOST_TEST_MESSAGE( "--- Mid Point Checks" );
      vote_regen.vote_regeneration_period_seconds = 86400;
      vote_regen.votes_per_regeneration_period = SMT_MAX_NOMINAL_VOTES_PER_DAY;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( vote_regen );
      op.validate();

      vote_regen.vote_regeneration_period_seconds = 86399;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( vote_regen );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      vote_regen.vote_regeneration_period_seconds = 86400;
      vote_regen.votes_per_regeneration_period = SMT_MAX_NOMINAL_VOTES_PER_DAY + 1;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( vote_regen );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      vote_regen.vote_regeneration_period_seconds = 86401;
      vote_regen.votes_per_regeneration_period = SMT_MAX_NOMINAL_VOTES_PER_DAY;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( vote_regen );
      op.validate();

      vote_regen.vote_regeneration_period_seconds = 86400;
      vote_regen.votes_per_regeneration_period = SMT_MAX_NOMINAL_VOTES_PER_DAY - 1;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( vote_regen );
      op.validate();

      BOOST_TEST_MESSAGE( "--- Lower Bound Checks" );
      vote_regen.vote_regeneration_period_seconds = practical_regen_seconds_lower_bound;
      vote_regen.votes_per_regeneration_period = 1;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( vote_regen );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      vote_regen.vote_regeneration_period_seconds = practical_regen_seconds_lower_bound + 1;
      vote_regen.votes_per_regeneration_period = 2;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( vote_regen );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      vote_regen.vote_regeneration_period_seconds = practical_regen_seconds_lower_bound + 2;
      vote_regen.votes_per_regeneration_period = 1;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( vote_regen );
      op.validate();

      BOOST_TEST_MESSAGE( "--- Upper Bound Checks" );
      vote_regen.vote_regeneration_period_seconds = SMT_VESTING_WITHDRAW_INTERVAL_SECONDS;
      vote_regen.votes_per_regeneration_period = SMT_MAX_VOTES_PER_REGENERATION;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( vote_regen );
      op.validate();

      vote_regen.votes_per_regeneration_period = SMT_MAX_VOTES_PER_REGENERATION + 1;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( vote_regen );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      vote_regen.vote_regeneration_period_seconds = SMT_VESTING_WITHDRAW_INTERVAL_SECONDS - 1;
      vote_regen.votes_per_regeneration_period = SMT_MAX_VOTES_PER_REGENERATION;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( vote_regen );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      vote_regen.vote_regeneration_period_seconds = SMT_VESTING_WITHDRAW_INTERVAL_SECONDS;
      vote_regen.votes_per_regeneration_period = SMT_MAX_VOTES_PER_REGENERATION - 1;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( vote_regen );
      op.validate();

      /*
       * Conditions to test:
       *
       * percent_curation_rewards <= 10000
       *
       * percent_content_rewards + percent_curation_rewards == 10000
       *
       * author_reward_curve must be quadratic or linear
       *
       * curation_reward_curve must be bounded_curation, linear, or square_root
       */
      BOOST_TEST_MESSAGE( "--- Failure when percent_curation_rewards greater than 10000" );
      smt_param_rewards_v1 rewards;
      rewards.content_constant = STEEM_CONTENT_CONSTANT_HF0;
      rewards.percent_curation_rewards = STEEM_100_PERCENT + 1;
      rewards.author_reward_curve = curve_id::linear;
      rewards.curation_reward_curve = curve_id::square_root;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( rewards );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Success when percent_curation_rewards is 10000" );
      rewards.percent_curation_rewards = STEEM_100_PERCENT;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( rewards );
      op.validate();

      BOOST_TEST_MESSAGE( "--- Success when author curve is quadratic" );
      rewards.author_reward_curve = curve_id::quadratic;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( rewards );
      op.validate();

      BOOST_TEST_MESSAGE( "--- Failure when author curve is bounded_curation" );
      rewards.author_reward_curve = curve_id::bounded_curation;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( rewards );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Failure when author curve is square_root" );
      rewards.author_reward_curve = curve_id::square_root;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( rewards );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Success when curation curve is bounded_curation" );
      rewards.author_reward_curve = curve_id::linear;
      rewards.curation_reward_curve = curve_id::bounded_curation;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( rewards );
      op.validate();

      BOOST_TEST_MESSAGE( "--- Success when curation curve is linear" );
      rewards.curation_reward_curve = curve_id::linear;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( rewards );
      op.validate();

      BOOST_TEST_MESSAGE( "--- Failure when curation curve is quadratic" );
      rewards.curation_reward_curve = curve_id::quadratic;
      op.runtime_parameters.clear();
      op.runtime_parameters.insert( rewards );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      // Literally nothing to test for smt_param_allow_downvotes because it can only be true or false.
      // Inclusion success was tested in initial positive validation at the beginning of the test.
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_set_runtime_parameters_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_set_runtime_parameters_authorities" );
      smt_set_runtime_parameters_operation op;
      op.control_account = "alice";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "alice" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_set_runtime_parameters_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_set_runtime_parameters_evaluate" );

      ACTORS( (alice)(bob) )
      SMT_SYMBOL( alice, 3, db );
      SMT_SYMBOL( bob, 3, db );

      generate_block();

      db_plugin->debug_update( [=](database& db)
      {
         db.create< smt_token_object >( [&]( smt_token_object& o )
         {
            o.control_account = "alice";
            o.liquid_symbol = alice_symbol;
         });
      });

      smt_set_runtime_parameters_operation op;
      signed_transaction tx;

      BOOST_TEST_MESSAGE( "--- Failure with wrong control account" );
      op.control_account = "bob";
      op.symbol = alice_symbol;
      op.runtime_parameters.insert( smt_param_allow_downvotes() );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, bob_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Failure with a non-existent asset symbol" );
      op.control_account = "alice";
      op.symbol = bob_symbol;
      tx.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Failure with wrong precision in asset symbol" );
      op.symbol = alice_symbol;
      op.symbol.asset_num++;
      tx.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Success updating runtime parameters" );
      op.runtime_parameters.clear();
      // These are params different than the default
      smt_param_windows_v1 windows;
      windows.cashout_window_seconds = 86400 * 4;
      windows.reverse_auction_window_seconds = 60 * 5;
      smt_param_vote_regeneration_period_seconds_v1 vote_regen;
      vote_regen.vote_regeneration_period_seconds = 86400 * 6;
      vote_regen.votes_per_regeneration_period = 600;
      smt_param_rewards_v1 rewards;
      rewards.content_constant = uint128_t( uint64_t( 1000000000000ull ) );
      rewards.percent_curation_rewards = 15 * STEEM_1_PERCENT;
      rewards.author_reward_curve = curve_id::quadratic;
      rewards.curation_reward_curve = curve_id::linear;
      smt_param_allow_downvotes downvotes;
      downvotes.value = false;
      op.runtime_parameters.insert( windows );
      op.runtime_parameters.insert( vote_regen );
      op.runtime_parameters.insert( rewards );
      op.runtime_parameters.insert( downvotes );
      op.symbol = alice_symbol;
      tx.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      const auto& token = db->get< smt_token_object, by_symbol >( alice_symbol );

      BOOST_REQUIRE( token.cashout_window_seconds == windows.cashout_window_seconds );
      BOOST_REQUIRE( token.reverse_auction_window_seconds == windows.reverse_auction_window_seconds );
      BOOST_REQUIRE( token.vote_regeneration_period_seconds == vote_regen.vote_regeneration_period_seconds );
      BOOST_REQUIRE( token.votes_per_regeneration_period == vote_regen.votes_per_regeneration_period );
      BOOST_REQUIRE( token.content_constant == rewards.content_constant );
      BOOST_REQUIRE( token.percent_curation_rewards == rewards.percent_curation_rewards );
      BOOST_REQUIRE( token.author_reward_curve == rewards.author_reward_curve );
      BOOST_REQUIRE( token.curation_reward_curve == rewards.curation_reward_curve );
      BOOST_REQUIRE( token.allow_downvotes == downvotes.value );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_contribute_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_contribute_validate" );

      auto new_symbol = get_new_smt_symbol( 3, db );

      smt_contribute_operation op;
      op.contributor = "alice";
      op.contribution = asset( 1000, STEEM_SYMBOL );
      op.contribution_id = 1;
      op.symbol = new_symbol;
      op.validate();

      BOOST_TEST_MESSAGE( " -- Failure on invalid account name" );
      op.contributor = "@@@@@";
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.contributor = "alice";

      BOOST_TEST_MESSAGE( " -- Failure on negative contribution" );
      op.contribution = asset( -1, STEEM_SYMBOL );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.contribution = asset( 1000, STEEM_SYMBOL );

      BOOST_TEST_MESSAGE( " -- Failure on no contribution" );
      op.contribution = asset( 0, STEEM_SYMBOL );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.contribution = asset( 1000, STEEM_SYMBOL );

      BOOST_TEST_MESSAGE( " -- Failure on VESTS contribution" );
      op.contribution = asset( 1000, VESTS_SYMBOL );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.contribution = asset( 1000, STEEM_SYMBOL );

      BOOST_TEST_MESSAGE( " -- Failure on SBD contribution" );
      op.contribution = asset( 1000, SBD_SYMBOL );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.contribution = asset( 1000, STEEM_SYMBOL );

      BOOST_TEST_MESSAGE( " -- Failure on SMT contribution" );
      op.contribution = asset( 1000, new_symbol );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.contribution = asset( 1000, STEEM_SYMBOL );

      BOOST_TEST_MESSAGE( " -- Failure on contribution to STEEM" );
      op.symbol = STEEM_SYMBOL;
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.symbol = new_symbol;

      BOOST_TEST_MESSAGE( " -- Failure on contribution to VESTS" );
      op.symbol = VESTS_SYMBOL;
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.symbol = new_symbol;

      BOOST_TEST_MESSAGE( " -- Failure on contribution to SBD" );
      op.symbol = SBD_SYMBOL;
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.symbol = new_symbol;

      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_contribute_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_contribute_evaluate" );

      ACTORS( (alice)(bob)(sam) );
      SMT_SYMBOL( alice, 3, db );

      generate_block();

      auto alice_asset_accumulator = asset( 0, STEEM_SYMBOL );
      auto bob_asset_accumulator = asset( 0, STEEM_SYMBOL );
      auto sam_asset_accumulator = asset( 0, STEEM_SYMBOL );

      auto alice_contribution_counter = 0;
      auto bob_contribution_counter = 0;
      auto sam_contribution_counter = 0;

      FUND( "sam", ASSET( "1000.000 TESTS" ) );

      generate_block();

      db_plugin->debug_update( [=] ( database& db )
      {
         db.create< smt_token_object >( [&]( smt_token_object& o )
         {
            o.control_account = "alice";
            o.liquid_symbol = alice_symbol;
         } );

         db.create< smt_ico_object >( [&]( smt_ico_object& o )
         {
            o.symbol = alice_symbol;
            o.steem_units_soft_cap = SMT_MIN_SOFT_CAP_STEEM_UNITS;
            o.steem_units_hard_cap = 99000;
         } );
      } );

      smt_contribute_operation bob_op;
      bob_op.contributor = "bob";
      bob_op.contribution = asset( 1000, STEEM_SYMBOL );
      bob_op.contribution_id = bob_contribution_counter;
      bob_op.symbol = alice_symbol;

      smt_contribute_operation alice_op;
      alice_op.contributor = "alice";
      alice_op.contribution = asset( 2000, STEEM_SYMBOL );
      alice_op.contribution_id = alice_contribution_counter;
      alice_op.symbol = alice_symbol;

      smt_contribute_operation sam_op;
      sam_op.contributor = "sam";
      sam_op.contribution = asset( 3000, STEEM_SYMBOL );
      sam_op.contribution_id = sam_contribution_counter;
      sam_op.symbol = alice_symbol;

      BOOST_TEST_MESSAGE( " -- Failure on SMT not in contribution phase" );
      FAIL_WITH_OP( bob_op, bob_private_key, fc::assert_exception );

      BOOST_TEST_MESSAGE( " -- Failure on SMT not in contribution phase" );
      FAIL_WITH_OP( alice_op, alice_private_key, fc::assert_exception );

      BOOST_TEST_MESSAGE( " -- Failure on SMT not in contribution phase" );
      FAIL_WITH_OP( sam_op, sam_private_key, fc::assert_exception );

      db_plugin->debug_update( [=] ( database& db )
      {
         const smt_token_object *token = util::smt::find_token( db, alice_symbol );
         db.modify( *token, [&]( smt_token_object& o )
         {
            o.phase = smt_phase::ico;
         } );
      } );

      BOOST_TEST_MESSAGE( " -- Failure on insufficient funds" );
      FAIL_WITH_OP( bob_op, bob_private_key, fc::assert_exception );

      BOOST_TEST_MESSAGE( " -- Failure on insufficient funds" );
      FAIL_WITH_OP( alice_op, alice_private_key, fc::assert_exception );

      FUND( "alice", ASSET( "1000.000 TESTS" ) );
      FUND( "bob",   ASSET( "1000.000 TESTS" ) );

      generate_block();

      BOOST_TEST_MESSAGE( " -- Succeed on sufficient funds" );
      bob_op.contribution_id = bob_contribution_counter++;
      PUSH_OP( bob_op, bob_private_key );
      bob_asset_accumulator += bob_op.contribution;

      BOOST_TEST_MESSAGE( " -- Failure on duplicate contribution ID" );
      FAIL_WITH_OP( bob_op, bob_private_key, fc::assert_exception );

      BOOST_TEST_MESSAGE( " -- Succeed with new contribution ID" );
      bob_op.contribution_id = bob_contribution_counter++;
      PUSH_OP( bob_op, bob_private_key );
      bob_asset_accumulator += bob_op.contribution;

      BOOST_TEST_MESSAGE( " -- Succeed on sufficient funds" );
      alice_op.contribution_id = alice_contribution_counter++;
      PUSH_OP( alice_op, alice_private_key );
      alice_asset_accumulator += alice_op.contribution;

      BOOST_TEST_MESSAGE( " -- Failure on duplicate contribution ID" );
      FAIL_WITH_OP( alice_op, alice_private_key, fc::assert_exception );

      BOOST_TEST_MESSAGE( " -- Succeed with new contribution ID" );
      alice_op.contribution_id = alice_contribution_counter++;
      PUSH_OP( alice_op, alice_private_key );
      alice_asset_accumulator += alice_op.contribution;

      BOOST_TEST_MESSAGE( " -- Succeed with sufficient funds" );
      sam_op.contribution_id = sam_contribution_counter++;
      PUSH_OP( sam_op, sam_private_key );
      sam_asset_accumulator += sam_op.contribution;

      validate_database();

      for ( int i = 0; i < 15; i++ )
      {
         BOOST_TEST_MESSAGE( " -- Successful contribution (alice)" );
         alice_op.contribution_id = alice_contribution_counter++;
         PUSH_OP( alice_op, alice_private_key );
         alice_asset_accumulator += alice_op.contribution;

         BOOST_TEST_MESSAGE( " -- Successful contribution (bob)" );
         bob_op.contribution_id = bob_contribution_counter++;
         PUSH_OP( bob_op, bob_private_key );
         bob_asset_accumulator += bob_op.contribution;

         BOOST_TEST_MESSAGE( " -- Successful contribution (sam)" );
         sam_op.contribution_id = sam_contribution_counter++;
         PUSH_OP( sam_op, sam_private_key );
         sam_asset_accumulator += sam_op.contribution;
      }

      BOOST_TEST_MESSAGE( " -- Fail to contribute after hard cap (alice)" );
      alice_op.contribution_id = alice_contribution_counter + 1;
      FAIL_WITH_OP( alice_op, alice_private_key, fc::exception );

      BOOST_TEST_MESSAGE( " -- Fail to contribute after hard cap (bob)" );
      bob_op.contribution_id = bob_contribution_counter + 1;
      FAIL_WITH_OP( bob_op, bob_private_key, fc::exception );

      BOOST_TEST_MESSAGE( " -- Fail to contribute after hard cap (sam)" );
      sam_op.contribution_id = sam_contribution_counter + 1;
      FAIL_WITH_OP( sam_op, sam_private_key, fc::exception );

      validate_database();

      generate_block();

      db_plugin->debug_update( [=] ( database& db )
      {
         const smt_token_object *token = util::smt::find_token( db, alice_symbol );
         db.modify( *token, [&]( smt_token_object& o )
         {
            o.phase = smt_phase::ico_completed;
         } );
      } );

      BOOST_TEST_MESSAGE( " -- Failure SMT contribution phase has ended" );
      alice_op.contribution_id = alice_contribution_counter;
      FAIL_WITH_OP( alice_op, alice_private_key, fc::assert_exception );

      BOOST_TEST_MESSAGE( " -- Failure SMT contribution phase has ended" );
      bob_op.contribution_id = bob_contribution_counter;
      FAIL_WITH_OP( bob_op, bob_private_key, fc::assert_exception );

      BOOST_TEST_MESSAGE( " -- Failure SMT contribution phase has ended" );
      sam_op.contribution_id = sam_contribution_counter;
      FAIL_WITH_OP( sam_op, sam_private_key, fc::assert_exception );

      auto alices_contributions = asset( 0, STEEM_SYMBOL );
      auto bobs_contributions = asset( 0, STEEM_SYMBOL );
      auto sams_contributions = asset( 0, STEEM_SYMBOL );

      auto alices_num_contributions = 0;
      auto bobs_num_contributions = 0;
      auto sams_num_contributions = 0;

      const auto& idx = db->get_index< smt_contribution_index, by_symbol_contributor >();

      auto itr = idx.lower_bound( boost::make_tuple( alice_symbol, account_name_type( "alice" ), 0 ) );
      while( itr != idx.end() && itr->contributor == account_name_type( "alice" ) )
      {
         alices_contributions += itr->contribution;
         alices_num_contributions++;
         ++itr;
      }

      itr = idx.lower_bound( boost::make_tuple( alice_symbol, account_name_type( "bob" ), 0 ) );
      while( itr != idx.end() && itr->contributor == account_name_type( "bob" ) )
      {
         bobs_contributions += itr->contribution;
         bobs_num_contributions++;
         ++itr;
      }

      itr = idx.lower_bound( boost::make_tuple( alice_symbol, account_name_type( "sam" ), 0 ) );
      while( itr != idx.end() && itr->contributor == account_name_type( "sam" ) )
      {
         sams_contributions += itr->contribution;
         sams_num_contributions++;
         ++itr;
      }

      BOOST_TEST_MESSAGE( " -- Checking account contributions" );
      BOOST_REQUIRE( alices_contributions == alice_asset_accumulator );
      BOOST_REQUIRE( bobs_contributions == bob_asset_accumulator );
      BOOST_REQUIRE( sams_contributions == sam_asset_accumulator );

      BOOST_TEST_MESSAGE( " -- Checking contribution counts" );
      BOOST_REQUIRE( alices_num_contributions == alice_contribution_counter );
      BOOST_REQUIRE( bobs_num_contributions == bob_contribution_counter );
      BOOST_REQUIRE( sams_num_contributions == sam_contribution_counter );

      BOOST_TEST_MESSAGE( " -- Checking account balances" );
      BOOST_REQUIRE( db->get_balance( "alice", STEEM_SYMBOL ) == ASSET( "1000.000 TESTS" ) - alice_asset_accumulator );
      BOOST_REQUIRE( db->get_balance( "bob", STEEM_SYMBOL ) == ASSET( "1000.000 TESTS" ) - bob_asset_accumulator );
      BOOST_REQUIRE( db->get_balance( "sam", STEEM_SYMBOL ) == ASSET( "1000.000 TESTS" ) - sam_asset_accumulator );

      BOOST_TEST_MESSAGE( " -- Checking ICO total contributions" );
      const auto* ico_obj = db->find< smt_ico_object, by_symbol >( alice_symbol );
      BOOST_REQUIRE( ico_obj->contributed == alice_asset_accumulator + bob_asset_accumulator + sam_asset_accumulator );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_transfer_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_transfer_validate" );
      ACTORS( (alice) )
      auto symbol = create_smt( "alice", alice_private_key, 3 );

      transfer_operation op;
      op.from = "alice";
      op.to = "bob";
      op.memo = "Memo";
      op.amount = asset( 100, symbol );
      op.validate();

      BOOST_TEST_MESSAGE( " --- Invalid from account" );
      op.from = "alice-";
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.from = "alice";

      BOOST_TEST_MESSAGE( " --- Invalid to account" );
      op.to = "bob-";
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.to = "bob";

      BOOST_TEST_MESSAGE( " --- Memo too long" );
      std::string memo;
      for ( int i = 0; i < STEEM_MAX_MEMO_SIZE + 1; i++ )
         memo += "x";
      op.memo = memo;
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.memo = "Memo";

      BOOST_TEST_MESSAGE( " --- Negative amount" );
      op.amount = -op.amount;
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.amount = -op.amount;

      BOOST_TEST_MESSAGE( " --- Transferring vests" );
      op.amount = asset( 100, symbol.get_paired_symbol() );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.amount = asset( 100, symbol );

      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_transfer_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_transfer_apply" );

      ACTORS( (alice)(bob) )
      generate_block();

      auto symbol = create_smt( "alice", alice_private_key, 3 );

      fund( "alice", asset( 10000, symbol ) );

      BOOST_REQUIRE( db->get_balance( "alice", symbol ) == asset( 10000, symbol ) );
      BOOST_REQUIRE( db->get_balance( "bob", symbol ) == asset( 0, symbol ) );

      signed_transaction tx;
      transfer_operation op;

      op.from = "alice";
      op.to = "bob";
      op.amount = asset( 5000, symbol );

      BOOST_TEST_MESSAGE( "--- Test normal transaction" );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_balance( "alice", symbol ) == asset( 5000, symbol ) );
      BOOST_REQUIRE( db->get_balance( "bob", symbol ) == asset( 5000, symbol ) );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Generating a block" );
      generate_block();

      BOOST_REQUIRE( db->get_balance( "alice", symbol ) == asset( 5000, symbol ) );
      BOOST_REQUIRE( db->get_balance( "bob", symbol ) == asset( 5000, symbol ) );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test emptying an account" );
      tx.signatures.clear();
      tx.operations.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      db->push_transaction( tx, database::skip_transaction_dupe_check );

      BOOST_REQUIRE( db->get_balance( "alice", symbol ) == asset( 0, symbol ) );
      BOOST_REQUIRE( db->get_balance( "bob", symbol ) == asset( 10000, symbol ) );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test transferring non-existent funds" );
      tx.signatures.clear();
      tx.operations.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );

      BOOST_REQUIRE( db->get_balance( "alice", symbol ) == asset( 0, symbol ) );
      BOOST_REQUIRE( db->get_balance( "bob", symbol ) == asset( 10000, symbol ) );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_setup_validate )
{
   ACTORS( (alice) )
   generate_block();

   BOOST_TEST_MESSAGE( "Testing: smt_setup_operation validate" );

   smt_setup_operation op;
   op.control_account = "alice";
   op.symbol = create_smt( "alice", alice_private_key, 3 );

   op.contribution_begin_time = fc::variant( "2020-12-21T00:00:00" ).as< fc::time_point_sec >();
   op.contribution_end_time   = op.contribution_begin_time + fc::days( 365 );

   op.launch_time = op.contribution_end_time + fc::days( 1 );
   op.max_supply  = STEEM_MAX_SHARE_SUPPLY;
   op.steem_units_hard_cap = SMT_MIN_HARD_CAP_STEEM_UNITS;
   op.steem_units_soft_cap = SMT_MIN_SOFT_CAP_STEEM_UNITS;
   op.steem_units_min      = 0;

   smt_capped_generation_policy valid_generation_policy;
   valid_generation_policy.pre_soft_cap_unit = {
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 }
      },
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 }
      }
   };
   valid_generation_policy.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 }
      },
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 }
      }
   };
   valid_generation_policy.max_unit_ratio = 100;
   valid_generation_policy.min_unit_ratio = 50;
   op.initial_generation_policy = valid_generation_policy;

   op.validate();

   BOOST_TEST_MESSAGE( " -- Failure on invalid control acount name" );
   op.control_account = "@@@@";
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.control_account = "alice";

   BOOST_TEST_MESSAGE( " -- Failure on contribution_end_time > contribution_begin_time" );
   op.contribution_end_time = op.contribution_begin_time - fc::seconds( 1 );
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.contribution_end_time = op.contribution_begin_time + fc::days( 365 );

   BOOST_TEST_MESSAGE( " -- Failure on launch_time > contribution_end_time" );
   op.launch_time = op.contribution_end_time - fc::seconds( 1 );
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.launch_time = op.contribution_end_time + fc::days( 1 );

   BOOST_TEST_MESSAGE( " -- Failure on steem_units_hard_cap > SMT_MIN_HARD_CAP_STEEM_UNITS" );
   op.steem_units_hard_cap = SMT_MIN_HARD_CAP_STEEM_UNITS - 1;
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.steem_units_hard_cap = SMT_MIN_HARD_CAP_STEEM_UNITS * 2;

   BOOST_TEST_MESSAGE( " -- Failure on steem_units_soft_cap > SMT_MIN_SOFT_CAP_STEEM_UNITS" );
   op.steem_units_soft_cap = SMT_MIN_SOFT_CAP_STEEM_UNITS - 1;
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.steem_units_soft_cap = SMT_MIN_SOFT_CAP_STEEM_UNITS * 2;

   BOOST_TEST_MESSAGE( " -- Failure on steem_units_hard_cap > steem_units_soft_cap" );
   op.steem_units_hard_cap = op.steem_units_soft_cap - 1;
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.steem_units_hard_cap = SMT_MIN_HARD_CAP_STEEM_UNITS;

   BOOST_TEST_MESSAGE( " -- Failure on steem_units_soft_cap > steem_units_min" );
   op.steem_units_min = op.steem_units_soft_cap + 1;
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.steem_units_min = 0;

   BOOST_TEST_MESSAGE( " -- Failure on max_supply > STEEM_MAX_SHARE_SUPPLY" );
   op.max_supply = STEEM_MAX_SHARE_SUPPLY + 1;
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.max_supply = STEEM_MAX_SHARE_SUPPLY;

   BOOST_TEST_MESSAGE( " -- Successful sanity check" );
   op.validate();

   smt_capped_generation_policy invalid_generation_policy;
   invalid_generation_policy.pre_soft_cap_unit = {
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 1 }
      },
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 }
      }
   };
   invalid_generation_policy.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 }
      },
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 }
      }
   };
   invalid_generation_policy.max_unit_ratio = 100;
   invalid_generation_policy.min_unit_ratio = 50;

   BOOST_TEST_MESSAGE( " -- Failure on SMT_DESTINATION_REWARDS in steem_unit" );
   op.initial_generation_policy = invalid_generation_policy;
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.initial_generation_policy = valid_generation_policy;

   invalid_generation_policy.pre_soft_cap_unit = {
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_FROM, 1 }
      },
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 }
      }
   };
   invalid_generation_policy.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_FROM, 1 }
      },
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 }
      }
   };

   BOOST_TEST_MESSAGE( " -- Failure on SMT_DESTINATION_FROM in steem_unit" );
   op.initial_generation_policy = invalid_generation_policy;
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.initial_generation_policy = valid_generation_policy;

   invalid_generation_policy.pre_soft_cap_unit = {
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_FROM_VESTING, 1 }
      },
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 }
      }
   };
   invalid_generation_policy.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 }
      },
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 }
      }
   };

   BOOST_TEST_MESSAGE( " -- Failure on SMT_DESTINATION_FROM_VESTING in steem_unit" );
   op.initial_generation_policy = invalid_generation_policy;
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.initial_generation_policy = valid_generation_policy;

   invalid_generation_policy.pre_soft_cap_unit = {
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_VESTING, 1 }
      },
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 }
      }
   };
   invalid_generation_policy.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_VESTING, 1 }
      },
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 }
      }
   };

   BOOST_TEST_MESSAGE( " -- Failure on SMT_DESTINATION_VESTING in steem_unit" );
   op.initial_generation_policy = invalid_generation_policy;
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.initial_generation_policy = valid_generation_policy;

   invalid_generation_policy.pre_soft_cap_unit = {
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 }
      },
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 },
         { SMT_DESTINATION_VESTING, 1 }
      }
   };
   invalid_generation_policy.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 }
      },
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 },
         { SMT_DESTINATION_VESTING, 1 }
      }
   };

   BOOST_TEST_MESSAGE( " -- Failure on SMT_DESTINATION_VESTING in token_unit" );
   op.initial_generation_policy = invalid_generation_policy;
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.initial_generation_policy = valid_generation_policy;

   invalid_generation_policy.pre_soft_cap_unit = {
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { "$market_malformed", 2 }
      },
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 }
      }
   };
   invalid_generation_policy.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 }
      },
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 }
      }
   };

   BOOST_TEST_MESSAGE( " -- Failure on malformed special destination in pre soft cap steem unit" );
   op.initial_generation_policy = invalid_generation_policy;
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.initial_generation_policy = valid_generation_policy;

   invalid_generation_policy.pre_soft_cap_unit = {
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 }
      },
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { "$market_malformed", 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 }
      }
   };
   invalid_generation_policy.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 }
      },
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 }
      }
   };

   BOOST_TEST_MESSAGE( " -- Failure on malformed special destination in pre soft cap token unit" );
   op.initial_generation_policy = invalid_generation_policy;
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.initial_generation_policy = valid_generation_policy;

   invalid_generation_policy.pre_soft_cap_unit = {
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 }
      },
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 }
      }
   };
   invalid_generation_policy.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { "$market_maker1", 1 }
      },
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 }
      }
   };

   BOOST_TEST_MESSAGE( " -- Failure on malformed special destination in post soft cap steem unit" );
   op.initial_generation_policy = invalid_generation_policy;
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.initial_generation_policy = valid_generation_policy;

   invalid_generation_policy.pre_soft_cap_unit = {
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 }
      },
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 }
      }
   };
   invalid_generation_policy.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 }
      },
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { "$rewardsx", 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 }
      }
   };

   BOOST_TEST_MESSAGE( " -- Failure on malformed special destination in post soft cap token unit" );
   op.initial_generation_policy = invalid_generation_policy;
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.initial_generation_policy = valid_generation_policy;

   smt_capped_generation_policy valid_generation_policy2;
   valid_generation_policy2.pre_soft_cap_unit = {
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 }
      },
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 }
      }
   };
   valid_generation_policy2.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 }
      },
      {}
   };
   valid_generation_policy2.max_unit_ratio = 100;
   valid_generation_policy2.min_unit_ratio = 50;

   BOOST_TEST_MESSAGE( " -- Success on having an empty token unit in post soft cap units" );
   op.initial_generation_policy = valid_generation_policy2;
   op.validate();
   op.initial_generation_policy = valid_generation_policy;

   invalid_generation_policy.pre_soft_cap_unit = {
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 }
      },
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 },
         { SMT_DESTINATION_VESTING, 1 }
      }
   };
   invalid_generation_policy.post_soft_cap_unit = {
      {
         {}
      },
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 },
         { SMT_DESTINATION_VESTING, 1 }
      }
   };

   BOOST_TEST_MESSAGE( " -- Failure on empty post soft cap steem unit" );
   op.initial_generation_policy = invalid_generation_policy;
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.initial_generation_policy = valid_generation_policy;

   invalid_generation_policy.pre_soft_cap_unit = {
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 }
      },
      {
         {}
      }
   };
   invalid_generation_policy.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 }
      },
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 },
         { SMT_DESTINATION_VESTING, 1 }
      }
   };

   BOOST_TEST_MESSAGE( " -- Failure on empty pre soft cap token unit" );
   op.initial_generation_policy = invalid_generation_policy;
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.initial_generation_policy = valid_generation_policy;

   invalid_generation_policy.pre_soft_cap_unit = {
      {
         {}
      },
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 },
         { SMT_DESTINATION_VESTING, 1 }
      }
   };
   invalid_generation_policy.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 }
      },
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 },
         { SMT_DESTINATION_VESTING, 1 }
      }
   };

   BOOST_TEST_MESSAGE( " -- Failure on empty pre soft cap steem unit" );
   op.initial_generation_policy = invalid_generation_policy;
   BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );
   op.initial_generation_policy = valid_generation_policy;

   BOOST_TEST_MESSAGE( " -- Successful sanity check" );
   op.validate();
}


BOOST_AUTO_TEST_CASE( smt_setup_authorities )
{
   smt_setup_operation op;
   op.control_account = "alice";

   flat_set< account_name_type > auths;
   flat_set< account_name_type > expected;

   op.get_required_owner_authorities( auths );
   BOOST_REQUIRE( auths == expected );

   op.get_required_posting_authorities( auths );
   BOOST_REQUIRE( auths == expected );

   expected.insert( "alice" );
   op.get_required_active_authorities( auths );
   BOOST_REQUIRE( auths == expected );
}

BOOST_AUTO_TEST_CASE( smt_setup_apply )
{
   BOOST_TEST_MESSAGE( "Testing: smt_setup_operation apply" );
   ACTORS( (alice)(bob)(sam)(dave) )

   generate_block();

   BOOST_TEST_MESSAGE( " -- SMT creation" );
   auto symbol = create_smt( "alice", alice_private_key, 3 );

   BOOST_TEST_MESSAGE( " -- SMT setup" );
   signed_transaction tx;
   smt_setup_operation setup_op;

   setup_op.control_account = "alice";
   setup_op.symbol = symbol;
   setup_op.contribution_begin_time = db->head_block_time() + STEEM_BLOCK_INTERVAL;
   setup_op.contribution_end_time = setup_op.contribution_begin_time + fc::days( 30 );
   setup_op.steem_units_min      = 0;
   setup_op.steem_units_soft_cap = SMT_MIN_SOFT_CAP_STEEM_UNITS;
   setup_op.steem_units_hard_cap = SMT_MIN_HARD_CAP_STEEM_UNITS;
   setup_op.max_supply = STEEM_MAX_SHARE_SUPPLY;
   setup_op.launch_time = setup_op.contribution_end_time + fc::days( 1 );

   smt_capped_generation_policy invalid_generation_policy;
   invalid_generation_policy.pre_soft_cap_unit = {
      {
         { "elaine", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 }
      },
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 }
      }
   };
   invalid_generation_policy.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 }
      },
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 }
      }
   };
   invalid_generation_policy.max_unit_ratio = 100;
   invalid_generation_policy.min_unit_ratio = 50;
   setup_op.initial_generation_policy = invalid_generation_policy;

   BOOST_TEST_MESSAGE( " -- Failure on non-existent account (elaine) on pre soft cap steem unit" );
   tx.operations.push_back( setup_op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, alice_private_key );
   BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
   tx.operations.clear();
   tx.signatures.clear();

   invalid_generation_policy.pre_soft_cap_unit = {
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 }
      },
      {
         { "elaine", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 }
      }
   };
   invalid_generation_policy.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 }
      },
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 }
      }
   };
   invalid_generation_policy.max_unit_ratio = 100;
   invalid_generation_policy.min_unit_ratio = 50;
   setup_op.initial_generation_policy = invalid_generation_policy;

   BOOST_TEST_MESSAGE( " -- Failure on non-existent account (elaine) on pre soft cap token unit" );
   tx.operations.push_back( setup_op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, alice_private_key );
   BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
   tx.operations.clear();
   tx.signatures.clear();

   invalid_generation_policy.pre_soft_cap_unit = {
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 }
      },
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 }
      }
   };
   invalid_generation_policy.post_soft_cap_unit = {
      {
         { "elaine", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 }
      },
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 }
      }
   };
   invalid_generation_policy.max_unit_ratio = 100;
   invalid_generation_policy.min_unit_ratio = 50;
   setup_op.initial_generation_policy = invalid_generation_policy;

   BOOST_TEST_MESSAGE( " -- Failure on non-existent account (elaine) on post soft cap steem unit" );
   tx.operations.push_back( setup_op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, alice_private_key );
   BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
   tx.operations.clear();
   tx.signatures.clear();

   invalid_generation_policy.pre_soft_cap_unit = {
      {
         { "elaine", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 }
      },
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 }
      }
   };
   invalid_generation_policy.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 }
      },
      {
         { "elaine", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 }
      }
   };
   invalid_generation_policy.max_unit_ratio = 100;
   invalid_generation_policy.min_unit_ratio = 50;
   setup_op.initial_generation_policy = invalid_generation_policy;

   BOOST_TEST_MESSAGE( " -- Failure on non-existent account (elaine) on post soft cap token unit" );
   tx.operations.push_back( setup_op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, alice_private_key );
   BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
   tx.operations.clear();
   tx.signatures.clear();

   invalid_generation_policy.pre_soft_cap_unit = {
      {
         { "alice", 2 },
         { "$elaine.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 }
      },
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 }
      }
   };
   invalid_generation_policy.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 }
      },
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 }
      }
   };
   invalid_generation_policy.max_unit_ratio = 100;
   invalid_generation_policy.min_unit_ratio = 50;
   setup_op.initial_generation_policy = invalid_generation_policy;

   BOOST_TEST_MESSAGE( " -- Failure on non-existent founder vesting account (elaine) on pre soft cap steem unit" );
   tx.operations.push_back( setup_op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, alice_private_key );
   BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
   tx.operations.clear();
   tx.signatures.clear();

   invalid_generation_policy.pre_soft_cap_unit = {
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 }
      },
      {
         { "alice", 2 },
         { "$elaine.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 }
      }
   };
   invalid_generation_policy.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 }
      },
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 }
      }
   };
   invalid_generation_policy.max_unit_ratio = 100;
   invalid_generation_policy.min_unit_ratio = 50;
   setup_op.initial_generation_policy = invalid_generation_policy;

   BOOST_TEST_MESSAGE( " -- Failure on non-existent founder vesting account (elaine) on pre soft cap token unit" );
   tx.operations.push_back( setup_op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, alice_private_key );
   BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
   tx.operations.clear();
   tx.signatures.clear();

   invalid_generation_policy.pre_soft_cap_unit = {
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 }
      },
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 }
      }
   };
   invalid_generation_policy.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$elaine.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 }
      },
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 }
      }
   };
   invalid_generation_policy.max_unit_ratio = 100;
   invalid_generation_policy.min_unit_ratio = 50;
   setup_op.initial_generation_policy = invalid_generation_policy;

   BOOST_TEST_MESSAGE( " -- Failure on non-existent founder vesting account (elaine) on post soft cap steem unit" );
   tx.operations.push_back( setup_op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, alice_private_key );
   BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
   tx.operations.clear();
   tx.signatures.clear();

   invalid_generation_policy.pre_soft_cap_unit = {
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 }
      },
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 }
      }
   };
   invalid_generation_policy.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 }
      },
      {
         { "alice", 1 },
         { "$elaine.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 }
      }
   };
   invalid_generation_policy.max_unit_ratio = 100;
   invalid_generation_policy.min_unit_ratio = 50;
   setup_op.initial_generation_policy = invalid_generation_policy;

   BOOST_TEST_MESSAGE( " -- Failure on non-existent founder vesting account (elaine) on post soft cap token unit" );
   tx.operations.push_back( setup_op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, alice_private_key );
   BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
   tx.operations.clear();
   tx.signatures.clear();

   smt_capped_generation_policy valid_generation_policy;
   valid_generation_policy.pre_soft_cap_unit = {
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 }
      },
      {
         { "alice", 2 },
         { "$alice.vesting", 2 },
         { SMT_DESTINATION_MARKET_MAKER, 2 },
         { SMT_DESTINATION_REWARDS, 2 },
         { SMT_DESTINATION_FROM, 2 },
         { SMT_DESTINATION_FROM_VESTING, 2 }
      }
   };
   valid_generation_policy.post_soft_cap_unit = {
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 }
      },
      {
         { "alice", 1 },
         { "$alice.vesting", 1 },
         { SMT_DESTINATION_MARKET_MAKER, 1 },
         { SMT_DESTINATION_REWARDS, 1 },
         { SMT_DESTINATION_FROM, 1 },
         { SMT_DESTINATION_FROM_VESTING, 1 }
      }
   };
   valid_generation_policy.max_unit_ratio = 100;
   valid_generation_policy.min_unit_ratio = 50;
   setup_op.initial_generation_policy = valid_generation_policy;

   setup_op.control_account = "bob";

   BOOST_TEST_MESSAGE( " -- Failure to setup SMT with non controlling account" );
   tx.operations.push_back( setup_op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, bob_private_key );
   BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
   tx.operations.clear();
   tx.signatures.clear();

   setup_op.control_account = "alice";

   BOOST_TEST_MESSAGE( " -- Success on valid SMT setup" );
   tx.operations.push_back( setup_op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   sign( tx, alice_private_key );
   db->push_transaction( tx, 0 );

   generate_block();
}

BOOST_AUTO_TEST_CASE( comment_votable_assets_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Test Comment Votable Assets Validate" );
      ACTORS((alice));

      generate_block();

      std::array< asset_symbol_type, SMT_MAX_VOTABLE_ASSETS + 1 > smts;
      /// Create one more than limit to test negative cases
      for( size_t i = 0; i < SMT_MAX_VOTABLE_ASSETS + 1; ++i )
      {
         smts[i] = create_smt( "alice", alice_private_key, 0 );
      }

      {
         comment_options_operation op;

         op.author = "alice";
         op.permlink = "test";

         BOOST_TEST_MESSAGE( "--- Testing valid configuration: no votable_assets" );
         allowed_vote_assets ava;
         op.extensions.insert( ava );
         op.validate();
      }

      {
         comment_options_operation op;

         op.author = "alice";
         op.permlink = "test";

         BOOST_TEST_MESSAGE( "--- Testing valid configuration of votable_assets" );
         allowed_vote_assets ava;
         for( size_t i = 0; i < SMT_MAX_VOTABLE_ASSETS; ++i )
         {
            ava.add_votable_asset( smts[i], 10 + i, (i & 2) != 0 );
         }

         op.extensions.insert( ava );
         op.validate();
      }

      {
         comment_options_operation op;

         op.author = "alice";
         op.permlink = "test";

         BOOST_TEST_MESSAGE( "--- Testing invalid configuration of votable_assets - too much assets specified" );
         allowed_vote_assets ava;
         for( size_t i = 0; i < smts.size(); ++i )
         {
            ava.add_votable_asset( smts[i], 20 + i, (i & 2) != 0 );
         }

         op.extensions.insert( ava );
         STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      }

      {
         comment_options_operation op;

         op.author = "alice";
         op.permlink = "test";

         BOOST_TEST_MESSAGE( "--- Testing invalid configuration of votable_assets - STEEM added to container" );
         allowed_vote_assets ava;
         ava.add_votable_asset( smts.front(), 20, false);
         ava.add_votable_asset( STEEM_SYMBOL, 20, true);
         op.extensions.insert( ava );
         STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      }

      {
         comment_options_operation op;

         op.author = "alice";
         op.permlink = "test";

         BOOST_TEST_MESSAGE( "--- Testing more than 100% weight on a single route" );
         allowed_vote_assets ava;
         ava.add_votable_asset( smts[0], 10, true );

         auto& b = ava.votable_assets[smts[0]].beneficiaries;

         b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "bob" ), STEEM_100_PERCENT + 1 ) );
         op.extensions.insert( ava );
         STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

         BOOST_TEST_MESSAGE( "--- Testing more than 100% total weight" );
         b.beneficiaries.clear();
         b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "bob" ), STEEM_1_PERCENT * 75 ) );
         b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "sam" ), STEEM_1_PERCENT * 75 ) );
         op.extensions.clear();
         op.extensions.insert( ava );
         STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

         BOOST_TEST_MESSAGE( "--- Testing maximum number of routes" );
         b.beneficiaries.clear();
         for( size_t i = 0; i < 127; i++ )
         {
            b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "foo" + fc::to_string( i ) ), 1 ) );
         }

         op.extensions.clear();
         std::sort( b.beneficiaries.begin(), b.beneficiaries.end() );
         op.extensions.insert( ava );
         op.validate();

         BOOST_TEST_MESSAGE( "--- Testing one too many routes" );
         b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "bar" ), 1 ) );
         std::sort( b.beneficiaries.begin(), b.beneficiaries.end() );
         op.extensions.clear();
         op.extensions.insert( ava );
         STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );


         BOOST_TEST_MESSAGE( "--- Testing duplicate accounts" );
         b.beneficiaries.clear();
         b.beneficiaries.push_back( beneficiary_route_type( "bob", STEEM_1_PERCENT * 2 ) );
         b.beneficiaries.push_back( beneficiary_route_type( "bob", STEEM_1_PERCENT ) );
         op.extensions.clear();
         op.extensions.insert( ava );
         STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

         BOOST_TEST_MESSAGE( "--- Testing incorrect account sort order" );
         b.beneficiaries.clear();
         b.beneficiaries.push_back( beneficiary_route_type( "bob", STEEM_1_PERCENT ) );
         b.beneficiaries.push_back( beneficiary_route_type( "alice", STEEM_1_PERCENT ) );
         op.extensions.clear();
         op.extensions.insert( ava );
         STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

         BOOST_TEST_MESSAGE( "--- Testing correct account sort order" );
         b.beneficiaries.clear();
         b.beneficiaries.push_back( beneficiary_route_type( "alice", STEEM_1_PERCENT ) );
         b.beneficiaries.push_back( beneficiary_route_type( "bob", STEEM_1_PERCENT ) );
         op.extensions.clear();
         op.extensions.insert( ava );
         op.validate();
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( comment_votable_assets_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: comment votable assets apply" );
      ACTORS( (alice)(bob) );

      SMT_SYMBOL( alice, 3, db );
      generate_block();

      comment_operation comment;
      comment_options_operation op;
      allowed_vote_assets ava;
      votable_asset_options opts;
      signed_transaction tx;

      comment.author = "alice";
      comment.permlink = "test";
      comment.parent_permlink = "test";
      comment.title = "test";
      comment.body = "foobar";

      tx.operations.push_back( comment );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      db->push_transaction( tx );


      BOOST_TEST_MESSAGE( "--- Failure when SMT does not exist" );

      op.author = "alice";
      op.permlink = "test";
      ava.votable_assets[ alice_symbol ] = opts;
      op.extensions.insert( ava );
      tx.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Failure when SMT is not launched" );

      alice_symbol = create_smt( "alice", alice_private_key, 3 );
      ava.votable_assets.clear();
      ava.votable_assets[ alice_symbol ] = opts;
      op.extensions.clear();
      op.extensions.insert( ava );
      tx.clear();
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Success" );

      generate_block();
      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get< smt_token_object, by_symbol >( alice_symbol ), [=]( smt_token_object& smt )
         {
            smt.phase = smt_phase::launch_success;
         });
      });
      generate_block();

      ava.votable_assets.clear();
      ava.votable_assets[ alice_symbol ] = opts;
      op.extensions.clear();
      op.extensions.insert( ava );
      tx.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      {
         const auto& alice_comment = db->get_comment( op.author, op.permlink );

         BOOST_REQUIRE( alice_comment.allowed_vote_assets.find( alice_symbol ) != alice_comment.allowed_vote_assets.end() );
         const auto va_opts = alice_comment.allowed_vote_assets.find( alice_symbol );
         BOOST_REQUIRE( va_opts->second.max_accepted_payout == opts.max_accepted_payout );
         BOOST_REQUIRE( va_opts->second.allow_curation_rewards == opts.allow_curation_rewards );
         BOOST_REQUIRE( va_opts->second.beneficiaries.beneficiaries.size() == 0 );
      }


      BOOST_TEST_MESSAGE( "--- Failure with non-existent beneficiary" );

      opts.beneficiaries.beneficiaries.push_back( beneficiary_route_type{ "charlie", STEEM_100_PERCENT } );
      ava.votable_assets.clear();
      ava.votable_assets[ alice_symbol ] = opts;
      op.extensions.clear();
      op.extensions.insert( ava );
      tx.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Success changing when rshares are 0" );

      opts.beneficiaries.beneficiaries.clear();
      opts.beneficiaries.beneficiaries.push_back( beneficiary_route_type{ "bob", STEEM_100_PERCENT } );
      opts.max_accepted_payout = 100;
      opts.allow_curation_rewards = false;
      ava.votable_assets.clear();
      ava.votable_assets[ alice_symbol ] = opts;
      op.extensions.clear();
      op.extensions.insert( ava );
      tx.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );
      {
         const auto& alice_comment = db->get_comment( op.author, op.permlink );

         BOOST_REQUIRE( alice_comment.allowed_vote_assets.find( alice_symbol ) != alice_comment.allowed_vote_assets.end() );
         const auto va_opts = alice_comment.allowed_vote_assets.find( alice_symbol );
         BOOST_REQUIRE( va_opts->second.max_accepted_payout == opts.max_accepted_payout );
         BOOST_REQUIRE( va_opts->second.allow_curation_rewards == opts.allow_curation_rewards );
         BOOST_REQUIRE( va_opts->second.beneficiaries.beneficiaries.size() == 1 );
      }


      BOOST_TEST_MESSAGE( "--- Failure changing when rshares are non-zero" );

      generate_block();
      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_comment( op.author, op.permlink ), [=]( comment_object& c )
         {
            c.net_rshares = 1;
            c.abs_rshares = 1;
         });
      });
      generate_block();

      opts.beneficiaries.beneficiaries.clear();
      ava.votable_assets.clear();
      ava.votable_assets[ alice_symbol ] = opts;
      op.extensions.clear();
      op.extensions.insert( ava );
      tx.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Failure specifying non-inherited votable asset" );

      generate_block();
      auto bob_symbol = create_smt( "bob", bob_private_key, 3 );
      generate_block();
      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get< smt_token_object, by_symbol >( bob_symbol ), [=]( smt_token_object& smt )
         {
            smt.phase = smt_phase::launch_success;
         });
      });
      generate_block();

      comment.parent_author = comment.author;
      comment.parent_permlink = comment.permlink;
      comment.author = "bob";
      tx.clear();
      tx.operations.push_back( comment );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );

      op.author = comment.author;
      ava.votable_assets.clear();
      ava.votable_assets[ bob_symbol ] = opts;
      op.extensions.clear();
      op.extensions.insert( ava );
      tx.clear();
      tx.operations.push_back( op );
      sign( tx, bob_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Success specifying inherited votable asset" );

      ava.votable_assets.clear();
      ava.votable_assets[ alice_symbol ] = opts;
      op.extensions.clear();
      op.extensions.insert( ava );
      tx.clear();
      tx.operations.push_back( op );
      sign( tx, bob_private_key );
      db->push_transaction( tx, 0 );


      BOOST_TEST_MESSAGE( "--- Failure specifying an SMT that disallows voting" );

      generate_block();
      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get< smt_token_object, by_symbol >( bob_symbol ), [=]( smt_token_object& smt )
         {
            smt.allow_voting = false;
         });
      });
      generate_block();

      comment.parent_author = "";
      comment.parent_permlink = "foorbar";
      comment.permlink = "foobar";
      op.author = comment.author;
      op.permlink = comment.permlink;
      ava.votable_assets.clear();
      ava.votable_assets[ bob_symbol ] = opts;
      op.extensions.clear();
      op.extensions.insert( ava );
      tx.clear();
      tx.operations.push_back( comment );
      tx.operations.push_back( op );
      sign( tx, bob_private_key );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( vote2_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: vote_authorities" );

      vote2_operation op;
      op.voter = "alice";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "alice" );
      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( vote2_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: vote2_apply" );

      ACTORS( (alice)(bob)(sam)(dave) )
      generate_block();

      auto alice_symbol = create_smt( "alice", alice_private_key, 3 );
      auto bob_symbol   = create_smt( "bob", bob_private_key, 3 );

      vest( STEEM_INIT_MINER_NAME, "alice", ASSET( "10.000 TESTS" ) );
      validate_database();
      vest( STEEM_INIT_MINER_NAME, "bob" , ASSET( "10.000 TESTS" ) );
      vest( STEEM_INIT_MINER_NAME, "sam" , ASSET( "10.000 TESTS" ) );
      vest( STEEM_INIT_MINER_NAME, "dave" , ASSET( "10.000 TESTS" ) );

      db_plugin->debug_update( [=]( database& db )
      {
         auto alice_vests = db.create_vesting( db.get_account( "alice" ), asset( 100000, alice_symbol ), false );
         alice_vests += db.create_vesting( db.get_account( "bob" ),   asset( 100000, alice_symbol ), false );
         alice_vests += db.create_vesting( db.get_account( "sam" ),   asset( 100000, alice_symbol ), false );
         alice_vests += db.create_vesting( db.get_account( "dave" ),  asset( 100000, alice_symbol ), false );

         auto bob_vests = db.create_vesting( db.get_account( "alice" ), asset( 100000, bob_symbol ), false );

         db.modify( db.get< smt_token_object, by_symbol >( alice_symbol ), [=]( smt_token_object& smt )
         {
            smt.phase = smt_phase::launch_success;
            smt.current_supply = 40000;
            smt.total_vesting_shares = alice_vests.amount;
            smt.total_vesting_fund_smt = 40000;
            smt.votes_per_regeneration_period = 50;
            smt.vote_regeneration_period_seconds = 5*24*60*60;
         });

         db.modify( db.get< smt_token_object, by_symbol >( bob_symbol ), [=]( smt_token_object& smt )
         {
            smt.phase = smt_phase::launch_success;
            smt.current_supply = 10000;
            smt.total_vesting_shares = bob_vests.amount;
            smt.total_vesting_fund_smt = 10000;
         });
      });
      generate_block();

      const auto& vote_idx = db->get_index< comment_vote_index, by_comment_voter_symbol >();

      {
         const auto& alice = db->get_account( "alice" );
         const auto& alice_smt = db->get< account_regular_balance_object, by_owner_liquid_symbol >( boost::make_tuple( "alice", alice_symbol ) );

         signed_transaction tx;
         comment_operation comment_op;
         comment_op.author = "alice";
         comment_op.permlink = "foo";
         comment_op.parent_permlink = "test";
         comment_op.title = "bar";
         comment_op.body = "foo bar";
         allowed_vote_assets ava;
         ava.votable_assets[ alice_symbol ] = votable_asset_options();
         comment_options_operation comment_opts;
         comment_opts.author = "alice";
         comment_opts.permlink = "foo";
         comment_opts.extensions.insert( ava );
         tx.operations.push_back( comment_op );
         tx.operations.push_back( comment_opts );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         sign( tx, alice_private_key );
         db->push_transaction( tx, 0 );


         BOOST_TEST_MESSAGE( "--- Testing voting on a non-existent comment" );

         tx.operations.clear();
         tx.signatures.clear();

         vote2_operation op;
         op.voter = "alice";
         op.author = "bob";
         op.permlink = "foo";
         op.rshares = alice.vesting_shares.amount.value / 50;
         tx.operations.push_back( op );
         sign( tx, alice_private_key );

         STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

         validate_database();


         BOOST_TEST_MESSAGE( "--- Testing voting with 0 rshares" );

         op.rshares = 0;
         tx.operations.clear();
         tx.signatures.clear();
         tx.operations.push_back( op );
         sign( tx, alice_private_key );

         STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

         validate_database();


         BOOST_TEST_MESSAGE( "--- Testing voting with a non-existent SMT" );

         SMT_SYMBOL( sam, 3, db );
         op.rshares = alice.vesting_shares.amount.value / 50;
         op.author = "alice";
         op.smt_rshares[ sam_symbol ] = 1;
         tx.clear();
         tx.operations.push_back( op );
         sign( tx, alice_private_key );

         STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

         validate_database();


         BOOST_TEST_MESSAGE( "--- Test voting with a non-votable asset" );
         op.smt_rshares.clear();
         op.smt_rshares[ bob_symbol ] = 1;
         tx.clear();
         tx.operations.push_back( op );
         sign( tx, alice_private_key );

         STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

         validate_database();


         BOOST_TEST_MESSAGE( "--- Testing success" );

         util::manabar_params params( util::get_effective_vesting_shares( alice ), STEEM_VOTING_MANA_REGENERATION_SECONDS );
         util::manabar old_manabar = alice.voting_manabar;
         old_manabar.regenerate_mana( params, db->head_block_time() );

         util::manabar old_smt_manabar = alice_smt.voting_manabar;
         params.max_mana = util::get_effective_vesting_shares( alice_smt );
         old_smt_manabar.regenerate_mana( params, db->head_block_time() );

         op.smt_rshares.clear();
         op.smt_rshares[ alice_symbol ] = alice_smt.vesting_shares.amount.value / 50;
         tx.operations.clear();
         tx.signatures.clear();
         tx.operations.push_back( op );
         sign( tx, alice_private_key );

         db->push_transaction( tx, 0 );

         auto& alice_comment = db->get_comment( "alice", string( "foo" ) );
         auto itr = vote_idx.find( boost::make_tuple( alice_comment.id, alice.id, STEEM_SYMBOL ) );

         BOOST_REQUIRE( alice.last_vote_time == db->head_block_time() );
         BOOST_REQUIRE( old_manabar.current_mana - op.rshares == alice.voting_manabar.current_mana );
         BOOST_REQUIRE( alice_comment.net_rshares.value == op.rshares - STEEM_VOTE_DUST_THRESHOLD );
         BOOST_REQUIRE( alice_comment.cashout_time == alice_comment.created + STEEM_CASHOUT_WINDOW_SECONDS );
         BOOST_REQUIRE( itr->rshares == op.rshares - STEEM_VOTE_DUST_THRESHOLD );
         BOOST_REQUIRE( itr != vote_idx.end() );

         itr = vote_idx.find( boost::make_tuple( alice_comment.id, alice.id, alice_symbol ) );

         idump( (old_smt_manabar.current_mana)(op.smt_rshares[alice_symbol])(alice_smt.voting_manabar.current_mana) );

         BOOST_REQUIRE( old_smt_manabar.current_mana - op.smt_rshares[ alice_symbol ] == alice_smt.voting_manabar.current_mana );
         BOOST_REQUIRE( alice_comment.smt_rshares.find( alice_symbol )->second.net_rshares.value == op.smt_rshares[ alice_symbol ] - STEEM_VOTE_DUST_THRESHOLD );
         BOOST_REQUIRE( itr != vote_idx.end() );
         BOOST_REQUIRE( itr->rshares == op.smt_rshares[ alice_symbol ] - STEEM_VOTE_DUST_THRESHOLD );

         validate_database();


         BOOST_TEST_MESSAGE( "--- Test reduced power for quick voting" );

         generate_blocks( db->head_block_time() + STEEM_MIN_VOTE_INTERVAL_SEC );
         {
            const auto& alice = db->get_account( "alice" );
            params.max_mana = util::get_effective_vesting_shares( alice );
            old_manabar = alice.voting_manabar;
            old_manabar.regenerate_mana( params, db->head_block_time() );

            const auto& alice_smt = db->get< account_regular_balance_object, by_owner_liquid_symbol >( boost::make_tuple( "alice", alice_symbol ) );
            params.max_mana = util::get_effective_vesting_shares( alice_smt );
            old_smt_manabar = alice_smt.voting_manabar;
            old_smt_manabar.regenerate_mana( params, db->head_block_time() );

            comment_op.author = "bob";
            comment_op.permlink = "foo";
            comment_op.title = "bar";
            comment_op.body = "foo bar";
            comment_opts.author = "bob";
            comment_opts.permlink = "foo";
            tx.operations.clear();
            tx.signatures.clear();
            tx.operations.push_back( comment_op );
            tx.operations.push_back( comment_opts );
            sign( tx, bob_private_key );
            db->push_transaction( tx, 0 );

            op.rshares = old_manabar.current_mana / 50;
            op.smt_rshares[ alice_symbol ] = old_smt_manabar.current_mana / 50;
            op.voter = "alice";
            op.author = "bob";
            op.permlink = "foo";

            op.rshares += 10;
            tx.clear();
            tx.operations.push_back( op );
            sign( tx, alice_private_key );
            BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

            op.rshares -= 10;
            op.smt_rshares[ alice_symbol ] += 10;
            tx.clear();
            tx.operations.push_back( op );
            sign( tx, alice_private_key );
            BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

            op.smt_rshares[ alice_symbol ] -= 10;
            tx.clear();
            tx.operations.push_back( op );
            sign( tx, alice_private_key );
            db->push_transaction( tx, 0 );

            const auto& bob_comment = db->get_comment( "bob", string( "foo" ) );
            itr = vote_idx.find( boost::make_tuple( bob_comment.id, alice.id, STEEM_SYMBOL ) );

            BOOST_REQUIRE( bob_comment.net_rshares.value == op.rshares - STEEM_VOTE_DUST_THRESHOLD );
            BOOST_REQUIRE( bob_comment.cashout_time == bob_comment.created + STEEM_CASHOUT_WINDOW_SECONDS );
            BOOST_REQUIRE( alice.voting_manabar.current_mana == old_manabar.current_mana - op.rshares );
            BOOST_REQUIRE( itr != vote_idx.end() );

            itr = vote_idx.find( boost::make_tuple( bob_comment.id, alice.id, alice_symbol ) );
            BOOST_REQUIRE( bob_comment.smt_rshares.find( alice_symbol )->second.net_rshares.value == op.smt_rshares[ alice_symbol ] - STEEM_VOTE_DUST_THRESHOLD );
            BOOST_REQUIRE( alice_smt.voting_manabar.current_mana == old_smt_manabar.current_mana - op.smt_rshares[ alice_symbol ] );
            BOOST_REQUIRE( itr != vote_idx.end() );

            validate_database();
         }


         BOOST_TEST_MESSAGE( "--- Test payout time extension on vote" );
         {
            old_manabar = db->get_account( "bob" ).voting_manabar;
            params.max_mana = util::get_effective_vesting_shares( db->get_account( "bob" ) );
            old_manabar.regenerate_mana( params, db->head_block_time() );

            const auto& bob_smt = db->get< account_regular_balance_object, by_owner_liquid_symbol >( boost::make_tuple( "bob", alice_symbol ) );
            params.max_mana = util::get_effective_vesting_shares( bob_smt );
            old_smt_manabar = bob_smt.voting_manabar;
            old_smt_manabar.regenerate_mana( params, db->head_block_time() );

            auto old_abs_rshares = db->get_comment( "alice", string( "foo" ) ).abs_rshares.value;
            auto old_smt_abs_rshares = db->get_comment( "alice", string( "foo" ) ).smt_rshares.find( alice_symbol )->second.abs_rshares.value;

            generate_blocks( db->head_block_time() + fc::seconds( ( STEEM_CASHOUT_WINDOW_SECONDS / 2 ) ), true );

            const auto& new_bob = db->get_account( "bob" );
            const auto& new_bob_smt = db->get< account_regular_balance_object, by_owner_liquid_symbol >( boost::make_tuple( "bob", alice_symbol ) );
            const auto& new_alice_comment = db->get_comment( "alice", string( "foo" ) );

            op.rshares = old_manabar.current_mana / 50;
            op.smt_rshares[ alice_symbol ] = old_smt_manabar.current_mana / 50;
            op.voter = "bob";
            op.author = "alice";
            op.permlink = "foo";
            tx.operations.clear();
            tx.signatures.clear();
            tx.operations.push_back( op );
            tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
            sign( tx, bob_private_key );
            db->push_transaction( tx, 0 );

            itr = vote_idx.find( boost::make_tuple( new_alice_comment.id, new_bob.id, STEEM_SYMBOL ) );

            BOOST_REQUIRE( new_alice_comment.net_rshares.value == old_abs_rshares + ( old_manabar.current_mana - new_bob.voting_manabar.current_mana ) - STEEM_VOTE_DUST_THRESHOLD );
            BOOST_REQUIRE( new_alice_comment.cashout_time == new_alice_comment.created + STEEM_CASHOUT_WINDOW_SECONDS );
            BOOST_REQUIRE( itr != vote_idx.end() );

            itr = vote_idx.find( boost::make_tuple( new_alice_comment.id, new_bob.id, alice_symbol ) );
            idump( (new_alice_comment.smt_rshares.find( alice_symbol )->second.net_rshares.value) );
            idump( (old_smt_abs_rshares)(old_smt_manabar.current_mana)(new_bob_smt.voting_manabar.current_mana) );
            BOOST_REQUIRE( new_alice_comment.smt_rshares.find( alice_symbol )->second.net_rshares.value == old_smt_abs_rshares + ( old_smt_manabar.current_mana - new_bob_smt.voting_manabar.current_mana ) - STEEM_VOTE_DUST_THRESHOLD );
            BOOST_REQUIRE( itr != vote_idx.end() );

            validate_database();
         }


         BOOST_TEST_MESSAGE( "--- Test negative vote" );
         util::manabar old_downvote_manabar;
         util::manabar old_smt_downvote_manabar;

         {
            const auto& new_sam = db->get_account( "sam" );
            const auto& new_sam_smt = db->get< account_regular_balance_object, by_owner_liquid_symbol >( boost::make_tuple( "sam", alice_symbol ) );
            const auto& new_bob_comment = db->get_comment( "bob", string( "foo" ) );

            auto old_abs_rshares = new_bob_comment.abs_rshares.value;
            auto old_smt_abs_rshares = new_bob_comment.smt_rshares.find( alice_symbol )->second.abs_rshares.value;

            old_manabar = new_sam.voting_manabar;
            params.max_mana = util::get_effective_vesting_shares( new_sam );
            old_manabar.regenerate_mana( params, db->head_block_time() );

            const auto& sam_smt = db->get< account_regular_balance_object, by_owner_liquid_symbol >( boost::make_tuple( "sam", alice_symbol ) );
            params.max_mana = util::get_effective_vesting_shares( sam_smt );
            old_smt_manabar = sam_smt.voting_manabar;
            old_smt_manabar.regenerate_mana( params, db->head_block_time() );

            op.rshares = -1 * old_manabar.current_mana / 50 / 2;
            op.smt_rshares[ alice_symbol ] = -1 * old_smt_manabar.current_mana / 50 / 2;
            op.voter = "sam";
            op.author = "bob";
            op.permlink = "foo";
            tx.operations.clear();
            tx.signatures.clear();
            tx.operations.push_back( op );
            sign( tx, sam_private_key );
            db->push_transaction( tx, 0 );

            itr = vote_idx.find( boost::make_tuple( new_bob_comment.id, new_sam.id, STEEM_SYMBOL ) );

            params.max_mana = util::get_effective_vesting_shares( new_sam ) / 4;
            old_downvote_manabar.regenerate_mana( params, db->head_block_time() );
            int64_t sam_rshares = old_downvote_manabar.current_mana - new_sam.downvote_manabar.current_mana - STEEM_VOTE_DUST_THRESHOLD;

            params.max_mana = util::get_effective_vesting_shares( new_sam_smt ) / 4;
            old_smt_downvote_manabar.regenerate_mana( params, db->head_block_time() );
            int64_t sam_smt_rshares = old_smt_downvote_manabar.current_mana - new_sam_smt.downvote_manabar.current_mana - STEEM_VOTE_DUST_THRESHOLD;

            BOOST_REQUIRE( new_bob_comment.net_rshares.value == old_abs_rshares - sam_rshares );
            BOOST_REQUIRE( new_bob_comment.abs_rshares.value == old_abs_rshares + sam_rshares );
            BOOST_REQUIRE( new_bob_comment.cashout_time == new_bob_comment.created + STEEM_CASHOUT_WINDOW_SECONDS );
            BOOST_REQUIRE( itr != vote_idx.end() );

            itr = vote_idx.find( boost::make_tuple( new_bob_comment.id, new_sam.id, alice_symbol ) );

            BOOST_REQUIRE( new_bob_comment.smt_rshares.find( alice_symbol )->second.net_rshares.value == old_smt_abs_rshares - sam_smt_rshares );
            BOOST_REQUIRE( new_bob_comment.smt_rshares.find( alice_symbol )->second.abs_rshares.value == old_smt_abs_rshares + sam_smt_rshares );
            BOOST_REQUIRE( itr != vote_idx.end() );
            validate_database();
         }


         BOOST_TEST_MESSAGE( "--- Test nested voting on nested comments" );
         {
            old_manabar = db->get_account( "alice" ).voting_manabar;
            params.max_mana = util::get_effective_vesting_shares( db->get_account( "alice" ) );
            old_manabar.regenerate_mana( params, db->head_block_time() );

            const auto& alice_smt = db->get< account_regular_balance_object, by_owner_liquid_symbol >( boost::make_tuple( "alice", alice_symbol ) );
            params.max_mana = util::get_effective_vesting_shares( alice_smt );
            old_smt_manabar = alice_smt.voting_manabar;
            old_smt_manabar.regenerate_mana( params, db->head_block_time() );

            comment_op.author = "sam";
            comment_op.permlink = "foo";
            comment_op.title = "bar";
            comment_op.body = "foo bar";
            comment_op.parent_author = "alice";
            comment_op.parent_permlink = "foo";
            tx.operations.clear();
            tx.signatures.clear();
            tx.operations.push_back( comment_op );
            sign( tx, sam_private_key );
            db->push_transaction( tx, 0 );

            op.rshares = old_manabar.current_mana / 50;
            op.smt_rshares[ alice_symbol ] = old_smt_manabar.current_mana / 50;
            op.voter = "alice";
            op.author = "sam";
            op.permlink = "foo";
            tx.operations.clear();
            tx.signatures.clear();
            tx.operations.push_back( op );
            sign( tx, alice_private_key );
            db->push_transaction( tx, 0 );

            BOOST_REQUIRE( db->get_comment( "alice", string( "foo" ) ).cashout_time == db->get_comment( "alice", string( "foo" ) ).created + STEEM_CASHOUT_WINDOW_SECONDS );

            validate_database();
         }


         BOOST_TEST_MESSAGE( "--- Test increasing vote rshares" );
         {
            generate_blocks( db->head_block_time() + STEEM_MIN_VOTE_INTERVAL_SEC );

            const auto& new_alice = db->get_account( "alice" );
            const auto& new_bob_comment = db->get_comment( "bob", string( "foo" ) );
            auto alice_bob_vote = vote_idx.find( boost::make_tuple( new_bob_comment.id, new_alice.id, STEEM_SYMBOL ) );
            auto old_vote_rshares = alice_bob_vote->rshares;
            auto old_net_rshares = new_bob_comment.net_rshares.value;
            auto old_abs_rshares = new_bob_comment.abs_rshares.value;

            alice_bob_vote = vote_idx.find( boost::make_tuple( new_bob_comment.id, new_alice.id, alice_symbol ) );
            auto old_smt_vote_rshares = alice_bob_vote->rshares;
            auto old_smt_net_rshares = new_bob_comment.smt_rshares.find( alice_symbol )->second.net_rshares.value;
            auto old_smt_abs_rshares = new_bob_comment.smt_rshares.find( alice_symbol )->second.abs_rshares.value;

            old_manabar = db->get_account( "alice" ).voting_manabar;
            params.max_mana = util::get_effective_vesting_shares( db->get_account( "alice" ) );
            old_manabar.regenerate_mana( params, db->head_block_time() );

            const auto& alice_smt = db->get< account_regular_balance_object, by_owner_liquid_symbol >( boost::make_tuple( "sam", alice_symbol ) );
            params.max_mana = util::get_effective_vesting_shares( alice_smt );
            old_smt_manabar = alice_smt.voting_manabar;
            old_smt_manabar.regenerate_mana( params, db->head_block_time() );

            op.voter = "alice";
            op.rshares = old_manabar.current_mana / 50 / 2;
            op.smt_rshares[ alice_symbol ] = old_smt_manabar.current_mana / 50 / 2;
            op.author = "bob";
            op.permlink = "foo";
            tx.operations.clear();
            tx.signatures.clear();
            tx.operations.push_back( op );
            sign( tx, alice_private_key );
            db->push_transaction( tx, 0 );
            alice_bob_vote = vote_idx.find( boost::make_tuple( new_bob_comment.id, new_alice.id, STEEM_SYMBOL ) );

            int64_t new_rshares = op.rshares - STEEM_VOTE_DUST_THRESHOLD;
            int64_t new_smt_rshares = op.smt_rshares[ alice_symbol ] - STEEM_VOTE_DUST_THRESHOLD;

            BOOST_REQUIRE( new_bob_comment.net_rshares == old_net_rshares - old_vote_rshares + new_rshares );
            BOOST_REQUIRE( new_bob_comment.abs_rshares == old_abs_rshares + new_rshares );
            BOOST_REQUIRE( new_bob_comment.cashout_time == new_bob_comment.created + STEEM_CASHOUT_WINDOW_SECONDS );
            BOOST_REQUIRE( alice_bob_vote->rshares == new_rshares );
            BOOST_REQUIRE( alice_bob_vote->last_update == db->head_block_time() );

            alice_bob_vote = vote_idx.find( boost::make_tuple( new_bob_comment.id, new_alice.id, alice_symbol ) );

            BOOST_REQUIRE( new_bob_comment.smt_rshares.find( alice_symbol )->second.net_rshares == old_smt_net_rshares - old_smt_vote_rshares + new_smt_rshares );
            BOOST_REQUIRE( new_bob_comment.smt_rshares.find( alice_symbol )->second.abs_rshares == old_smt_abs_rshares + new_smt_rshares );
            BOOST_REQUIRE( alice_bob_vote->rshares == new_smt_rshares );
            BOOST_REQUIRE( alice_bob_vote->last_update == db->head_block_time() );

            validate_database();
         }


         BOOST_TEST_MESSAGE( "--- Test decreasing vote rshares" );
         {
            generate_blocks( db->head_block_time() + STEEM_MIN_VOTE_INTERVAL_SEC );

            const auto& new_alice = db->get_account( "alice" );
            const auto& new_bob_comment = db->get_comment( "bob", string( "foo" ) );
            int64_t old_vote_rshares = op.rshares - STEEM_VOTE_DUST_THRESHOLD;
            auto old_net_rshares = new_bob_comment.net_rshares.value;
            auto old_abs_rshares = new_bob_comment.abs_rshares.value;

            int64_t old_smt_vote_rshares = op.smt_rshares[ alice_symbol ] - STEEM_VOTE_DUST_THRESHOLD;
            auto old_smt_net_rshares = new_bob_comment.smt_rshares.find( alice_symbol )->second.net_rshares.value;
            auto old_smt_abs_rshares = new_bob_comment.smt_rshares.find( alice_symbol )->second.abs_rshares.value;

            auto old_manabar = db->get_account( "alice" ).voting_manabar;
            params.max_mana = util::get_effective_vesting_shares( db->get_account( "alice" ) );
            old_manabar.regenerate_mana( params, db->head_block_time() );

            old_downvote_manabar = db->get_account( "alice" ).downvote_manabar;
            params.max_mana = util::get_effective_vesting_shares( db->get_account( "alice" ) ) / 4;
            old_downvote_manabar.regenerate_mana( params, db->head_block_time() );

            const auto& alice_smt = db->get< account_regular_balance_object, by_owner_liquid_symbol >( boost::make_tuple( "alice", alice_symbol ) );
            old_smt_manabar = alice_smt.voting_manabar;
            params.max_mana = util::get_effective_vesting_shares( alice_smt );
            old_smt_manabar.regenerate_mana( params, db->head_block_time() );

            old_smt_downvote_manabar = alice_smt.downvote_manabar;
            params.max_mana = util::get_effective_vesting_shares( alice_smt ) / 4;
            old_smt_downvote_manabar.regenerate_mana( params, db->head_block_time() );

            op.rshares = old_manabar.current_mana / 50 / 3;
            op.smt_rshares[ alice_symbol ] = old_smt_manabar.current_mana / 50 / 3;
            tx.operations.clear();
            tx.signatures.clear();
            tx.operations.push_back( op );
            sign( tx, alice_private_key );
            db->push_transaction( tx, 0 );
            auto alice_bob_vote = vote_idx.find( boost::make_tuple( new_bob_comment.id, new_alice.id, STEEM_SYMBOL ) );

            int64_t new_rshares = op.rshares - STEEM_VOTE_DUST_THRESHOLD;
            int64_t new_smt_rshares = op.smt_rshares[ alice_symbol ] - STEEM_VOTE_DUST_THRESHOLD;

            BOOST_REQUIRE( new_bob_comment.net_rshares == old_net_rshares - old_vote_rshares + new_rshares );
            BOOST_REQUIRE( new_bob_comment.abs_rshares == old_abs_rshares + new_rshares );
            BOOST_REQUIRE( new_bob_comment.cashout_time == new_bob_comment.created + STEEM_CASHOUT_WINDOW_SECONDS );
            BOOST_REQUIRE( alice_bob_vote->rshares == new_rshares );
            BOOST_REQUIRE( alice_bob_vote->last_update == db->head_block_time() );

            alice_bob_vote = vote_idx.find( boost::make_tuple( new_bob_comment.id, new_alice.id, alice_symbol ) );
            BOOST_REQUIRE( new_bob_comment.smt_rshares.find( alice_symbol )->second.net_rshares == old_smt_net_rshares - old_smt_vote_rshares + new_smt_rshares );
            BOOST_REQUIRE( new_bob_comment.smt_rshares.find( alice_symbol )->second.abs_rshares == old_smt_abs_rshares + new_smt_rshares );
            BOOST_REQUIRE( alice_bob_vote->rshares == new_smt_rshares );
            BOOST_REQUIRE( alice_bob_vote->last_update == db->head_block_time() );
            validate_database();
         }


         BOOST_TEST_MESSAGE( "--- Test changing a vote to 0 rshares (aka: removing a vote)" );
         {
            generate_blocks( db->head_block_time() + STEEM_MIN_VOTE_INTERVAL_SEC );

            const auto& new_alice = db->get_account( "alice" );
            const auto& new_bob_comment = db->get_comment( "bob", string( "foo" ) );
            auto old_net_rshares = new_bob_comment.net_rshares.value;
            auto old_abs_rshares = new_bob_comment.abs_rshares.value;
            int64_t old_vote_rshares = op.rshares - STEEM_VOTE_DUST_THRESHOLD;

            auto old_smt_net_rshares = new_bob_comment.smt_rshares.find( alice_symbol )->second.net_rshares.value;
            auto old_smt_abs_rshares = new_bob_comment.smt_rshares.find( alice_symbol )->second.abs_rshares.value;
            int64_t old_smt_vote_rshares = op.smt_rshares[ alice_symbol ] - STEEM_VOTE_DUST_THRESHOLD;

            op.rshares = 0;
            op.smt_rshares[ alice_symbol ] = 0;
            tx.operations.clear();
            tx.signatures.clear();
            tx.operations.push_back( op );
            sign( tx, alice_private_key );
            db->push_transaction( tx, 0 );
            auto alice_bob_vote = vote_idx.find( boost::make_tuple( new_bob_comment.id, new_alice.id, STEEM_SYMBOL ) );

            BOOST_REQUIRE( new_bob_comment.net_rshares == old_net_rshares - old_vote_rshares );
            BOOST_REQUIRE( new_bob_comment.abs_rshares == old_abs_rshares );
            BOOST_REQUIRE( new_bob_comment.cashout_time == new_bob_comment.created + STEEM_CASHOUT_WINDOW_SECONDS );
            BOOST_REQUIRE( alice_bob_vote->rshares == op.rshares );
            BOOST_REQUIRE( alice_bob_vote->last_update == db->head_block_time() );

            alice_bob_vote = vote_idx.find( boost::make_tuple( new_bob_comment.id, new_alice.id, alice_symbol ) );
            BOOST_REQUIRE( new_bob_comment.smt_rshares.find( alice_symbol )->second.net_rshares == old_smt_net_rshares - old_smt_vote_rshares );
            BOOST_REQUIRE( new_bob_comment.smt_rshares.find( alice_symbol )->second.abs_rshares == old_smt_abs_rshares );
            BOOST_REQUIRE( alice_bob_vote->rshares == op.smt_rshares[ alice_symbol ] );
            BOOST_REQUIRE( alice_bob_vote->last_update == db->head_block_time() );

            validate_database();
         }


         BOOST_TEST_MESSAGE( "--- Test downvote overlap when downvote mana is low" );
         {
            generate_block();

            db_plugin->debug_update( [=]( database& db )
            {
               db.modify( db.get_account( "alice" ), [&]( account_object& a )
               {
                  a.downvote_manabar.current_mana /= 30;
                  a.downvote_manabar.last_update_time = db.head_block_time().sec_since_epoch();
               });

               const auto& alice_smt = db.get< account_regular_balance_object, by_owner_liquid_symbol >( boost::make_tuple( "alice", alice_symbol ) );

               db.modify( alice_smt, [&]( account_regular_balance_object& a )
               {
                  a.downvote_manabar.current_mana /= 30;
                  a.downvote_manabar.last_update_time = db.head_block_time().sec_since_epoch();
               });
            });

            const auto& new_alice = db->get_account( "alice" );
            const auto& bob_comment = db->get_comment( "bob", string( "foo" ) );
            auto old_net_rshares = bob_comment.net_rshares.value;
            auto old_abs_rshares = bob_comment.abs_rshares.value;

            auto old_smt_net_rshares = bob_comment.smt_rshares.find( alice_symbol )->second.net_rshares.value;
            auto old_smt_abs_rshares = bob_comment.smt_rshares.find( alice_symbol )->second.abs_rshares.value;

            old_manabar = db->get_account( "alice" ).voting_manabar;
            params.max_mana = util::get_effective_vesting_shares( db->get_account( "alice" ) );
            old_manabar.regenerate_mana( params, db->head_block_time() );

            old_downvote_manabar = db->get_account( "alice" ).downvote_manabar;
            params.max_mana = util::get_effective_vesting_shares( db->get_account( "alice" ) ) / 4;
            old_downvote_manabar.regenerate_mana( params, db->head_block_time() );

            const auto& alice_smt = db->get< account_regular_balance_object, by_owner_liquid_symbol >( boost::make_tuple( "alice", alice_symbol ) );

            old_smt_manabar = alice_smt.voting_manabar;
            params.max_mana = util::get_effective_vesting_shares( alice_smt );
            old_smt_manabar.regenerate_mana( params, db->head_block_time() );

            old_smt_downvote_manabar = alice_smt.downvote_manabar;
            params.max_mana = util::get_effective_vesting_shares( alice_smt ) / 4;
            old_smt_downvote_manabar.regenerate_mana( params, db->head_block_time() );

            op.rshares = -1 * old_manabar.current_mana / 50;
            op.smt_rshares[ alice_symbol ] = -1 * old_smt_manabar.current_mana / 50;
            tx.operations.clear();
            tx.signatures.clear();
            tx.operations.push_back( op );
            sign( tx, alice_private_key );
            db->push_transaction( tx, 0 );
            auto alice_bob_vote = vote_idx.find( boost::make_tuple( bob_comment.id, new_alice.id, STEEM_SYMBOL ) );

            int64_t new_rshares = op.rshares + STEEM_VOTE_DUST_THRESHOLD;
            int64_t new_smt_rshares = op.smt_rshares[ alice_symbol ] + STEEM_VOTE_DUST_THRESHOLD;

            BOOST_REQUIRE( bob_comment.net_rshares == old_net_rshares + new_rshares );
            BOOST_REQUIRE( bob_comment.abs_rshares == old_abs_rshares - new_rshares );
            BOOST_REQUIRE( alice_bob_vote->rshares == new_rshares );
            BOOST_REQUIRE( alice_bob_vote->last_update == db->head_block_time() );
            BOOST_REQUIRE( alice.downvote_manabar.current_mana == 0 );
            BOOST_REQUIRE( alice.voting_manabar.current_mana == old_manabar.current_mana + op.rshares + old_downvote_manabar.current_mana );

            alice_bob_vote = vote_idx.find( boost::make_tuple( bob_comment.id, new_alice.id, alice_symbol ) );

            BOOST_REQUIRE( bob_comment.smt_rshares.find( alice_symbol )->second.net_rshares == old_smt_net_rshares + new_smt_rshares );
            BOOST_REQUIRE( bob_comment.smt_rshares.find( alice_symbol )->second.abs_rshares == old_smt_abs_rshares - new_smt_rshares );
            BOOST_REQUIRE( alice_bob_vote->rshares == new_smt_rshares );
            BOOST_REQUIRE( alice_bob_vote->last_update == db->head_block_time() );
            BOOST_REQUIRE( alice_smt.downvote_manabar.current_mana == 0 );
            BOOST_REQUIRE( alice_smt.voting_manabar.current_mana == old_smt_manabar.current_mana + op.smt_rshares[ alice_symbol ] + old_smt_downvote_manabar.current_mana );

            validate_database();
         }


         BOOST_TEST_MESSAGE( "--- Test reduced effectiveness when increasing rshares within lockout period" );
         {
            const auto& bob_comment = db->get_comment( "bob", string( "foo" ) );
            generate_blocks( fc::time_point_sec( ( bob_comment.cashout_time - STEEM_UPVOTE_LOCKOUT_HF17 ).sec_since_epoch() + STEEM_BLOCK_INTERVAL ), true );

            old_manabar = db->get_account( "dave" ).voting_manabar;
            params.max_mana = util::get_effective_vesting_shares( db->get_account( "dave" ) );
            old_manabar.regenerate_mana( params, db->head_block_time() );

            const auto& dave_smt = db->get< account_regular_balance_object, by_owner_liquid_symbol >( boost::make_tuple( "dave", alice_symbol ) );
            old_smt_manabar = dave_smt.voting_manabar;
            params.max_mana = util::get_effective_vesting_shares( dave_smt );
            old_smt_manabar.regenerate_mana( params, db->head_block_time() );

            op.voter = "dave";
            op.rshares = old_manabar.current_mana / 50;
            op.smt_rshares[ alice_symbol ] = old_smt_manabar.current_mana / 50;
            tx.operations.clear();
            tx.signatures.clear();
            tx.operations.push_back( op );
            sign( tx, dave_private_key );
            db->push_transaction( tx, 0 );

            int64_t new_rshares = op.rshares - STEEM_VOTE_DUST_THRESHOLD;
            new_rshares = ( new_rshares * ( STEEM_UPVOTE_LOCKOUT_SECONDS - STEEM_BLOCK_INTERVAL ) ) / STEEM_UPVOTE_LOCKOUT_SECONDS;

            int64_t new_smt_rshares = op.smt_rshares[ alice_symbol ] - STEEM_VOTE_DUST_THRESHOLD;
            new_smt_rshares = ( new_smt_rshares * ( STEEM_UPVOTE_LOCKOUT_SECONDS - STEEM_BLOCK_INTERVAL ) ) / STEEM_UPVOTE_LOCKOUT_SECONDS;

            account_id_type dave_id = db->get_account( "dave" ).id;
            comment_id_type bob_comment_id = db->get_comment( "bob", string( "foo" ) ).id;

            {
               auto dave_bob_vote = db->get< comment_vote_object, by_comment_voter_symbol >( boost::make_tuple( bob_comment_id, dave_id, STEEM_SYMBOL ) );
               BOOST_REQUIRE( dave_bob_vote.rshares = new_rshares );
            }

            {
               auto dave_bob_vote = db->get< comment_vote_object, by_comment_voter_symbol >( boost::make_tuple( bob_comment_id, dave_id, alice_symbol ) );
               BOOST_REQUIRE( dave_bob_vote.rshares = new_smt_rshares );
            }
            validate_database();
         }

         BOOST_TEST_MESSAGE( "--- Test reduced effectiveness when reducing rshares within lockout period" );
         {
            generate_block();
            old_manabar = db->get_account( "dave" ).voting_manabar;
            params.max_mana = util::get_effective_vesting_shares( db->get_account( "dave" ) );
            old_manabar.regenerate_mana( params, db->head_block_time() );

            const auto& dave_smt = db->get< account_regular_balance_object, by_owner_liquid_symbol >( boost::make_tuple( "dave", alice_symbol ) );
            old_smt_manabar = dave_smt.voting_manabar;
            params.max_mana = util::get_effective_vesting_shares( dave_smt );
            old_smt_manabar.regenerate_mana( params, db->head_block_time() );

            op.rshares = -1 * old_manabar.current_mana / 50;
            op.smt_rshares[ alice_symbol ] = -1 * old_smt_manabar.current_mana / 50;
            tx.operations.clear();
            tx.signatures.clear();
            tx.operations.push_back( op );
            sign( tx, dave_private_key );
            db->push_transaction( tx, 0 );

            int64_t new_rshares = op.rshares + STEEM_VOTE_DUST_THRESHOLD;
            new_rshares = ( new_rshares * ( STEEM_UPVOTE_LOCKOUT_SECONDS - STEEM_BLOCK_INTERVAL - STEEM_BLOCK_INTERVAL ) ) / STEEM_UPVOTE_LOCKOUT_SECONDS;

            int64_t new_smt_rshares = op.smt_rshares[ alice_symbol ] - STEEM_VOTE_DUST_THRESHOLD;
            new_smt_rshares = ( new_smt_rshares * ( STEEM_UPVOTE_LOCKOUT_SECONDS - STEEM_BLOCK_INTERVAL ) ) / STEEM_UPVOTE_LOCKOUT_SECONDS;

            account_id_type dave_id = db->get_account( "dave" ).id;
            comment_id_type bob_comment_id = db->get_comment( "bob", string( "foo" ) ).id;

            {
               auto dave_bob_vote = db->get< comment_vote_object, by_comment_voter_symbol >( boost::make_tuple( bob_comment_id, dave_id, STEEM_SYMBOL ) );
               BOOST_REQUIRE( dave_bob_vote.rshares = new_rshares );
            }

            {
               auto dave_bob_vote = db->get< comment_vote_object, by_comment_voter_symbol >( boost::make_tuple( bob_comment_id, dave_id, alice_symbol ) );
               BOOST_REQUIRE( dave_bob_vote.rshares = new_smt_rshares );
            }
            validate_database();
         }
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif
