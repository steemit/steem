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

#include <steemit/protocol/exceptions.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/chain/steem_objects.hpp>
#include <steemit/chain/history_object.hpp>

#include <steemit/account_history/account_history_plugin.hpp>

#include <graphene/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace steemit;
using namespace steemit::chain;
using namespace steemit::protocol;

#define TEST_SHARED_MEM_SIZE (1024 * 1024 * 8)

BOOST_AUTO_TEST_SUITE(block_tests)

BOOST_AUTO_TEST_CASE( generate_empty_blocks )
{
   try {
      fc::time_point_sec now( STEEMIT_TESTING_GENESIS_TIMESTAMP );
      fc::temp_directory data_dir( graphene::utilities::temp_directory_path() );
      signed_block b;

      // TODO:  Don't generate this here
      auto init_account_priv_key = fc::ecc::private_key::regenerate( fc::sha256::hash( string( "init_key" ) ) );
      signed_block cutoff_block;
      {
         database db;
         db._log_hardforks = false;
         db.open(data_dir.path(), data_dir.path(), INITIAL_TEST_SUPPLY, TEST_SHARED_MEM_SIZE, chainbase::database::read_write );
         b = db.generate_block(db.get_slot_time(1), db.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);

         // TODO:  Change this test when we correct #406
         // n.b. we generate STEEMIT_MIN_UNDO_HISTORY+1 extra blocks which will be discarded on save
         for( uint32_t i = 1; ; ++i )
         {
            BOOST_CHECK( db.head_block_id() == b.id() );
            //witness_id_type prev_witness = b.witness;
            string cur_witness = db.get_scheduled_witness(1);
            //BOOST_CHECK( cur_witness != prev_witness );
            b = db.generate_block(db.get_slot_time(1), cur_witness, init_account_priv_key, database::skip_nothing);
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
         db._log_hardforks = false;
         db.open(data_dir.path(), data_dir.path(), INITIAL_TEST_SUPPLY, TEST_SHARED_MEM_SIZE, chainbase::database::read_write );
         BOOST_CHECK_EQUAL( db.head_block_num(), cutoff_block.block_num() );
         b = cutoff_block;
         for( uint32_t i = 0; i < 200; ++i )
         {
            BOOST_CHECK( db.head_block_id() == b.id() );
            //witness_id_type prev_witness = b.witness;
            string cur_witness = db.get_scheduled_witness(1);
            //BOOST_CHECK( cur_witness != prev_witness );
            b = db.generate_block(db.get_slot_time(1), cur_witness, init_account_priv_key, database::skip_nothing);
         }
         BOOST_CHECK_EQUAL( db.head_block_num(), cutoff_block.block_num()+200 );
      }
   } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( undo_block )
{
   try {
      fc::temp_directory data_dir( graphene::utilities::temp_directory_path() );
      {
         database db;
         db._log_hardforks = false;
         db.open(data_dir.path(), data_dir.path(), INITIAL_TEST_SUPPLY, TEST_SHARED_MEM_SIZE, chainbase::database::read_write );
         fc::time_point_sec now( STEEMIT_TESTING_GENESIS_TIMESTAMP );
         std::vector< time_point_sec > time_stack;

         auto init_account_priv_key  = fc::ecc::private_key::regenerate(fc::sha256::hash(string("init_key")) );
         for( uint32_t i = 0; i < 5; ++i )
         {
            now = db.get_slot_time(1);
            time_stack.push_back( now );
            auto b = db.generate_block( now, db.get_scheduled_witness( 1 ), init_account_priv_key, database::skip_nothing );
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
            auto b = db.generate_block( now, db.get_scheduled_witness( 1 ), init_account_priv_key, database::skip_nothing );
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
      fc::temp_directory data_dir1( graphene::utilities::temp_directory_path() );
      fc::temp_directory data_dir2( graphene::utilities::temp_directory_path() );

      //TODO This test needs 6-7 ish witnesses prior to fork

      database db1;
      db1._log_hardforks = false;
      db1.open( data_dir1.path(), data_dir1.path(), INITIAL_TEST_SUPPLY, TEST_SHARED_MEM_SIZE, chainbase::database::read_write );
      database db2;
      db2._log_hardforks = false;
      db2.open( data_dir2.path(), data_dir2.path(), INITIAL_TEST_SUPPLY, TEST_SHARED_MEM_SIZE, chainbase::database::read_write );

      auto init_account_priv_key  = fc::ecc::private_key::regenerate(fc::sha256::hash(string("init_key")) );
      for( uint32_t i = 0; i < 10; ++i )
      {
         auto b = db1.generate_block(db1.get_slot_time(1), db1.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
         try {
            PUSH_BLOCK( db2, b );
         } FC_CAPTURE_AND_RETHROW( ("db2") );
      }
      for( uint32_t i = 10; i < 13; ++i )
      {
         auto b =  db1.generate_block(db1.get_slot_time(1), db1.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
      }
      string db1_tip = db1.head_block_id().str();
      uint32_t next_slot = 3;
      for( uint32_t i = 13; i < 16; ++i )
      {
         auto b =  db2.generate_block(db2.get_slot_time(next_slot), db2.get_scheduled_witness(next_slot), init_account_priv_key, database::skip_nothing);
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
         auto b = db2.generate_block(db2.get_slot_time(1), db2.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
         good_block = b;
         b.transactions.emplace_back(signed_transaction());
         b.transactions.back().operations.emplace_back(transfer_operation());
         b.sign( init_account_priv_key );
         BOOST_CHECK_EQUAL(b.block_num(), 14);
         STEEMIT_CHECK_THROW(PUSH_BLOCK( db1, b ), fc::exception);
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
      fc::temp_directory dir1( graphene::utilities::temp_directory_path() ),
                         dir2( graphene::utilities::temp_directory_path() );
      database db1,
               db2;
      db1._log_hardforks = false;
      db1.open( dir1.path(), dir1.path(), INITIAL_TEST_SUPPLY, TEST_SHARED_MEM_SIZE, chainbase::database::read_write );
      db2._log_hardforks = false;
      db2.open( dir2.path(), dir2.path(), INITIAL_TEST_SUPPLY, TEST_SHARED_MEM_SIZE, chainbase::database::read_write );

      auto init_account_priv_key  = fc::ecc::private_key::regenerate(fc::sha256::hash(string("init_key")) );
      public_key_type init_account_pub_key  = init_account_priv_key.get_public_key();
      db1.get_index< account_index >();

      //*
      signed_transaction trx;
      account_create_operation cop;
      cop.new_account_name = "alice";
      cop.creator = STEEMIT_INIT_MINER_NAME;
      cop.owner = authority(1, init_account_pub_key, 1);
      cop.active = cop.owner;
      trx.operations.push_back(cop);
      trx.set_expiration( db1.head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION );
      trx.sign( init_account_priv_key, db1.get_chain_id() );
      PUSH_TX( db1, trx );
      //*/
      // generate blocks
      // db1 : A
      // db2 : B C D

      auto b = db1.generate_block(db1.get_slot_time(1), db1.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);

      auto alice_id = db1.get_account( "alice" ).id;
      BOOST_CHECK( db1.get(alice_id).name == "alice" );

      b = db2.generate_block(db2.get_slot_time(1), db2.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
      db1.push_block(b);
      b = db2.generate_block(db2.get_slot_time(1), db2.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
      db1.push_block(b);
      STEEMIT_REQUIRE_THROW(db2.get(alice_id), std::exception);
      db1.get(alice_id); /// it should be included in the pending state
      db1.clear_pending(); // clear it so that we can verify it was properly removed from pending state.
      STEEMIT_REQUIRE_THROW(db1.get(alice_id), std::exception);

      PUSH_TX( db2, trx );

      b = db2.generate_block(db2.get_slot_time(1), db2.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
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
      fc::temp_directory dir1( graphene::utilities::temp_directory_path() ),
                         dir2( graphene::utilities::temp_directory_path() );
      database db1,
               db2;
      db1._log_hardforks = false;
      db1.open(dir1.path(), dir1.path(), INITIAL_TEST_SUPPLY, TEST_SHARED_MEM_SIZE, chainbase::database::read_write );
      db2._log_hardforks = false;
      db2.open(dir2.path(), dir2.path(), INITIAL_TEST_SUPPLY, TEST_SHARED_MEM_SIZE, chainbase::database::read_write );
      BOOST_CHECK( db1.get_chain_id() == db2.get_chain_id() );

      auto skip_sigs = database::skip_transaction_signatures | database::skip_authority_check;

      auto init_account_priv_key  = fc::ecc::private_key::regenerate(fc::sha256::hash(string("init_key")) );
      public_key_type init_account_pub_key  = init_account_priv_key.get_public_key();

      signed_transaction trx;
      account_create_operation cop;
      cop.new_account_name = "alice";
      cop.creator = STEEMIT_INIT_MINER_NAME;
      cop.owner = authority(1, init_account_pub_key, 1);
      cop.active = cop.owner;
      trx.operations.push_back(cop);
      trx.set_expiration( db1.head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION );
      trx.sign( init_account_priv_key, db1.get_chain_id() );
      PUSH_TX( db1, trx, skip_sigs );

      trx = decltype(trx)();
      transfer_operation t;
      t.from = STEEMIT_INIT_MINER_NAME;
      t.to = "alice";
      t.amount = asset(500,STEEM_SYMBOL);
      trx.operations.push_back(t);
      trx.set_expiration( db1.head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION );
      trx.sign( init_account_priv_key, db1.get_chain_id() );
      PUSH_TX( db1, trx, skip_sigs );

      STEEMIT_CHECK_THROW(PUSH_TX( db1, trx, skip_sigs ), fc::exception);

      auto b = db1.generate_block( db1.get_slot_time(1), db1.get_scheduled_witness( 1 ), init_account_priv_key, skip_sigs );
      PUSH_BLOCK( db2, b, skip_sigs );

      STEEMIT_CHECK_THROW(PUSH_TX( db1, trx, skip_sigs ), fc::exception);
      STEEMIT_CHECK_THROW(PUSH_TX( db2, trx, skip_sigs ), fc::exception);
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
      fc::temp_directory dir1( graphene::utilities::temp_directory_path() );
      database db1;
      db1._log_hardforks = false;
      db1.open(dir1.path(), dir1.path(), INITIAL_TEST_SUPPLY, TEST_SHARED_MEM_SIZE, chainbase::database::read_write );

      auto init_account_priv_key  = fc::ecc::private_key::regenerate(fc::sha256::hash(string("init_key")) );
      public_key_type init_account_pub_key  = init_account_priv_key.get_public_key();

      auto b = db1.generate_block( db1.get_slot_time(1), db1.get_scheduled_witness( 1 ), init_account_priv_key, database::skip_nothing);

      BOOST_TEST_MESSAGE( "Creating a transaction with reference block" );
      idump((db1.head_block_id()));
      signed_transaction trx;
      //This transaction must be in the next block after its reference, or it is invalid.
      trx.set_reference_block( db1.head_block_id() );

      account_create_operation cop;
      cop.new_account_name = "alice";
      cop.creator = STEEMIT_INIT_MINER_NAME;
      cop.owner = authority(1, init_account_pub_key, 1);
      cop.active = cop.owner;
      trx.operations.push_back(cop);
      trx.set_expiration( db1.head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION );
      trx.sign( init_account_priv_key, db1.get_chain_id() );

      BOOST_TEST_MESSAGE( "Pushing Pending Transaction" );
      idump((trx));
      db1.push_transaction(trx);
      BOOST_TEST_MESSAGE( "Generating a block" );
      b = db1.generate_block(db1.get_slot_time(1), db1.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
      trx.clear();

      transfer_operation t;
      t.from = STEEMIT_INIT_MINER_NAME;
      t.to = "alice";
      t.amount = asset(50,STEEM_SYMBOL);
      trx.operations.push_back(t);
      trx.set_expiration( db1.head_block_time() + fc::seconds(2) );
      trx.sign( init_account_priv_key, db1.get_chain_id() );
      idump((trx)(db1.head_block_time()));
      b = db1.generate_block(db1.get_slot_time(1), db1.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
      idump((b));
      b = db1.generate_block(db1.get_slot_time(1), db1.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
      trx.signatures.clear();
      trx.sign( init_account_priv_key, db1.get_chain_id() );
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
      idump((db.get_account("initminer")));
      ACTORS( (alice)(bob) );

      generate_block();

      BOOST_TEST_MESSAGE( "Create transaction" );

      transfer( STEEMIT_INIT_MINER_NAME, "alice", 1000000 );
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
      tx.set_expiration( db.head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      PUSH_TX( db, tx );

      BOOST_TEST_MESSAGE( "proper ref_block_num, ref_block_prefix" );

      tx.signatures.clear();
      tx.set_expiration( db.head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      PUSH_TX( db, tx, database::skip_transaction_dupe_check );

      BOOST_TEST_MESSAGE( "ref_block_num=0, ref_block_prefix=12345678" );

      tx.ref_block_num = 0;
      tx.ref_block_prefix = 0x12345678;
      tx.signatures.clear();
      tx.set_expiration( db.head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      STEEMIT_REQUIRE_THROW( PUSH_TX( db, tx, database::skip_transaction_dupe_check ), fc::exception );

      BOOST_TEST_MESSAGE( "ref_block_num=1, ref_block_prefix=12345678" );

      tx.ref_block_num = 1;
      tx.ref_block_prefix = 0x12345678;
      tx.signatures.clear();
      tx.set_expiration( db.head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      STEEMIT_REQUIRE_THROW( PUSH_TX( db, tx, database::skip_transaction_dupe_check ), fc::exception );

      BOOST_TEST_MESSAGE( "ref_block_num=9999, ref_block_prefix=12345678" );

      tx.ref_block_num = 9999;
      tx.ref_block_prefix = 0x12345678;
      tx.signatures.clear();
      tx.set_expiration( db.head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      STEEMIT_REQUIRE_THROW( PUSH_TX( db, tx, database::skip_transaction_dupe_check ), fc::exception );
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
   t.from = STEEMIT_INIT_MINER_NAME;
   t.to = "bob";
   t.amount = asset(amount,STEEM_SYMBOL);
   trx.operations.push_back(t);
   trx.set_expiration( db.head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION );
   trx.validate();

   db.push_transaction(trx, ~0);

   trx.operations.clear();
   t.from = "bob";
   t.to = STEEMIT_INIT_MINER_NAME;
   t.amount = asset(amount,STEEM_SYMBOL);
   trx.operations.push_back(t);
   trx.validate();

   BOOST_TEST_MESSAGE( "Verify that not-signing causes an exception" );
   STEEMIT_REQUIRE_THROW( db.push_transaction(trx, 0), fc::exception );

   BOOST_TEST_MESSAGE( "Verify that double-signing causes an exception" );
   trx.sign( bob_private_key, db.get_chain_id() );
   trx.sign( bob_private_key, db.get_chain_id() );
   STEEMIT_REQUIRE_THROW( db.push_transaction(trx, 0), tx_duplicate_sig );

   BOOST_TEST_MESSAGE( "Verify that signing with an extra, unused key fails" );
   trx.signatures.pop_back();
   trx.sign( generate_private_key("bogus" ), db.get_chain_id() );
   STEEMIT_REQUIRE_THROW( db.push_transaction(trx, 0), tx_irrelevant_sig );

   BOOST_TEST_MESSAGE( "Verify that signing once with the proper key passes" );
   trx.signatures.pop_back();
   db.push_transaction(trx, 0);
   trx.sign( bob_private_key, db.get_chain_id() );

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
      account_object sam_account_object = account_create( "sam", sam_key.get_public_key() );

      //Get a sane head block time
      generate_block( skip_flags );

      transaction tx;
      signed_transaction ptx;

      db.get_account( STEEMIT_INIT_MINER_NAME );
      // transfer from committee account to Sam account
      transfer( STEEMIT_INIT_MINER_NAME, "sam", 100000 );

      generate_block(skip_flags);

      account_create( "alice", generate_private_key( "alice" ).get_public_key() );
      generate_block(skip_flags);
      account_create( "bob", generate_private_key( "bob" ).get_public_key() );
      generate_block(skip_flags);

      db.pop_block();
      db.pop_block();
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
         fc::uint128 rsf = db.get_dynamic_global_properties().recent_slots_filled;
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
         return uint64_t( STEEMIT_100_PERCENT ) * x / 128;
      };

      BOOST_TEST_MESSAGE("checking initial participation rate" );
      BOOST_CHECK_EQUAL( rsf(),
         "1111111111111111111111111111111111111111111111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.witness_participation_rate(), STEEMIT_100_PERCENT );

      BOOST_TEST_MESSAGE("Generating a block skipping 1" );
      generate_block( ~database::skip_fork_db, init_account_priv_key, 1 );
      BOOST_CHECK_EQUAL( rsf(),
         "0111111111111111111111111111111111111111111111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.witness_participation_rate(), pct(127) );

      BOOST_TEST_MESSAGE("Generating a block skipping 1" );
      generate_block( ~database::skip_fork_db, init_account_priv_key, 1 );
      BOOST_CHECK_EQUAL( rsf(),
         "0101111111111111111111111111111111111111111111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.witness_participation_rate(), pct(126) );

      BOOST_TEST_MESSAGE("Generating a block skipping 2" );
      generate_block( ~database::skip_fork_db, init_account_priv_key, 2 );
      BOOST_CHECK_EQUAL( rsf(),
         "0010101111111111111111111111111111111111111111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.witness_participation_rate(), pct(124) );

      BOOST_TEST_MESSAGE("Generating a block for skipping 3" );
      generate_block( ~database::skip_fork_db, init_account_priv_key, 3 );
      BOOST_CHECK_EQUAL( rsf(),
         "0001001010111111111111111111111111111111111111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.witness_participation_rate(), pct(121) );

      BOOST_TEST_MESSAGE("Generating a block skipping 5" );
      generate_block( ~database::skip_fork_db, init_account_priv_key, 5 );
      BOOST_CHECK_EQUAL( rsf(),
         "0000010001001010111111111111111111111111111111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.witness_participation_rate(), pct(116) );

      BOOST_TEST_MESSAGE("Generating a block skipping 8" );
      generate_block( ~database::skip_fork_db, init_account_priv_key, 8 );
      BOOST_CHECK_EQUAL( rsf(),
         "0000000010000010001001010111111111111111111111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.witness_participation_rate(), pct(108) );

      BOOST_TEST_MESSAGE("Generating a block skipping 13" );
      generate_block( ~database::skip_fork_db, init_account_priv_key, 13 );
      BOOST_CHECK_EQUAL( rsf(),
         "0000000000000100000000100000100010010101111111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.witness_participation_rate(), pct(95) );

      BOOST_TEST_MESSAGE("Generating a block skipping none" );
      generate_block();
      BOOST_CHECK_EQUAL( rsf(),
         "1000000000000010000000010000010001001010111111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.witness_participation_rate(), pct(95) );

      BOOST_TEST_MESSAGE("Generating a block" );
      generate_block();
      BOOST_CHECK_EQUAL( rsf(),
         "1100000000000001000000001000001000100101011111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.witness_participation_rate(), pct(95) );

      generate_block();
      BOOST_CHECK_EQUAL( rsf(),
         "1110000000000000100000000100000100010010101111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.witness_participation_rate(), pct(95) );

      generate_block();
      BOOST_CHECK_EQUAL( rsf(),
         "1111000000000000010000000010000010001001010111111111111111111111"
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.witness_participation_rate(), pct(95) );

      generate_block( ~database::skip_fork_db, init_account_priv_key, 64 );
      BOOST_CHECK_EQUAL( rsf(),
         "0000000000000000000000000000000000000000000000000000000000000000"
         "1111100000000000001000000001000001000100101011111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.witness_participation_rate(), pct(31) );

      generate_block( ~database::skip_fork_db, init_account_priv_key, 32 );
      BOOST_CHECK_EQUAL( rsf(),
         "0000000000000000000000000000000010000000000000000000000000000000"
         "0000000000000000000000000000000001111100000000000001000000001000"
      );
      BOOST_CHECK_EQUAL( db.witness_participation_rate(), pct(8) );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( skip_block, clean_database_fixture )
{
   try
   {
      BOOST_TEST_MESSAGE( "Skipping blocks through db" );
      BOOST_REQUIRE( db.head_block_num() == 2 );

      int init_block_num = db.head_block_num();
      int miss_blocks = fc::minutes( 1 ).to_seconds() / STEEMIT_BLOCK_INTERVAL;
      auto witness = db.get_scheduled_witness( miss_blocks );
      auto block_time = db.get_slot_time( miss_blocks );
      db.generate_block( block_time , witness, init_account_priv_key, 0 );

      BOOST_CHECK_EQUAL( db.head_block_num(), init_block_num + 1 );
      BOOST_CHECK( db.head_block_time() == block_time );

      BOOST_TEST_MESSAGE( "Generating a block through fixture" );
      generate_block();

      BOOST_CHECK_EQUAL( db.head_block_num(), init_block_num + 2 );
      BOOST_CHECK( db.head_block_time() == block_time + STEEMIT_BLOCK_INTERVAL );
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
      auto ahplugin = app.register_plugin< steemit::account_history::account_history_plugin >();
      db_plugin = app.register_plugin< steemit::plugin::debug_node::debug_node_plugin >();
      init_account_pub_key = init_account_priv_key.get_public_key();

      boost::program_options::variables_map options;

      ahplugin->plugin_initialize( options );
      db_plugin->plugin_initialize( options );

      open_database();

      generate_blocks( 2 );

      ahplugin->plugin_startup();
      db_plugin->plugin_startup();

      vest( "initminer", 10000 );

      // Fill up the rest of the required miners
      for( int i = STEEMIT_NUM_INIT_MINERS; i < STEEMIT_MAX_WITNESSES; i++ )
      {
         account_create( STEEMIT_INIT_MINER_NAME + fc::to_string( i ), init_account_pub_key );
         fund( STEEMIT_INIT_MINER_NAME + fc::to_string( i ), STEEMIT_MIN_PRODUCER_REWARD.amount.value );
         witness_create( STEEMIT_INIT_MINER_NAME + fc::to_string( i ), init_account_priv_key, "foo.bar", init_account_pub_key, STEEMIT_MIN_PRODUCER_REWARD.amount );
      }

      validate_database();
      } catch ( const fc::exception& e )
      {
         edump( (e.to_detail_string()) );
         throw;
      }

      BOOST_TEST_MESSAGE( "Check hardfork not applied at genesis" );
      BOOST_REQUIRE( db.has_hardfork( 0 ) );
      BOOST_REQUIRE( !db.has_hardfork( STEEMIT_HARDFORK_0_1 ) );

      BOOST_TEST_MESSAGE( "Generate blocks up to the hardfork time and check hardfork still not applied" );
      generate_blocks( fc::time_point_sec( STEEMIT_HARDFORK_0_1_TIME - STEEMIT_BLOCK_INTERVAL ), true );

      BOOST_REQUIRE( db.has_hardfork( 0 ) );
      BOOST_REQUIRE( !db.has_hardfork( STEEMIT_HARDFORK_0_1 ) );

      BOOST_TEST_MESSAGE( "Generate a block and check hardfork is applied" );
      generate_block();

      string op_msg = "Testnet: Hardfork applied";
      auto itr = db.get_index< account_history_index >().indices().get< by_id >().end();
      itr--;

      BOOST_REQUIRE( db.has_hardfork( 0 ) );
      BOOST_REQUIRE( db.has_hardfork( STEEMIT_HARDFORK_0_1 ) );
      BOOST_REQUIRE( get_last_operations( 1 )[0].get< custom_operation >().data == vector< char >( op_msg.begin(), op_msg.end() ) );
      BOOST_REQUIRE( db.get(itr->op).timestamp == db.head_block_time() );

      BOOST_TEST_MESSAGE( "Testing hardfork is only applied once" );
      generate_block();

      itr = db.get_index< account_history_index >().indices().get< by_id >().end();
      itr--;

      BOOST_REQUIRE( db.has_hardfork( 0 ) );
      BOOST_REQUIRE( db.has_hardfork( STEEMIT_HARDFORK_0_1 ) );
      BOOST_REQUIRE( get_last_operations( 1 )[0].get< custom_operation >().data == vector< char >( op_msg.begin(), op_msg.end() ) );
      BOOST_REQUIRE( db.get(itr->op).timestamp == db.head_block_time() - STEEMIT_BLOCK_INTERVAL );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif
