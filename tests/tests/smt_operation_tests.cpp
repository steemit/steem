#include <fc/macros.hpp>

#if defined IS_TEST_NET && defined STEEM_ENABLE_SMT

#include <boost/test/unit_test.hpp>

#include <steem/chain/steem_fwd.hpp>

#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/hardfork.hpp>

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
/*
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
*/

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
      op.emissions_unit.token_unit[ "alice" ] = 10;
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
      op.emissions_unit.token_unit[ "alice" ] = 1;
      op.emissions_unit.token_unit[ "$rewards" ] = 1;
      op.emissions_unit.token_unit[ "$market_maker" ] = 1;
      op.emissions_unit.token_unit[ "$vesting" ] = 1;

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
      op.emissions_unit.token_unit[ "alice" ] = 10;
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
         obj.phase = smt_phase::account_elevated;
      } );
      PUSH_OP( op, alice_private_key );

      BOOST_TEST_MESSAGE( " -- Emissions range is overlapping" );
      smt_setup_emissions_operation op2;
      op2.control_account = "alice";
      op2.symbol = alice_symbol;
      op2.emissions_unit.token_unit[ "alice" ] = 10;
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
      op3.emissions_unit.token_unit[ "alice" ] = 10;
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
      op4.emissions_unit.token_unit[ "alice" ] = 10;
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

BOOST_AUTO_TEST_SUITE_END()
#endif
