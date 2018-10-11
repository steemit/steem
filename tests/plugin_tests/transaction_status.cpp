#if defined IS_TEST_NET
#include <boost/test/unit_test.hpp>
#include <steem/chain/account_object.hpp>
#include <steem/protocol/steem_operations.hpp>
#include <steem/plugins/transaction_status/transaction_status_plugin.hpp>
#include <steem/plugins/transaction_status/transaction_status_objects.hpp>

#include "../db_fixture/database_fixture.hpp"

using namespace steem::chain;
using namespace steem::protocol;

#define TRANSACTION_STATUS_TEST_BLOCK_DEPTH 10
#define TRANSACTION_STATUS_TEST_BLOCK_DEPTH_STR BOOST_PP_STRINGIZE( TRANSACTION_STATUS_TEST_BLOCK_DEPTH )

BOOST_FIXTURE_TEST_SUITE( transaction_status, database_fixture )

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
      db_plugin = &appbase::app().register_plugin< steem::plugins::debug_node::debug_node_plugin >();
      init_account_pub_key = init_account_priv_key.get_public_key();

      // We create an argc/argv so that the transaction_status plugin can be initialized with a reasonable block depth
      int test_argc = 3;
      const char* test_argv[] = { boost::unit_test::framework::master_test_suite().argv[0], "--transaction-status-block-depth", TRANSACTION_STATUS_TEST_BLOCK_DEPTH_STR };
      db_plugin->logging = false;
      appbase::app().initialize<
         steem::plugins::transaction_status::transaction_status_plugin,
         steem::plugins::debug_node::debug_node_plugin >( test_argc, (char**)test_argv );

      db = &appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db();
      BOOST_REQUIRE( db );

      open_database();

      BOOST_REQUIRE( db->get_index< transaction_status_index >().indices().get< by_id >().empty() );
      BOOST_REQUIRE( db->get_index< transaction_status_index >().indices().get< by_trx_id >().empty() );
      BOOST_REQUIRE( db->get_index< transaction_status_index >().indices().get< by_block_num >().empty() );

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

      signed_transaction tx1;
      transfer_operation op1;

      op1.from = "alice";
      op1.to = "bob";
      op1.amount = ASSET( "5.000 TESTS" );

      // Create transaction 1
      tx1.operations.push_back( op1 );
      tx1.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx1, alice_private_key );
      db->push_transaction( tx1, 0 );

      // Transaction 1 exists in the mem pool
      auto tso = db->find< transaction_status_object, by_trx_id >( tx1.id() );
      BOOST_REQUIRE( tso != nullptr );
      BOOST_REQUIRE( tso->block_num == 0 );

      generate_block();

      signed_transaction tx2;
      transfer_operation op2;

      op2.from = "alice";
      op2.to = "bob";
      op2.amount = ASSET( "5.000 TESTS" );

      // Create transaction 2
      tx2.operations.push_back( op2 );
      tx2.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx2, alice_private_key );
      db->push_transaction( tx2, 0 );

      // Transaction 1 exists in a block
      tso = db->find< transaction_status_object, by_trx_id >( tx1.id() );
      BOOST_REQUIRE( tso != nullptr );
      BOOST_REQUIRE( tso->block_num == db->head_block_num() );

      // Transaction 2 exists in a mem pool
      tso = db->find< transaction_status_object, by_trx_id >( tx2.id() );
      BOOST_REQUIRE( tso != nullptr );
      BOOST_REQUIRE( tso->block_num == 0 );

      generate_blocks( TRANSACTION_STATUS_TEST_BLOCK_DEPTH );

      // Transaction 1 is too old to be tracked
      tso = db->find< transaction_status_object, by_trx_id >( tx1.id() );
      BOOST_REQUIRE( tso == nullptr );

      // Transaction 2 exists in a block
      tso = db->find< transaction_status_object, by_trx_id >( tx2.id() );
      BOOST_REQUIRE( tso != nullptr );
      BOOST_REQUIRE( tso->block_num > 0 );

      generate_block();

      // Transaction 2 is too old to be tracked
      tso = db->find< transaction_status_object, by_trx_id >( tx2.id() );
      BOOST_REQUIRE( tso == nullptr );

      BOOST_REQUIRE( db->get_index< transaction_status_index >().indices().get< by_id >().empty() );
      BOOST_REQUIRE( db->get_index< transaction_status_index >().indices().get< by_trx_id >().empty() );
      BOOST_REQUIRE( db->get_index< transaction_status_index >().indices().get< by_block_num >().empty() );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif

