#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <steem/protocol/exceptions.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/steem_objects.hpp>

#include <steem/plugins/fork_monitor/fork_monitor_plugin.hpp>

#include <steem/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>

#include "../db_fixture/database_fixture.hpp"


using namespace steem;
using namespace steem::chain;
using namespace steem::protocol;

#define TEST_SHARED_MEM_SIZE (1024 * 1024 * 8)

BOOST_AUTO_TEST_SUITE(fork_monitor)

BOOST_FIXTURE_TEST_CASE( fork_monitor_blocks, clean_database_fixture )
{
   using namespace steem::plugins::fork_monitor;

   try {

     int argc = boost::unit_test::framework::master_test_suite().argc;
     char** argv = boost::unit_test::framework::master_test_suite().argv;
     for( int i=1; i<argc; i++ )
     {
        const std::string arg = argv[i];
        if( arg == "--record-assert-trip" )
           fc::enable_record_assert_trip = true;
        if( arg == "--show-test-names" )
           std::cout << "running test " << boost::unit_test::framework::current_test_case().p_name << std::endl;
     }

      plugins::fork_monitor::fork_monitor_plugin* fm_plugin = &appbase::app().register_plugin< fork_monitor_plugin >();
      appbase::app().initialize< fork_monitor_plugin >( argc, argv );

      fc::temp_directory data_dir2( steem::utilities::temp_directory_path() );

      auto init_account_priv_key  = fc::ecc::private_key::regenerate(fc::sha256::hash(string("init_key")) );
      public_key_type init_account_pub_key = init_account_priv_key.get_public_key();

      database db2;
      db2._log_hardforks = false;
      db2.open( data_dir2.path(), data_dir2.path(), INITIAL_TEST_SUPPLY, 1024 * 1024 * 8 );

      db2.generate_block( db2.get_slot_time(1), db2.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing );
      db2.set_hardfork( STEEM_BLOCKCHAIN_VERSION.minor() );
      db2.generate_block( db2.get_slot_time(1), db2.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing );
      signed_transaction trx;

      for( int i = STEEM_NUM_INIT_MINERS; i < STEEM_MAX_WITNESSES; i++ )
      {
          account_create_with_delegation_operation aop;
          aop.new_account_name = STEEM_INIT_MINER_NAME + fc::to_string( i );
          aop.creator = STEEM_INIT_MINER_NAME;
          aop.fee = std::max( db2.get_witness_schedule_object().median_props.account_creation_fee.amount * STEEM_CREATE_ACCOUNT_WITH_STEEM_MODIFIER, share_type( 100 ) ),
          aop.delegation = asset( 0, VESTS_SYMBOL );
          aop.owner = authority( 1, init_account_pub_key, 1 );
          aop.active = authority( 1, init_account_pub_key, 1 );
          aop.posting = authority( 1, init_account_pub_key, 1 );
          aop.memo_key = init_account_pub_key;
          aop.json_metadata = "";

          trx.operations.push_back( aop );
          trx.set_expiration( db2.head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
          trx.sign( init_account_priv_key, db2.get_chain_id() );
          trx.validate();
          db2.push_transaction( trx, 0 );
          trx.operations.clear();
          trx.signatures.clear();

          witness_update_operation wop;
          wop.owner = STEEM_INIT_MINER_NAME + fc::to_string( i );
          wop.url = "foo.bar";
          wop.block_signing_key = init_account_pub_key;
          wop.fee = asset( STEEM_MIN_PRODUCER_REWARD.amount, STEEM_SYMBOL );

          trx.operations.push_back( wop );
          trx.set_expiration( db2.head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
          trx.sign( init_account_priv_key, db2.get_chain_id() );
          trx.validate();
          db2.push_transaction( trx, 0 );
          trx.operations.clear();
          trx.signatures.clear();
      }

      db->generate_block(db->get_slot_time(1), db->get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
      db2.generate_block(db2.get_slot_time(1), db2.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);

      for( uint32_t i = 3; i < 63; ++i )
      {
         auto b = db->generate_block(db->get_slot_time(1), db->get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
         try {
            PUSH_BLOCK( db2, b );
         } FC_CAPTURE_AND_RETHROW( ("db2") );
      }
      // db and db2 should be equal now.
      BOOST_CHECK_EQUAL(db->head_block_id().str(), db2.head_block_id().str());

      transfer_operation top;
      top.from = "initminer";
      top.to = STEEM_INIT_MINER_NAME + fc::to_string( 1 );
      top.amount = share_type(STEEM_MIN_PRODUCER_REWARD.amount.value);

      trx.operations.push_back( top );
      trx.set_expiration( db2.head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      trx.sign( init_account_priv_key, db2.get_chain_id() );
      trx.validate();
      db2.push_transaction( trx, 0 );
      trx.operations.clear();
      trx.signatures.clear();

      auto good_block1 =  db2.generate_block(db2.get_slot_time(1), db2.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
      auto good_block2 =  db2.generate_block(db2.get_slot_time(1), db2.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
      auto good_block3 =  db2.generate_block(db2.get_slot_time(1), db2.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
      auto bad_block1  =  db->generate_block(db->get_slot_time(1), db->get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
      auto bad_block2  =  db->generate_block(db->get_slot_time(1), db->get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
      db->push_block(good_block1, 0);
      db->push_block(good_block2, 0);
      db->push_block(good_block3, 0);

      BOOST_CHECK( db->head_block_id() == good_block3.id() );
      std::map< uint32_t, std::vector< signed_block > > the_map = fm_plugin->get_orphans();
      std::vector< steem::protocol::signed_block > vec_blocks = the_map[good_block1.block_num()];
      BOOST_CHECK( vec_blocks.size() > 1 );
      steem::protocol::signed_block bad_block = vec_blocks[0];
      BOOST_CHECK( bad_block.id() == bad_block1.id() );
      steem::protocol::signed_block good_block = vec_blocks[1];
      BOOST_CHECK( good_block.id() == good_block1.id() );
      vec_blocks = the_map[good_block2.block_num()];
      BOOST_CHECK( vec_blocks.size() > 1 );
      bad_block = vec_blocks[0];
      BOOST_CHECK( bad_block.id() == bad_block2.id() );
      good_block = vec_blocks[1];
      BOOST_CHECK( good_block.id() == good_block2.id() );

      for( uint32_t i = 63; i < 84; ++i )
      {
         db->generate_block(db->get_slot_time(1), db->get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
      }
   } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}
BOOST_AUTO_TEST_SUITE_END()
#endif
