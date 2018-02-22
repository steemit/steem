#include <fc/macros.hpp>

#if defined IS_TEST_NET && defined STEEM_ENABLE_SMT

#include <boost/test/unit_test.hpp>

#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/hardfork.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/database_exceptions.hpp>
#include <steem/chain/steem_objects.hpp>
#include <steem/chain/smt_objects.hpp>

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

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );

      BOOST_TEST_MESSAGE( "--- Test failure when no signature." );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test success with account signature" );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, database::skip_transaction_dupe_check );

      BOOST_TEST_MESSAGE( "--- Test failure with duplicate signature" );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure with additional incorrect signature" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test failure with incorrect signature" );
      tx.signatures.clear();
      tx.sign( alice_post_key, db->get_chain_id() );
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

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );

      BOOST_TEST_MESSAGE( "--- Test failure when no signature." );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test success with account signature" );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, database::skip_transaction_dupe_check );

      BOOST_TEST_MESSAGE( "--- Test failure with duplicate signature" );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure with additional incorrect signature" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test failure with incorrect signature" );
      tx.signatures.clear();
      tx.sign( alice_post_key, db->get_chain_id() );
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
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db->get_chain_id() );
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
      tx.sign( alice_private_key, db->get_chain_id() );
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
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( db->get_balance( bob_account, STEEM_SYMBOL ).amount.value == bob_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test success creating limit order that will not be filled" );

      op.amount_to_sell = ASSET( "10.000 TESTS" );
      op.min_to_receive = asset( 15000, alice_symbol );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
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
      tx.sign( alice_private_key, db->get_chain_id() );
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
      tx.sign( alice_private_key, db->get_chain_id() );
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
      tx.sign( bob_private_key, db->get_chain_id() );
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
      tx.sign( bob_private_key, db->get_chain_id() );
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
      tx.sign( alice_private_key, db->get_chain_id() );
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
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      op.owner = "bob";
      op.orderid = 4;
      op.amount_to_sell = asset( 12000, alice_symbol );
      op.min_to_receive = ASSET( "10.000 TESTS" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
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
      tx.sign( bob_private_key, db->get_chain_id() );
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
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      op.owner = "bob";
      op.orderid = 5;
      op.amount_to_sell = asset( 12000, alice_symbol );
      op.min_to_receive = ASSET( "10.000 TESTS" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
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

      tx.operations.push_back( c );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
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
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, database::skip_transaction_dupe_check );

      BOOST_TEST_MESSAGE( "--- Test failure with duplicate signature" );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure with additional incorrect signature" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test failure with incorrect signature" );
      tx.signatures.clear();
      tx.sign( alice_post_key, db->get_chain_id() );
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
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_TEST_MESSAGE( "--- Test cancel order" );

      limit_order_create_operation create;
      create.owner = "alice";
      create.orderid = 5;
      create.amount_to_sell = ASSET( "5.000 TESTS" );
      create.min_to_receive = asset( 7500, alice_symbol );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( create );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", 5 ) ) != limit_order_idx.end() );

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
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
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db->get_chain_id() );
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
      tx.sign( alice_private_key, db->get_chain_id() );
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
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( db->get_balance( bob_account, STEEM_SYMBOL ).amount.value == bob_balance.amount.value );
      BOOST_REQUIRE( db->get_balance( alice_account, alice_symbol ).amount.value == alice_smt_balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test success creating limit order that will not be filled" );

      op.amount_to_sell = ASSET( "10.000 TESTS" );
      op.exchange_rate = price( ASSET( "2.000 TESTS" ), asset( 3000, alice_symbol ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
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
      tx.sign( alice_private_key, db->get_chain_id() );
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
      tx.sign( alice_private_key, db->get_chain_id() );
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
      tx.sign( bob_private_key, db->get_chain_id() );
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
      tx.sign( bob_private_key, db->get_chain_id() );
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
      tx.sign( alice_private_key, db->get_chain_id() );
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
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      op.owner = "bob";
      op.orderid = 4;
      op.amount_to_sell = asset( 12000, alice_symbol );
      op.exchange_rate = price( asset( 1200, alice_symbol ), ASSET( "1.000 TESTS" ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
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
      tx.sign( bob_private_key, db->get_chain_id() );
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
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      op.owner = "bob";
      op.orderid = 5;
      op.amount_to_sell = asset( 12000, alice_symbol );
      op.exchange_rate = price( asset( 1200, alice_symbol ), ASSET( "1.000 TESTS" ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
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

BOOST_AUTO_TEST_SUITE_END()
#endif
