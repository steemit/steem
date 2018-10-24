#if defined IS_TEST_NET
#include <boost/test/unit_test.hpp>
#include <steem/chain/account_object.hpp>
#include <steem/protocol/steem_operations.hpp>
#include <steem/protocol/config.hpp>
#include <steem/plugins/transaction_status/transaction_status_plugin.hpp>
#include <steem/plugins/transaction_status/transaction_status_objects.hpp>
#include <steem/plugins/transaction_status_api/transaction_status_api_plugin.hpp>
#include <steem/plugins/transaction_status_api/transaction_status_api.hpp>

#include "../db_fixture/database_fixture.hpp"

using namespace steem::chain;
using namespace steem::protocol;

#define TRANSCATION_STATUS_TRACK_AFTER_BLOCK 1300
#define TRANSCATION_STATUS_TRACK_AFTER_BLOCK_STR BOOST_PP_STRINGIZE( TRANSCATION_STATUS_TRACK_AFTER_BLOCK )
#define TRANSACTION_STATUS_TEST_BLOCK_DEPTH 30
#define TRANSACTION_STATUS_TEST_BLOCK_DEPTH_STR BOOST_PP_STRINGIZE( TRANSACTION_STATUS_TEST_BLOCK_DEPTH )

BOOST_FIXTURE_TEST_SUITE( transaction_status, database_fixture );

