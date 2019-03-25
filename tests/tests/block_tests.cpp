/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <steem/chain/steem_fwd.hpp>

#include <steem/protocol/exceptions.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/steem_objects.hpp>
#include <steem/chain/history_object.hpp>

#include <steem/plugins/account_history/account_history_plugin.hpp>
#include <steem/plugins/witness/block_producer.hpp>

#include <steem/utilities/tempdir.hpp>
#include <steem/utilities/database_configuration.hpp>

#include <fc/crypto/digest.hpp>

#include "../db_fixture/database_fixture.hpp"

using namespace steem;
using namespace steem::chain;
using namespace steem::protocol;
using namespace steem::plugins;

#define TEST_SHARED_MEM_SIZE (1024 * 1024 * 8)

BOOST_AUTO_TEST_SUITE(block_tests)

void open_test_database( database& db, const fc::path& dir )
{
   database::open_args args;
   args.data_dir = dir;
   args.shared_mem_dir = dir;
   args.initial_supply = INITIAL_TEST_SUPPLY;
   args.shared_file_size = TEST_SHARED_MEM_SIZE;
   args.database_cfg = steem::utilities::default_database_configuration();
   db.open( args );
}

BOOST_AUTO_TEST_CASE( generate_empty_blocks )
{
   try {
      fc::time_point_sec now( STEEM_TESTING_GENESIS_TIMESTAMP );
      fc::temp_directory data_dir( steem::utilities::temp_directory_path() );
      signed_block b;

      // TODO:  Don't generate this here
      auto init_account_priv_key = fc::ecc::private_key::regenerate( fc::sha256::hash( string( "init_key" ) ) );
      signed_block cutoff_block;
      {
         database db;
         witness::block_producer bp( db );
         db._log_hardforks = false;
         open_test_database( db, data_dir.path() );
         b = bp.generate_block(db.get_slot_time(1), db.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);

         // TODO:  Change this test when we correct #406
         // n.b. we generate STEEM_MIN_UNDO_HISTORY+1 extra blocks which will be discarded on save
         for( uint32_t i = 1; ; ++i )
         {
            BOOST_CHECK( db.head_block_id() == b.id() );
            //witness_id_type prev_witness = b.witness;
            string cur_witness = db.get_scheduled_witness(1);
            //BOOST_CHECK( cur_witness != prev_witness );
            b = bp.generate_block(db.get_slot_time(1), cur_witness, init_account_priv_key, database::skip_nothing);
            BOOST_CHECK( b.witness == cur_witness );
            uint32_t cutoff_height = db.get_dynamic_global_properties().last_irreversible_block_num;
            if( cutoff_height >= 200 )
            {
               auto block = db.fetch_block_by_number( cutoff_height );
               BOOST_REQUIRE( block.valid() );
               cutoff_block = *block;
               break;
            }
         }
         db.close();
      }
      {
         database db;
         witness::block_producer bp( db );
         db._log_hardforks = false;
         open_test_database( db, data_dir.path() );

#ifndef ENABLE_STD_ALLOCATOR
         BOOST_CHECK_EQUAL( db.head_block_num(), cutoff_block.block_num() );
#endif

         b = cutoff_block;
         for( uint32_t i = 0; i < 200; ++i )
         {
#ifndef ENABLE_STD_ALLOCATOR
            BOOST_CHECK( db.head_block_id() == b.id() );
#else
            BOOST_CHECK( i==0 || ( db.head_block_id() == b.id() ) );
#endif
            //witness_id_type prev_witness = b.witness;
            string cur_witness = db.get_scheduled_witness(1);
            //BOOST_CHECK( cur_witness != prev_witness );
            b = bp.generate_block(db.get_slot_time(1), cur_witness, init_account_priv_key, database::skip_nothing);
         }
#ifndef ENABLE_STD_ALLOCATOR
         BOOST_CHECK_EQUAL( db.head_block_num(), cutoff_block.block_num()+200 );
#endif
      }
   } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( undo_block )
{
   try {
      fc::temp_directory data_dir( steem::utilities::temp_directory_path() );
      {
         database db;
         witness::block_producer bp( db );
         db._log_hardforks = false;
         open_test_database( db, data_dir.path() );
         fc::time_point_sec now( STEEM_TESTING_GENESIS_TIMESTAMP );
         std::vector< time_point_sec > time_stack;

         auto init_account_priv_key  = fc::ecc::private_key::regenerate(fc::sha256::hash(string("init_key")) );
         for( uint32_t i = 0; i < 5; ++i )
         {
            now = db.get_slot_time(1);
            time_stack.push_back( now );
            auto b = bp.generate_block( now, db.get_scheduled_witness( 1 ), init_account_priv_key, database::skip_nothing );
         }
         BOOST_CHECK( db.head_block_num() == 5 );
         BOOST_CHECK( db.head_block_time() == now );
         db.pop_block();
         time_stack.pop_back();
         now = time_stack.back();
         BOOST_CHECK( db.head_block_num() == 4 );
         BOOST_CHECK( db.head_block_time() == now );
         db.pop_block();
         time_stack.pop_back();
         now = time_stack.back();
         BOOST_CHECK( db.head_block_num() == 3 );
         BOOST_CHECK( db.head_block_time() == now );
         db.pop_block();
         time_stack.pop_back();
         now = time_stack.back();
         BOOST_CHECK( db.head_block_num() == 2 );
         BOOST_CHECK( db.head_block_time() == now );
         for( uint32_t i = 0; i < 5; ++i )
         {
            now = db.get_slot_time(1);
            time_stack.push_back( now );
            auto b = bp.generate_block( now, db.get_scheduled_witness( 1 ), init_account_priv_key, database::skip_nothing );
         }
         BOOST_CHECK( db.head_block_num() == 7 );
      }
   } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( fork_blocks )
{
   try {
      fc::temp_directory data_dir1( steem::utilities::temp_directory_path() );
      fc::temp_directory data_dir2( steem::utilities::temp_directory_path() );

      //TODO This test needs 6-7 ish witnesses prior to fork

      database db1;
      witness::block_producer bp1( db1 );
      db1._log_hardforks = false;
      open_test_database( db1, data_dir1.path() );
      database db2;
      witness::block_producer bp2( db2 );
      db2._log_hardforks = false;
      open_test_database( db2, data_dir2.path() );

      auto init_account_priv_key  = fc::ecc::private_key::regenerate(fc::sha256::hash(string("init_key")) );
      for( uint32_t i = 0; i < 10; ++i )
      {
         auto b = bp1.generate_block(db1.get_slot_time(1), db1.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
         try {
            PUSH_BLOCK( db2, b );
         } FC_CAPTURE_AND_RETHROW( ("db2") );
      }
      for( uint32_t i = 10; i < 13; ++i )
      {
         auto b =  bp1.generate_block(db1.get_slot_time(1), db1.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
      }
      string db1_tip = db1.head_block_id().str();
      uint32_t next_slot = 3;
      for( uint32_t i = 13; i < 16; ++i )
      {
         auto b =  bp2.generate_block(db2.get_slot_time(next_slot), db2.get_scheduled_witness(next_slot), init_account_priv_key, database::skip_nothing);
         next_slot = 1;
         // notify both databases of the new block.
         // only db2 should switch to the new fork, db1 should not
         PUSH_BLOCK( db1, b );
         BOOST_CHECK_EQUAL(db1.head_block_id().str(), db1_tip);
         BOOST_CHECK_EQUAL(db2.head_block_id().str(), b.id().str());
      }

      //The two databases are on distinct forks now, but at the same height. Make a block on db2, make it invalid, then
      //pass it to db1 and assert that db1 doesn't switch to the new fork.
      signed_block good_block;
      BOOST_CHECK_EQUAL(db1.head_block_num(), 13);
      BOOST_CHECK_EQUAL(db2.head_block_num(), 13);
      {
         auto b = bp2.generate_block(db2.get_slot_time(1), db2.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
         good_block = b;
         b.transactions.emplace_back(signed_transaction());
         b.transactions.back().operations.emplace_back(transfer_operation());
         b.sign( init_account_priv_key );
         BOOST_CHECK_EQUAL(b.block_num(), 14);
         STEEM_CHECK_THROW(PUSH_BLOCK( db1, b ), fc::exception);
      }
      BOOST_CHECK_EQUAL(db1.head_block_num(), 13);
      BOOST_CHECK_EQUAL(db1.head_block_id().str(), db1_tip);

      // assert that db1 switches to new fork with good block
      BOOST_CHECK_EQUAL(db2.head_block_num(), 14);
      PUSH_BLOCK( db1, good_block );
      BOOST_CHECK_EQUAL(db1.head_block_id().str(), db2.head_block_id().str());
   } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( switch_forks_undo_create )
{
   try {
      fc::temp_directory dir1( steem::utilities::temp_directory_path() ),
                         dir2( steem::utilities::temp_directory_path() );
      database db1,
               db2;
      witness::block_producer bp1( db1 ),
                              bp2( db2 );
      db1._log_hardforks = false;
      open_test_database( db1, dir1.path() );
      db2._log_hardforks = false;
      open_test_database( db2, dir2.path() );

      auto init_account_priv_key  = fc::ecc::private_key::regenerate(fc::sha256::hash(string("init_key")) );
      public_key_type init_account_pub_key  = init_account_priv_key.get_public_key();
      db1.get_index< account_index >();

      //*
      signed_transaction trx;
      account_create_operation cop;
      cop.new_account_name = "alice";
      cop.creator = STEEM_INIT_MINER_NAME;
      cop.owner = authority(1, init_account_pub_key, 1);
      cop.active = cop.owner;
      trx.operations.push_back(cop);
      trx.set_expiration( db1.head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      trx.sign( init_account_priv_key, db1.get_chain_id(), fc::ecc::fc_canonical );
      PUSH_TX( db1, trx );
      //*/
      // generate blocks
      // db1 : A
      // db2 : B C D

      auto b = bp1.generate_block(db1.get_slot_time(1), db1.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);

      auto alice_id = db1.get_account( "alice" ).id;
      BOOST_CHECK( db1.get(alice_id).name == "alice" );

      b = bp2.generate_block(db2.get_slot_time(1), db2.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
      db1.push_block(b);
      b = bp2.generate_block(db2.get_slot_time(1), db2.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
      db1.push_block(b);
      STEEM_REQUIRE_THROW(db2.get(alice_id), std::exception);
      db1.get(alice_id); /// it should be included in the pending state
      db1.clear_pending(); // clear it so that we can verify it was properly removed from pending state.
      STEEM_REQUIRE_THROW(db1.get(alice_id), std::exception);

      PUSH_TX( db2, trx );

      b = bp2.generate_block(db2.get_slot_time(1), db2.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
      db1.push_block(b);

      BOOST_CHECK( db1.get(alice_id).name == "alice");
      BOOST_CHECK( db2.get(alice_id).name == "alice");
   } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( duplicate_transactions )
{
   try {
      fc::temp_directory dir1( steem::utilities::temp_directory_path() ),
                         dir2( steem::utilities::temp_directory_path() );
      database db1,
               db2;
      witness::block_producer bp1( db1 );
      db1._log_hardforks = false;
      open_test_database( db1, dir1.path() );
      db2._log_hardforks = false;
      open_test_database( db2, dir2.path() );
      BOOST_CHECK( db1.get_chain_id() == db2.get_chain_id() );

      auto skip_sigs = database::skip_transaction_signatures | database::skip_authority_check;

      auto init_account_priv_key  = fc::ecc::private_key::regenerate(fc::sha256::hash(string("init_key")) );
      public_key_type init_account_pub_key  = init_account_priv_key.get_public_key();

      signed_transaction trx;
      account_create_operation cop;
      cop.new_account_name = "alice";
      cop.creator = STEEM_INIT_MINER_NAME;
      cop.owner = authority(1, init_account_pub_key, 1);
      cop.active = cop.owner;
      trx.operations.push_back(cop);
      trx.set_expiration( db1.head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      trx.sign( init_account_priv_key, db1.get_chain_id(), fc::ecc::fc_canonical );
      PUSH_TX( db1, trx, skip_sigs );

      trx = decltype(trx)();
      transfer_operation t;
      t.from = STEEM_INIT_MINER_NAME;
      t.to = "alice";
      t.amount = asset(500,STEEM_SYMBOL);
      trx.operations.push_back(t);
      trx.set_expiration( db1.head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      trx.sign( init_account_priv_key, db1.get_chain_id(), fc::ecc::fc_canonical );
      PUSH_TX( db1, trx, skip_sigs );

      STEEM_CHECK_THROW(PUSH_TX( db1, trx, skip_sigs ), fc::exception);

      auto b = bp1.generate_block( db1.get_slot_time(1), db1.get_scheduled_witness( 1 ), init_account_priv_key, skip_sigs );
      PUSH_BLOCK( db2, b, skip_sigs );

      STEEM_CHECK_THROW(PUSH_TX( db1, trx, skip_sigs ), fc::exception);
      STEEM_CHECK_THROW(PUSH_TX( db2, trx, skip_sigs ), fc::exception);
      BOOST_CHECK_EQUAL(db1.get_balance( "alice", STEEM_SYMBOL ).amount.value, 500);
      BOOST_CHECK_EQUAL(db2.get_balance( "alice", STEEM_SYMBOL ).amount.value, 500);
   } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( tapos )
{
   try {
      fc::temp_directory dir1( steem::utilities::temp_directory_path() );
      database db1;
      witness::block_producer bp1( db1 );
      db1._log_hardforks = false;
      open_test_database( db1, dir1.path() );

      auto init_account_priv_key  = fc::ecc::private_key::regenerate(fc::sha256::hash(string("init_key")) );
      public_key_type init_account_pub_key  = init_account_priv_key.get_public_key();

      auto b = bp1.generate_block( db1.get_slot_time(1), db1.get_scheduled_witness( 1 ), init_account_priv_key, database::skip_nothing);

      BOOST_TEST_MESSAGE( "Creating a transaction with reference block" );
      idump((db1.head_block_id()));
      signed_transaction trx;
      //This transaction must be in the next block after its reference, or it is invalid.
      trx.set_reference_block( db1.head_block_id() );

      account_create_operation cop;
      cop.new_account_name = "alice";
      cop.creator = STEEM_INIT_MINER_NAME;
      cop.owner = authority(1, init_account_pub_key, 1);
      cop.active = cop.owner;
      trx.operations.push_back(cop);
      trx.set_expiration( db1.head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      trx.sign( init_account_priv_key, db1.get_chain_id(), fc::ecc::fc_canonical );

      BOOST_TEST_MESSAGE( "Pushing Pending Transaction" );
      idump((trx));
      db1.push_transaction(trx);
      BOOST_TEST_MESSAGE( "Generating a block" );
      b = bp1.generate_block(db1.get_slot_time(1), db1.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
      trx.clear();

      transfer_operation t;
      t.from = STEEM_INIT_MINER_NAME;
      t.to = "alice";
      t.amount = asset(50,STEEM_SYMBOL);
      trx.operations.push_back(t);
      trx.set_expiration( db1.head_block_time() + fc::seconds(2) );
      trx.sign( init_account_priv_key, db1.get_chain_id(), fc::ecc::fc_canonical );
      idump((trx)(db1.head_block_time()));
      b = bp1.generate_block(db1.get_slot_time(1), db1.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
      idump((b));
      b = bp1.generate_block(db1.get_slot_time(1), db1.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
      trx.signatures.clear();
      trx.sign( init_account_priv_key, db1.get_chain_id(), fc::ecc::fc_canonical );
      BOOST_REQUIRE_THROW( db1.push_transaction(trx, 0/*database::skip_transaction_signatures | database::skip_authority_check*/), fc::exception );
   } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_FIXTURE_TEST_CASE( optional_tapos, clean_database_fixture )
{
   try
   {
      idump((db->get_account("initminer")));
      ACTORS( (alice)(bob) );

      generate_block();

      BOOST_TEST_MESSAGE( "Create transaction" );

      transfer( STEEM_INIT_MINER_NAME, "alice", asset( 1000000, STEEM_SYMBOL ) );
      transfer_operation op;
      op.from = "alice";
      op.to = "bob";
      op.amount = asset(1000,STEEM_SYMBOL);
      signed_transaction tx;
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "ref_block_num=0, ref_block_prefix=0" );

      tx.ref_block_num = 0;
      tx.ref_block_prefix = 0;
      tx.signatures.clear();
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      PUSH_TX( *db, tx );

      BOOST_TEST_MESSAGE( "proper ref_block_num, ref_block_prefix" );

      tx.signatures.clear();
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      PUSH_TX( *db, tx, database::skip_transaction_dupe_check );

      BOOST_TEST_MESSAGE( "ref_block_num=0, ref_block_prefix=12345678" );

      tx.ref_block_num = 0;
      tx.ref_block_prefix = 0x12345678;
      tx.signatures.clear();
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( PUSH_TX( *db, tx, database::skip_transaction_dupe_check ), fc::exception );

      BOOST_TEST_MESSAGE( "ref_block_num=1, ref_block_prefix=12345678" );

      tx.ref_block_num = 1;
      tx.ref_block_prefix = 0x12345678;
      tx.signatures.clear();
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( PUSH_TX( *db, tx, database::skip_transaction_dupe_check ), fc::exception );

      BOOST_TEST_MESSAGE( "ref_block_num=9999, ref_block_prefix=12345678" );

      tx.ref_block_num = 9999;
      tx.ref_block_prefix = 0x12345678;
      tx.signatures.clear();
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      sign( tx, alice_private_key );
      STEEM_REQUIRE_THROW( PUSH_TX( *db, tx, database::skip_transaction_dupe_check ), fc::exception );
   }
   catch (fc::exception& e)
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_FIXTURE_TEST_CASE( double_sign_check, clean_database_fixture )
{ try {
   generate_block();
   ACTOR(bob);
   share_type amount = 1000;

   transfer_operation t;
   t.from = STEEM_INIT_MINER_NAME;
   t.to = "bob";
   t.amount = asset(amount,STEEM_SYMBOL);
   trx.operations.push_back(t);
   trx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   trx.validate();

   db->push_transaction(trx, ~0);

   trx.operations.clear();
   t.from = "bob";
   t.to = STEEM_INIT_MINER_NAME;
   t.amount = asset(amount,STEEM_SYMBOL);
   trx.operations.push_back(t);
   trx.validate();

   BOOST_TEST_MESSAGE( "Verify that not-signing causes an exception" );
   STEEM_REQUIRE_THROW( db->push_transaction(trx, 0), fc::exception );

   BOOST_TEST_MESSAGE( "Verify that double-signing causes an exception" );
   sign( trx, bob_private_key );
   sign( trx, bob_private_key );
   STEEM_REQUIRE_THROW( db->push_transaction(trx, 0), tx_duplicate_sig );

   BOOST_TEST_MESSAGE( "Verify that signing with an extra, unused key fails" );
   trx.signatures.pop_back();
   sign( trx, generate_private_key( "bogus" ) );
   STEEM_REQUIRE_THROW( db->push_transaction(trx, 0), tx_irrelevant_sig );

   BOOST_TEST_MESSAGE( "Verify that signing once with the proper key passes" );
   trx.signatures.pop_back();
   db->push_transaction(trx, 0);
   sign( trx, bob_private_key );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE( pop_block_twice, clean_database_fixture )
{
   try
   {
      uint32_t skip_flags = (
           database::skip_witness_signature
         | database::skip_transaction_signatures
         | database::skip_authority_check
         );

      // Sam is the creator of accounts
      auto init_account_priv_key  = fc::ecc::private_key::regenerate(fc::sha256::hash(string("init_key")) );
      private_key_type sam_key = generate_private_key( "sam" );
      account_create( "sam", sam_key.get_public_key() );

      //Get a sane head block time
      generate_block( skip_flags );

      transaction tx;
      signed_transaction ptx;

      db->get_account( STEEM_INIT_MINER_NAME );
      // transfer from committee account to Sam account
      transfer( STEEM_INIT_MINER_NAME, "sam", asset( 100000, STEEM_SYMBOL ) );

      generate_block(skip_flags);

      account_create( "alice", generate_private_key( "alice" ).get_public_key() );
      generate_block(skip_flags);
      account_create( "bob", generate_private_key( "bob" ).get_public_key() );
      generate_block(skip_flags);

      db->pop_block();
      db->pop_block();
   } catch(const fc::exception& e) {
      edump( (e.to_detail_string()) );
      throw;
   }
}

BOOST_FIXTURE_TEST_CASE( rsf_missed_blocks, clean_database_fixture )
{
   try
   {
      generate_block();

      auto rsf = [&]() -> string
      {
         fc::uint128 rsf = db->get_dynamic_global_properties().recent_slots_filled;
         string result = "";
         result.reserve(128);
         for( int i=0; i<128; i++ )
         {
            result += ((rsf.lo & 1) == 0) ? '0' : '1';
            rsf >>= 1;
         }
         return result;
      };

      auto pct = []( uint32_t x ) -> uint32_t
      {
         return uint64_t( STEEM_100_PERCENT ) * x / 128;
      };

      BOOST_TEST_MESSAGE("checking initial participation rate" );
      BOOST_CHECK_EQUAL( rsf(),
         "1111111111111111111111111111111111111111111111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db->witness_participation_rate(), STEEM_100_PERCENT );

      BOOST_TEST_MESSAGE("Generating a block skipping 1" );
      generate_block( ~database::skip_fork_db, init_account_priv_key, 1 );
      BOOST_CHECK_EQUAL( rsf(),
         "0111111111111111111111111111111111111111111111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db->witness_participation_rate(), pct(127) );

      BOOST_TEST_MESSAGE("Generating a block skipping 1" );
      generate_block( ~database::skip_fork_db, init_account_priv_key, 1 );
      BOOST_CHECK_EQUAL( rsf(),
         "0101111111111111111111111111111111111111111111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db->witness_participation_rate(), pct(126) );

      BOOST_TEST_MESSAGE("Generating a block skipping 2" );
      generate_block( ~database::skip_fork_db, init_account_priv_key, 2 );
      BOOST_CHECK_EQUAL( rsf(),
         "0010101111111111111111111111111111111111111111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db->witness_participation_rate(), pct(124) );

      BOOST_TEST_MESSAGE("Generating a block for skipping 3" );
      generate_block( ~database::skip_fork_db, init_account_priv_key, 3 );
      BOOST_CHECK_EQUAL( rsf(),
         "0001001010111111111111111111111111111111111111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db->witness_participation_rate(), pct(121) );

      BOOST_TEST_MESSAGE("Generating a block skipping 5" );
      generate_block( ~database::skip_fork_db, init_account_priv_key, 5 );
      BOOST_CHECK_EQUAL( rsf(),
         "0000010001001010111111111111111111111111111111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db->witness_participation_rate(), pct(116) );

      BOOST_TEST_MESSAGE("Generating a block skipping 8" );
      generate_block( ~database::skip_fork_db, init_account_priv_key, 8 );
      BOOST_CHECK_EQUAL( rsf(),
         "0000000010000010001001010111111111111111111111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db->witness_participation_rate(), pct(108) );

      BOOST_TEST_MESSAGE("Generating a block skipping 13" );
      generate_block( ~database::skip_fork_db, init_account_priv_key, 13 );
      BOOST_CHECK_EQUAL( rsf(),
         "0000000000000100000000100000100010010101111111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db->witness_participation_rate(), pct(95) );

      BOOST_TEST_MESSAGE("Generating a block skipping none" );
      generate_block();
      BOOST_CHECK_EQUAL( rsf(),
         "1000000000000010000000010000010001001010111111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db->witness_participation_rate(), pct(95) );

      BOOST_TEST_MESSAGE("Generating a block" );
      generate_block();
      BOOST_CHECK_EQUAL( rsf(),
         "1100000000000001000000001000001000100101011111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db->witness_participation_rate(), pct(95) );

      generate_block();
      BOOST_CHECK_EQUAL( rsf(),
         "1110000000000000100000000100000100010010101111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db->witness_participation_rate(), pct(95) );

      generate_block();
      BOOST_CHECK_EQUAL( rsf(),
         "1111000000000000010000000010000010001001010111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db->witness_participation_rate(), pct(95) );

      generate_block( ~database::skip_fork_db, init_account_priv_key, 64 );
      BOOST_CHECK_EQUAL( rsf(),
         "0000000000000000000000000000000000000000000000000000000000000000"
         "1111100000000000001000000001000001000100101011111111111111111111"
      );
      BOOST_CHECK_EQUAL( db->witness_participation_rate(), pct(31) );

      generate_block( ~database::skip_fork_db, init_account_priv_key, 32 );
      BOOST_CHECK_EQUAL( rsf(),
         "0000000000000000000000000000000010000000000000000000000000000000"
         "0000000000000000000000000000000001111100000000000001000000001000"
      );
      BOOST_CHECK_EQUAL( db->witness_participation_rate(), pct(8) );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( skip_block, clean_database_fixture )
{
   try
   {
      BOOST_TEST_MESSAGE( "Skipping blocks through db" );
      BOOST_REQUIRE( db->head_block_num() == 2 );

      witness::block_producer bp( *db );
      int init_block_num = db->head_block_num();
      int miss_blocks = fc::minutes( 1 ).to_seconds() / STEEM_BLOCK_INTERVAL;
      auto witness = db->get_scheduled_witness( miss_blocks );
      auto block_time = db->get_slot_time( miss_blocks );
      bp.generate_block( block_time , witness, init_account_priv_key, 0 );

      BOOST_CHECK_EQUAL( db->head_block_num(), init_block_num + 1 );
      BOOST_CHECK( db->head_block_time() == block_time );

      BOOST_TEST_MESSAGE( "Generating a block through fixture" );
      generate_block();

      BOOST_CHECK_EQUAL( db->head_block_num(), init_block_num + 2 );
      BOOST_CHECK( db->head_block_time() == block_time + STEEM_BLOCK_INTERVAL );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_FIXTURE_TEST_CASE( hardfork_test, database_fixture )
{
   try
   {
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
      appbase::app().register_plugin< steem::plugins::account_history::account_history_plugin >();
      db_plugin = &appbase::app().register_plugin< steem::plugins::debug_node::debug_node_plugin >();
      init_account_pub_key = init_account_priv_key.get_public_key();

      appbase::app().initialize<
         steem::plugins::account_history::account_history_plugin,
         steem::plugins::debug_node::debug_node_plugin
      >( argc, argv );

      db = &appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db();
      BOOST_REQUIRE( db );


      open_database();

      generate_blocks( 2 );

      vest( "initminer", 10000 );

      // Fill up the rest of the required miners
      for( int i = STEEM_NUM_INIT_MINERS; i < STEEM_MAX_WITNESSES; i++ )
      {
         account_create( STEEM_INIT_MINER_NAME + fc::to_string( i ), init_account_pub_key );
         fund( STEEM_INIT_MINER_NAME + fc::to_string( i ), STEEM_MIN_PRODUCER_REWARD.amount.value );
         witness_create( STEEM_INIT_MINER_NAME + fc::to_string( i ), init_account_priv_key, "foo.bar", init_account_pub_key, STEEM_MIN_PRODUCER_REWARD.amount );
      }

      validate_database();
      } catch ( const fc::exception& e )
      {
         edump( (e.to_detail_string()) );
         throw;
      }

      BOOST_TEST_MESSAGE( "Check hardfork not applied at genesis" );
      BOOST_REQUIRE( db->has_hardfork( 0 ) );
      BOOST_REQUIRE( !db->has_hardfork( STEEM_HARDFORK_0_1 ) );

      BOOST_TEST_MESSAGE( "Generate blocks up to the hardfork time and check hardfork still not applied" );
      generate_blocks( fc::time_point_sec( STEEM_HARDFORK_0_1_TIME - STEEM_BLOCK_INTERVAL ), true );

      BOOST_REQUIRE( db->has_hardfork( 0 ) );
      BOOST_REQUIRE( !db->has_hardfork( STEEM_HARDFORK_0_1 ) );

      BOOST_TEST_MESSAGE( "Generate a block and check hardfork is applied" );
      generate_block();

      string op_msg = "Testnet: Hardfork applied";
      auto itr = db->get_index< account_history_index >().indices().get< by_id >().end();
      itr--;

      BOOST_REQUIRE( db->has_hardfork( 0 ) );
      BOOST_REQUIRE( db->has_hardfork( STEEM_HARDFORK_0_1 ) );
      operation hardfork_vop = hardfork_operation( STEEM_HARDFORK_0_1 );

      BOOST_REQUIRE( get_last_operations( 1 )[0] == hardfork_vop );
      BOOST_REQUIRE( db->get(itr->op).timestamp == db->head_block_time() );

      BOOST_TEST_MESSAGE( "Testing hardfork is only applied once" );
      generate_block();

      itr = db->get_index< account_history_index >().indices().get< by_id >().end();
      itr--;

      BOOST_REQUIRE( db->has_hardfork( 0 ) );
      BOOST_REQUIRE( db->has_hardfork( STEEM_HARDFORK_0_1 ) );
      BOOST_REQUIRE( get_last_operations( 1 )[0] == hardfork_vop );
      BOOST_REQUIRE( db->get(itr->op).timestamp == db->head_block_time() - STEEM_BLOCK_INTERVAL );

   }
   catch( fc::exception& e )
   {
      db->wipe( data_dir->path(), data_dir->path(), true );
      throw e;
   }
   catch( std::exception& e )
   {
      db->wipe( data_dir->path(), data_dir->path(), true );
      throw e;
   }

   db->wipe( data_dir->path(), data_dir->path(), true );
}

BOOST_FIXTURE_TEST_CASE( generate_block_size, clean_database_fixture )
{
   try
   {
      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo )
         {
            gpo.maximum_block_size = STEEM_MIN_BLOCK_SIZE_LIMIT;
         });
      });
      generate_block();

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );

      transfer_operation op;
      op.from = STEEM_INIT_MINER_NAME;
      op.to = STEEM_TEMP_ACCOUNT;
      op.amount = asset( 1000, STEEM_SYMBOL );

      // tx minus op is 79 bytes
      // op is 33 bytes (32 for op + 1 byte static variant tag)
      // total is 65254
      // Original generation logic only allowed 115 bytes for the header
      // We are targetting a size (minus header) of 65421 which creates a block of "size" 65535
      // This block will actually be larger because the header estimates is too small

      for( size_t i = 0; i < 1975; i++ )
      {
         tx.operations.push_back( op );
      }

      sign( tx, init_account_priv_key );
      db->push_transaction( tx, 0 );

      // Second transaction, tx minus op is 78 (one less byte for operation vector size)
      // We need a 88 byte op. We need a 22 character memo (1 byte for length) 55 = 32 (old op) + 55 + 1
      op.memo = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123";
      tx.clear();
      tx.operations.push_back( op );
      sign( tx, init_account_priv_key );
      db->push_transaction( tx, 0 );

      generate_block();

      // The last transfer should have been delayed due to size
      auto head_block = db->fetch_block_by_number( db->head_block_num() );
      BOOST_REQUIRE( head_block->transactions.size() == 1 );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif
