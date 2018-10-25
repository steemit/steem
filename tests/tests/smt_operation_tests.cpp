#include <fc/macros.hpp>

#if defined IS_TEST_NET && defined STEEM_ENABLE_SMT

#include <boost/test/unit_test.hpp>

#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/hardfork.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/database_exceptions.hpp>
#include <steem/chain/steem_objects.hpp>
#include <steem/chain/smt_objects.hpp>

#include <steem/chain/util/nai_generator.hpp>

#include "../db_fixture/database_fixture.hpp"

using namespace steem::chain;
using namespace steem::protocol;
using fc::string;
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

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "bob", op.orderid ) ) == limit_order_idx.end() );
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

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", op.orderid ) ) == limit_order_idx.end() );
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

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", op.orderid ) ) == limit_order_idx.end() );
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

      auto limit_order = limit_order_idx.find( std::make_tuple( "alice", op.orderid ) );
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

      limit_order = limit_order_idx.find( std::make_tuple( "alice", op.orderid ) );
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

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", op.orderid ) ) == limit_order_idx.end() );
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

      limit_order = limit_order_idx.find( std::make_tuple( "alice", 1 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == "alice" );
      BOOST_REQUIRE( limit_order->orderid == op.orderid );
      BOOST_REQUIRE( limit_order->for_sale == 5000 );
      BOOST_REQUIRE( limit_order->sell_price == price( ASSET( "10.000 TESTS" ), asset( 15000, alice_symbol ) ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( alice_symbol, STEEM_SYMBOL ) );
      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "bob", op.orderid ) ) == limit_order_idx.end() );
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

      limit_order = limit_order_idx.find( std::make_tuple( "bob", 1 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == "bob" );
      BOOST_REQUIRE( limit_order->orderid == 1 );
      BOOST_REQUIRE( limit_order->for_sale.value == 7500 );
      BOOST_REQUIRE( limit_order->sell_price == price( asset( 15000, alice_symbol ), ASSET( "10.000 TESTS" ) ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( alice_symbol, STEEM_SYMBOL ) );
      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", 1 ) ) == limit_order_idx.end() );
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

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", 3 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "bob", 1 ) ) == limit_order_idx.end() );
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

      limit_order = limit_order_idx.find( std::make_tuple( "bob", 4 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order_idx.find(std::make_tuple( "alice", 4 ) ) == limit_order_idx.end() );
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

      limit_order = limit_order_idx.find( std::make_tuple( "alice", 5 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order_idx.find(std::make_tuple( "bob", 5 ) ) == limit_order_idx.end() );
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

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", 5 ) ) != limit_order_idx.end() );

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      sign( tx, alice_private_key );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", 5 ) ) == limit_order_idx.end() );
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

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "bob", op.orderid ) ) == limit_order_idx.end() );
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

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", op.orderid ) ) == limit_order_idx.end() );
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

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", op.orderid ) ) == limit_order_idx.end() );
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

      auto limit_order = limit_order_idx.find( std::make_tuple( "alice", op.orderid ) );
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

      limit_order = limit_order_idx.find( std::make_tuple( "alice", op.orderid ) );
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

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", op.orderid ) ) == limit_order_idx.end() );
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

      limit_order = limit_order_idx.find( std::make_tuple( "alice", 1 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == "alice" );
      BOOST_REQUIRE( limit_order->orderid == op.orderid );
      BOOST_REQUIRE( limit_order->for_sale == 5000 );
      BOOST_REQUIRE( limit_order->sell_price == price( ASSET( "2.000 TESTS" ), asset( 3000, alice_symbol ) ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( alice_symbol, STEEM_SYMBOL ) );
      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "bob", op.orderid ) ) == limit_order_idx.end() );
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

      limit_order = limit_order_idx.find( std::make_tuple( "bob", 1 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == "bob" );
      BOOST_REQUIRE( limit_order->orderid == 1 );
      BOOST_REQUIRE( limit_order->for_sale.value == 7500 );
      BOOST_REQUIRE( limit_order->sell_price == price( asset( 3000, alice_symbol ), ASSET( "2.000 TESTS" ) ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( alice_symbol, STEEM_SYMBOL ) );
      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", 1 ) ) == limit_order_idx.end() );
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

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", 3 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "bob", 1 ) ) == limit_order_idx.end() );
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

      limit_order = limit_order_idx.find( std::make_tuple( "bob", 4 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order_idx.find(std::make_tuple( "alice", 4 ) ) == limit_order_idx.end() );
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

      limit_order = limit_order_idx.find( std::make_tuple( "alice", 5 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order_idx.find(std::make_tuple( "bob", 5 ) ) == limit_order_idx.end() );
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
   try
   {
      BOOST_TEST_MESSAGE( "Testing: transfer_to_vesting_validate with SMT" );

      // Dramatis personae
      ACTORS( (alice) )
      generate_block();
      // Create sample SMTs
      auto smts = create_smt_3( "alice", alice_private_key );
      const auto& smt1 = smts[0];
      // Fund creator with SMTs
      FUND( "alice", asset( 100, smt1 ) );

      transfer_to_vesting_operation op;
      op.from = "alice";
      op.amount = asset( 20, smt1 );
      op.validate();

      // Fail on invalid 'from' account name
      op.from = "@@@@@";
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.from = "alice";

      // Fail on invalid 'to' account name
      op.to = "@@@@@";
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.to = "";

      // Fail on vesting symbol (instead of liquid)
      op.amount = asset( 20, smt1.get_paired_symbol() );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.amount = asset( 20, smt1 );

      // Fail on 0 amount
      op.amount = asset( 0, smt1 );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.amount = asset( 20, smt1 );

      // Fail on negative amount
      op.amount = asset( -20, smt1 );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
      op.amount = asset( 20, smt1 );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

// Here would be smt_transfer_to_vesting_authorities if it differed from transfer_to_vesting_authorities

BOOST_AUTO_TEST_CASE( smt_transfer_to_vesting_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_transfer_to_vesting_apply" );

      auto test_single_smt = [this] (const asset_symbol_type& liquid_smt, const fc::ecc::private_key& key )
      {
         asset_symbol_type vesting_smt = liquid_smt.get_paired_symbol();
         // Fund creator with SMTs
         FUND( "alice", asset( 10000, liquid_smt ) );

         const auto& smt_object = db->get< smt_token_object, by_symbol >( liquid_smt );

         // Check pre-vesting balances
         FC_ASSERT( db->get_balance( "alice", liquid_smt ).amount == 10000, "SMT balance adjusting error" );
         FC_ASSERT( db->get_balance( "alice", vesting_smt ).amount == 0, "SMT balance adjusting error" );

         auto smt_shares = asset( smt_object.total_vesting_shares, vesting_smt );
         auto smt_vests = asset( smt_object.total_vesting_fund_smt, liquid_smt );
         auto smt_share_price = smt_object.get_vesting_share_price();
         auto alice_smt_shares = db->get_balance( "alice", vesting_smt );
         auto bob_smt_shares = db->get_balance( "bob", vesting_smt );;

         // Self transfer Alice's liquid
         transfer_to_vesting_operation op;
         op.from = "alice";
         op.to = "";
         op.amount = asset( 7500, liquid_smt );
         PUSH_OP( op, key );

         auto new_vest = op.amount * smt_share_price;
         smt_shares += new_vest;
         smt_vests += op.amount;
         alice_smt_shares += new_vest;

         BOOST_REQUIRE( db->get_balance( "alice", liquid_smt ) == asset( 2500, liquid_smt ) );
         BOOST_REQUIRE( db->get_balance( "alice", vesting_smt ) == alice_smt_shares );
         BOOST_REQUIRE( smt_object.total_vesting_fund_smt.value == smt_vests.amount.value );
         BOOST_REQUIRE( smt_object.total_vesting_shares.value == smt_shares.amount.value );
         validate_database();
         smt_share_price = smt_object.get_vesting_share_price();

         // Transfer Alice's liquid to Bob's vests
         op.to = "bob";
         op.amount = asset( 2000, liquid_smt );
         PUSH_OP( op, key );

         new_vest = op.amount * smt_share_price;
         smt_shares += new_vest;
         smt_vests += op.amount;
         bob_smt_shares += new_vest;

         BOOST_REQUIRE( db->get_balance( "alice", liquid_smt ) == asset( 500, liquid_smt ) );
         BOOST_REQUIRE( db->get_balance( "alice", vesting_smt ) == alice_smt_shares );
         BOOST_REQUIRE( db->get_balance( "bob", liquid_smt ) == asset( 0, liquid_smt ) );
         BOOST_REQUIRE( db->get_balance( "bob", vesting_smt ) == bob_smt_shares );
         BOOST_REQUIRE( smt_object.total_vesting_fund_smt.value == smt_vests.amount.value );
         BOOST_REQUIRE( smt_object.total_vesting_shares.value == smt_shares.amount.value );
         validate_database();
      };

      ACTORS( (alice)(bob) )
      generate_block();

      // Create sample SMTs
      auto smts = create_smt_3( "alice", alice_private_key );
      test_single_smt( smts[0], alice_private_key);
      test_single_smt( smts[1], alice_private_key );
      test_single_smt( smts[2], alice_private_key );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( smt_create_validate )
{
   try
   {
      ACTORS( (alice) );

      BOOST_TEST_MESSAGE( " -- A valid smt_create_operation" );
      smt_create_operation op;
      op.control_account = "alice";
      op.smt_creation_fee = ASSET( "1.000 TESTS" );
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
      op.smt_creation_fee = ASSET( "1.000 TESTS" );

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
      op.smt_creation_fee = ASSET( "1.000 TESTS" );

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

      BOOST_REQUIRE( ( db->find< smt_token_object, by_symbol >( op.symbol.to_nai() ) == nullptr ) );

      FUND( "alice", ASSET( "0.001 TESTS" ) );

      PUSH_OP( op, alice_private_key );

      BOOST_REQUIRE( ( db->find< smt_token_object, by_symbol >( op.symbol.to_nai() ) != nullptr ) );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( smt_create_with_sbd_funds )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: smt_create_with_sbd_funds" );

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

      BOOST_REQUIRE( ( db->find< smt_token_object, by_symbol >( op.symbol.to_nai() ) == nullptr ) );

      FUND( "alice", ASSET( "0.001 TBD" ) );

      PUSH_OP( op, alice_private_key );

      BOOST_REQUIRE( ( db->find< smt_token_object, by_symbol >( op.symbol.to_nai() ) != nullptr ) );
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

         ast = nai_generator::generate( seed++ );
      }
      while ( db->get< nai_pool_object >().contains( ast ) || db->find< smt_token_object, by_symbol >( ast.to_nai() ) );

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
         op.smt_creation_fee = ASSET( "1000.000 TBD" );
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

BOOST_AUTO_TEST_SUITE_END()
#endif