BOOST_AUTO_TEST_CASE( transaction_status_test )
{
   using namespace steem::plugins::transaction_status;

   try
   {
      int argc = boost::unit_test::framework::master_test_suite().argc;
      char** argv = boost::unit_test::framework::master_test_suite().argv;

      for ( int i = 1; i < argc; i++ )
      {
         const std::string arg = argv[ i ];
         if ( arg == "--record-assert-trip" )
            fc::enable_record_assert_trip = true;
         if ( arg == "--show-test-names" )
            std::cout << "running test " << boost::unit_test::framework::current_test_case().p_name << std::endl;
      }

      appbase::app().register_plugin< transaction_status_plugin >();
      appbase::app().register_plugin< steem::plugins::transaction_status_api::transaction_status_api_plugin >();
      db_plugin = &appbase::app().register_plugin< steem::plugins::debug_node::debug_node_plugin >();
      init_account_pub_key = init_account_priv_key.get_public_key();

      // We create an argc/argv so that the transaction_status plugin can be initialized with a reasonable block depth
      int test_argc = 5;
      const char* test_argv[] = { boost::unit_test::framework::master_test_suite().argv[0],
                                  "--transaction-status-block-depth",
                                  TRANSACTION_STATUS_TEST_BLOCK_DEPTH_STR,
                                  "--transaction-status-track-after-block",
                                  TRANSCATION_STATUS_TRACK_AFTER_BLOCK_STR };

      db_plugin->logging = false;
      appbase::app().initialize<
         steem::plugins::transaction_status_api::transaction_status_api_plugin,
         steem::plugins::debug_node::debug_node_plugin >( test_argc, (char**)test_argv );

      db = &appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db();
      BOOST_REQUIRE( db );

      auto tx_status_api = &appbase::app().get_plugin< steem::plugins::transaction_status_api::transaction_status_api_plugin >();
      BOOST_REQUIRE( tx_status_api );

      auto tx_status = &appbase::app().get_plugin< steem::plugins::transaction_status::transaction_status_plugin >();
      BOOST_REQUIRE( tx_status );

      open_database();

      BOOST_REQUIRE( db->get_index< transaction_status_index >().indices().get< by_id >().empty() );
      BOOST_REQUIRE( db->get_index< transaction_status_index >().indices().get< by_trx_id >().empty() );
      BOOST_REQUIRE( db->get_index< transaction_status_index >().indices().get< by_block_num >().empty() );

      BOOST_REQUIRE( tx_status->state_is_valid() );

      generate_block();
      db->set_hardfork( STEEM_NUM_HARDFORKS );
      generate_block();

      vest( "initminer", 10000 );

      // Fill up the rest of the required miners
      for( int i = STEEM_NUM_INIT_MINERS; i < STEEM_MAX_WITNESSES; i++ )
      {
         account_create( STEEM_INIT_MINER_NAME + fc::to_string( i ), init_account_pub_key );
         fund( STEEM_INIT_MINER_NAME + fc::to_string( i ), STEEM_MIN_PRODUCER_REWARD.amount.value );
         witness_create( STEEM_INIT_MINER_NAME + fc::to_string( i ), init_account_priv_key, "foo.bar", init_account_pub_key, STEEM_MIN_PRODUCER_REWARD.amount );
      }

      validate_database();

      ACTORS( (alice)(bob) );
      generate_block();

      fund( "alice", ASSET( "1000.000 TESTS" ) );
      fund( "alice", ASSET( "1000.000 TBD" ) );
      fund( "bob", ASSET( "1000.000 TESTS" ) );

      generate_block();

      validate_database();

      BOOST_TEST_MESSAGE(" -- transaction status tracking test" );

      signed_transaction tx0;
      transfer_operation op0;
      auto tx0_expiration = db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION;

      op0.from = "alice";
      op0.to = "bob";
      op0.amount = ASSET( "5.000 TESTS" );

      // Create transaction 0
      tx0.operations.push_back( op0 );
      tx0.set_expiration( tx0_expiration );
      sign( tx0, alice_private_key );
      db->push_transaction( tx0, 0 );

      // Tracking should not be enabled until we have reached TRANSCATION_STATUS_TRACK_AFTER_BLOCK - ( STEEM_MAX_TIME_UNTIL_EXPIRATION / STEEM_BLOCK_INTERVAL ) blocks
      BOOST_REQUIRE( db->get_index< transaction_status_index >().indices().get< by_id >().empty() );
      BOOST_REQUIRE( db->get_index< transaction_status_index >().indices().get< by_trx_id >().empty() );
      BOOST_REQUIRE( db->get_index< transaction_status_index >().indices().get< by_block_num >().empty() );

      // Transaction 0 should not be tracked
      auto tso = db->find< transaction_status_object, by_trx_id >( tx0.id() );
      BOOST_REQUIRE( tso == nullptr );

      auto api_return = tx_status_api->api->find_transaction( { .transaction_id = tx0.id() } );
      BOOST_REQUIRE( api_return.status == unknown );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx0.id(), tx0_expiration  } );
      BOOST_REQUIRE( api_return.status == unknown );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      generate_blocks( TRANSCATION_STATUS_TRACK_AFTER_BLOCK - db->head_block_num() );

      signed_transaction tx1;
      transfer_operation op1;
      auto tx1_expiration = db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION;

      op1.from = "alice";
      op1.to = "bob";
      op1.amount = ASSET( "5.000 TESTS" );

      // Create transaction 1
      tx1.operations.push_back( op1 );
      tx1.set_expiration( tx1_expiration );
      sign( tx1, alice_private_key );
      db->push_transaction( tx1, 0 );

      // Transaction 1 exists in the mem pool
      tso = db->find< transaction_status_object, by_trx_id >( tx1.id() );
      BOOST_REQUIRE( tso != nullptr );
      BOOST_REQUIRE( tso->block_num == 0 );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx1.id() } );
      BOOST_REQUIRE( api_return.status == within_mempool );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx1.id(), tx1_expiration } );
      BOOST_REQUIRE( api_return.status == within_mempool );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      generate_block();

      /*
       * Test for two transactions in the same block
       */

      // Create transaction 2
      signed_transaction tx2;
      transfer_operation op2;
      auto tx2_expiration = db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION;

      op2.from = "alice";
      op2.to = "bob";
      op2.amount = ASSET( "5.000 TESTS" );

      tx2.operations.push_back( op2 );
      tx2.set_expiration( tx2_expiration );
      sign( tx2, alice_private_key );
      db->push_transaction( tx2, 0 );

      // Create transaction 3
      signed_transaction tx3;
      transfer_operation op3;
      auto tx3_expiration = db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION;

      op3.from = "bob";
      op3.to = "alice";
      op3.amount = ASSET( "5.000 TESTS" );

      tx3.operations.push_back( op3 );
      tx3.set_expiration( tx3_expiration );
      sign( tx3, bob_private_key );
      db->push_transaction( tx3, 0 );

      // Transaction 1 exists in a block
      tso = db->find< transaction_status_object, by_trx_id >( tx1.id() );
      BOOST_REQUIRE( tso != nullptr );
      BOOST_REQUIRE( tso->block_num == db->head_block_num() );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx1.id() } );
      BOOST_REQUIRE( api_return.status == within_reversible_block );
      BOOST_REQUIRE( api_return.block_num.valid() );
      BOOST_REQUIRE( api_return.block_num == db->head_block_num() );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx1.id(), tx1_expiration } );
      BOOST_REQUIRE( api_return.status == within_reversible_block );
      BOOST_REQUIRE( api_return.block_num.valid() );
      BOOST_REQUIRE( api_return.block_num == db->head_block_num() );

      // Transaction 2 exists in a mem pool
      tso = db->find< transaction_status_object, by_trx_id >( tx2.id() );
      BOOST_REQUIRE( tso != nullptr );
      BOOST_REQUIRE( tso->block_num == 0 );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx2.id() } );
      BOOST_REQUIRE( api_return.status == within_mempool );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx2.id(), tx2_expiration } );
      BOOST_REQUIRE( api_return.status == within_mempool );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      // Transaction 3 exists in a mem pool
      tso = db->find< transaction_status_object, by_trx_id >( tx3.id() );
      BOOST_REQUIRE( tso != nullptr );
      BOOST_REQUIRE( tso->block_num == 0 );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx3.id() } );
      BOOST_REQUIRE( api_return.status == within_mempool );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx3.id(), tx3_expiration } );
      BOOST_REQUIRE( api_return.status == within_mempool );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      BOOST_REQUIRE( tx_status->state_is_valid() );

      generate_blocks( TRANSACTION_STATUS_TEST_BLOCK_DEPTH );

      // Transaction 1 exists in a block
      tso = db->find< transaction_status_object, by_trx_id >( tx1.id() );
      BOOST_REQUIRE( tso != nullptr );
      BOOST_REQUIRE( tso->block_num > 0 );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx1.id() } );
      BOOST_REQUIRE( api_return.status == within_irreversible_block );
      BOOST_REQUIRE( *api_return.block_num > 0 );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx1.id(), tx1_expiration } );
      BOOST_REQUIRE( api_return.status == within_irreversible_block );
      BOOST_REQUIRE( *api_return.block_num > 0 );

      // Transaction 2 exists in a block
      tso = db->find< transaction_status_object, by_trx_id >( tx2.id() );
      BOOST_REQUIRE( tso != nullptr );
      BOOST_REQUIRE( tso->block_num > 0 );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx2.id() } );
      BOOST_REQUIRE( api_return.status == within_irreversible_block );
      BOOST_REQUIRE( api_return.block_num.valid() );
      BOOST_REQUIRE( *api_return.block_num > 0 );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx2.id(), tx2_expiration } );
      BOOST_REQUIRE( api_return.status == within_irreversible_block );
      BOOST_REQUIRE( api_return.block_num.valid() );
      BOOST_REQUIRE( *api_return.block_num > 0 );

      // Transaction 3 exists in a block
      tso = db->find< transaction_status_object, by_trx_id >( tx3.id() );
      BOOST_REQUIRE( tso != nullptr );
      BOOST_REQUIRE( tso->block_num > 0 );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx3.id() } );
      BOOST_REQUIRE( api_return.status == within_irreversible_block );
      BOOST_REQUIRE( api_return.block_num.valid() );
      BOOST_REQUIRE( *api_return.block_num > 0 );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx3.id(), tx3_expiration } );
      BOOST_REQUIRE( api_return.status == within_irreversible_block );
      BOOST_REQUIRE( api_return.block_num.valid() );
      BOOST_REQUIRE( *api_return.block_num > 0 );

      BOOST_REQUIRE( tx_status->state_is_valid() );

      generate_blocks( STEEM_MAX_TIME_UNTIL_EXPIRATION / STEEM_BLOCK_INTERVAL );

      // Transaction 1 is no longer tracked
      tso = db->find< transaction_status_object, by_trx_id >( tx1.id() );
      BOOST_REQUIRE( tso == nullptr );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx1.id() } );
      BOOST_REQUIRE( api_return.status == unknown );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx1.id(), tx1_expiration } );
      BOOST_REQUIRE( api_return.status == too_old );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      // Transaction 2 exists in a block
      tso = db->find< transaction_status_object, by_trx_id >( tx2.id() );
      BOOST_REQUIRE( tso != nullptr );
      BOOST_REQUIRE( tso->block_num > 0 );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx2.id() } );
      BOOST_REQUIRE( api_return.status == within_irreversible_block );
      BOOST_REQUIRE( api_return.block_num.valid() );
      BOOST_REQUIRE( *api_return.block_num > 0 );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx2.id(), tx2_expiration } );
      BOOST_REQUIRE( api_return.status == within_irreversible_block );
      BOOST_REQUIRE( api_return.block_num.valid() );
      BOOST_REQUIRE( *api_return.block_num > 0 );

      // Transaction 3 exists in a block
      tso = db->find< transaction_status_object, by_trx_id >( tx3.id() );
      BOOST_REQUIRE( tso != nullptr );
      BOOST_REQUIRE( tso->block_num > 0 );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx3.id() } );
      BOOST_REQUIRE( api_return.status == within_irreversible_block );
      BOOST_REQUIRE( api_return.block_num.valid() );
      BOOST_REQUIRE( *api_return.block_num > 0 );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx3.id(), tx3_expiration } );
      BOOST_REQUIRE( api_return.status == within_irreversible_block );
      BOOST_REQUIRE( api_return.block_num.valid() );
      BOOST_REQUIRE( *api_return.block_num > 0 );

      BOOST_REQUIRE( tx_status->state_is_valid() );

      generate_block();

      // Transaction 2 is no longer tracked
      tso = db->find< transaction_status_object, by_trx_id >( tx2.id() );
      BOOST_REQUIRE( tso == nullptr );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx2.id() } );
      BOOST_REQUIRE( api_return.status == unknown );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx2.id(), tx2_expiration } );
      BOOST_REQUIRE( api_return.status == too_old );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      // Transaction 3 is no longer tracked
      tso = db->find< transaction_status_object, by_trx_id >( tx3.id() );
      BOOST_REQUIRE( tso == nullptr );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx3.id() } );
      BOOST_REQUIRE( api_return.status == unknown );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx3.id(), tx3_expiration } );
      BOOST_REQUIRE( api_return.status == too_old );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      // At this point our index should be empty
      BOOST_REQUIRE( db->get_index< transaction_status_index >().indices().get< by_id >().empty() );
      BOOST_REQUIRE( db->get_index< transaction_status_index >().indices().get< by_trx_id >().empty() );
      BOOST_REQUIRE( db->get_index< transaction_status_index >().indices().get< by_block_num >().empty() );

      BOOST_REQUIRE( tx_status->state_is_valid() );

      generate_block();

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx1.id() } );
      BOOST_REQUIRE( api_return.status == unknown );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx1.id(), tx1_expiration } );
      BOOST_REQUIRE( api_return.status == too_old );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx2.id() } );
      BOOST_REQUIRE( api_return.status == unknown );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      api_return = tx_status_api->api->find_transaction( { .transaction_id = tx2.id(), tx2_expiration } );
      BOOST_REQUIRE( api_return.status == too_old );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      /**
       * Testing transactions that do not exist, but expirations are provided
       */
      BOOST_TEST_MESSAGE( " -- transaction status expiration test" );

      // The time of our last irreversible block
      auto lib_time = db->fetch_block_by_number( db->get_dynamic_global_properties().last_irreversible_block_num )->timestamp;
      api_return = tx_status_api->api->find_transaction( { .transaction_id = transaction_id_type(), .expiration = lib_time } );
      BOOST_REQUIRE( api_return.status == expired_irreversible );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      // One second after our last irreversible block
      auto after_lib_time = lib_time + fc::seconds(1);
      api_return = tx_status_api->api->find_transaction( { .transaction_id = transaction_id_type(), after_lib_time } );
      BOOST_REQUIRE( api_return.status == expired_reversible );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      // One second before our block depth
      auto old_time = db->fetch_block_by_number( db->head_block_num() - TRANSACTION_STATUS_TEST_BLOCK_DEPTH + 1 )->timestamp - fc::seconds(1);
      api_return = tx_status_api->api->find_transaction( { .transaction_id = transaction_id_type(), old_time } );
      BOOST_REQUIRE( api_return.status == too_old );
      BOOST_REQUIRE( api_return.block_num.valid() == false );

      BOOST_REQUIRE( tx_status->state_is_valid() );

      /**
       * Testing transaction status plugin state
       */
      BOOST_TEST_MESSAGE( " -- transaction status state test" );

      // Create transaction 4
      signed_transaction tx4;
      transfer_operation op4;
      auto tx4_expiration = db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION;

      op4.from = "alice";
      op4.to = "bob";
      op4.amount = ASSET( "5.000 TESTS" );

      tx4.operations.push_back( op4 );
      tx4.set_expiration( tx4_expiration );
      sign( tx4, alice_private_key );
      db->push_transaction( tx4, 0 );

      generate_block();

      BOOST_REQUIRE( tx_status->state_is_valid() );

      const auto& tx_status_obj = db->get< transaction_status_object, by_trx_id >( tx4.id() );
      db->remove( tx_status_obj );

      // Upper bound of transaction status state should cause state to be invalid
      BOOST_REQUIRE( tx_status->state_is_valid() == false );

      tx_status->rebuild_state();

      BOOST_REQUIRE( tx_status->state_is_valid() );

      // Create transaction 5
      signed_transaction tx5;
      transfer_operation op5;
      auto tx5_expiration = db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION;

      op5.from = "alice";
      op5.to = "bob";
      op5.amount = ASSET( "5.000 TESTS" );

      tx5.operations.push_back( op5 );
      tx5.set_expiration( tx5_expiration );
      sign( tx5, alice_private_key );
      db->push_transaction( tx5, 0 );

      generate_blocks( TRANSACTION_STATUS_TEST_BLOCK_DEPTH + ( STEEM_MAX_TIME_UNTIL_EXPIRATION / STEEM_BLOCK_INTERVAL ) - 1 );

      const auto& tx_status_obj2 = db->get< transaction_status_object, by_trx_id >( tx5.id() );
      db->remove( tx_status_obj2 );

      // Lower bound of transaction status state should cause state to be invalid
      BOOST_REQUIRE( tx_status->state_is_valid() == false );

      tx_status->rebuild_state();

      BOOST_REQUIRE( tx_status->state_is_valid() );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif

