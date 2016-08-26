#include <boost/test/unit_test.hpp>
#include <boost/program_options.hpp>

#include <graphene/db/simple_index.hpp>
#include <graphene/time/time.hpp>
#include <graphene/utilities/tempdir.hpp>

#include <steemit/chain/steem_objects.hpp>
#include <steemit/chain/history_object.hpp>
#include <steemit/account_history/account_history_plugin.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/smart_ref_impl.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>

#include "database_fixture.hpp"

using namespace steemit::chain::test;

uint32_t STEEMIT_TESTING_GENESIS_TIMESTAMP = 1431700000;

namespace steemit { namespace chain {

using std::cout;
using std::cerr;

clean_database_fixture::clean_database_fixture()
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

   open_database();

   db_plugin->logging = false;
   ahplugin->plugin_initialize( options );
   db_plugin->plugin_initialize( options );

   generate_block();
   db.set_hardfork( STEEMIT_NUM_HARDFORKS );
   generate_block();

   //ahplugin->plugin_startup();
   db_plugin->plugin_startup();

   vest( "initminer", 10000 );

   // Fill up the rest of the required miners
   for( int i = STEEMIT_NUM_INIT_MINERS; i < STEEMIT_MAX_MINERS; i++ )
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

   return;
}

clean_database_fixture::~clean_database_fixture()
{ try {
   // If we're unwinding due to an exception, don't do any more checks.
   // This way, boost test's last checkpoint tells us approximately where the error was.
   if( !std::uncaught_exception() )
   {
      BOOST_CHECK( db.get_node_properties().skip_flags == database::skip_nothing );
   }

   if( data_dir )
      db.close();
   return;
} FC_CAPTURE_AND_RETHROW() }

live_database_fixture::live_database_fixture()
{
   try
   {
      ilog( "Loading saved chain" );
      _chain_dir = fc::current_path() / "test_blockchain";
      FC_ASSERT( fc::exists( _chain_dir ), "Requires blockchain to test on in ./test_blockchain" );

      db.open( _chain_dir );
      graphene::time::now();

      auto ahplugin = app.register_plugin< steemit::account_history::account_history_plugin >();
      ahplugin->plugin_initialize( boost::program_options::variables_map() );

      validate_database();
      generate_block();

      ilog( "Done loading saved chain" );
   }
   FC_LOG_AND_RETHROW()
}

live_database_fixture::~live_database_fixture()
{
   try
   {
      // If we're unwinding due to an exception, don't do any more checks.
      // This way, boost test's last checkpoint tells us approximately where the error was.
      if( !std::uncaught_exception() )
      {
         BOOST_CHECK( db.get_node_properties().skip_flags == database::skip_nothing );
      }

      db.pop_block();
      db.close();
      return;
   }
   FC_LOG_AND_RETHROW()
}

fc::ecc::private_key database_fixture::generate_private_key(string seed)
{
   static const fc::ecc::private_key committee = fc::ecc::private_key::regenerate( fc::sha256::hash( string( "init_key" ) ) );
   if( seed == "init_key" )
      return committee;
   return fc::ecc::private_key::regenerate( fc::sha256::hash( seed ) );
}

string database_fixture::generate_anon_acct_name()
{
   // names of the form "anon-acct-x123" ; the "x" is necessary
   //    to workaround issue #46
   return "anon-acct-x" + std::to_string( anon_acct_count++ );
}

void database_fixture::open_database()
{
   if( !data_dir ) {
      data_dir = fc::temp_directory( graphene::utilities::temp_directory_path() );
      db.open( data_dir->path(), INITIAL_TEST_SUPPLY );
   }
}

void database_fixture::generate_block(uint32_t skip, const fc::ecc::private_key& key, int miss_blocks)
{
   skip |= default_skip;
   db_plugin->debug_generate_blocks( graphene::utilities::key_to_wif( key ), 1, skip, miss_blocks );
}

void database_fixture::generate_blocks( uint32_t block_count )
{
   auto produced = db_plugin->debug_generate_blocks( debug_key, block_count, default_skip, 0 );
   BOOST_REQUIRE( produced == block_count );
}

void database_fixture::generate_blocks(fc::time_point_sec timestamp, bool miss_intermediate_blocks)
{
   db_plugin->debug_generate_blocks_until( debug_key, timestamp, miss_intermediate_blocks, default_skip );
   BOOST_REQUIRE( ( db.head_block_time() - timestamp ).to_seconds() < STEEMIT_BLOCK_INTERVAL );
}

const account_object& database_fixture::account_create(
   const string& name,
   const string& creator,
   const private_key_type& creator_key,
   const share_type& fee,
   const public_key_type& key,
   const public_key_type& post_key,
   const string& json_metadata
   )
{
   try
   {
      account_create_operation op;
      op.new_account_name = name;
      op.creator = creator;
      op.fee = fee;
      op.owner = authority( 1, key, 1 );
      op.active = authority( 1, key, 1 );
      op.posting = authority( 1, post_key, 1 );
      op.memo_key = key;
      op.json_metadata = json_metadata;

      trx.operations.push_back( op );
      trx.set_expiration( db.head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION );
      trx.sign( creator_key, db.get_chain_id() );
      trx.validate();
      db.push_transaction( trx, 0 );
      trx.operations.clear();
      trx.signatures.clear();

      const account_object& acct = db.get_account( name );

      return acct;
   }
   FC_CAPTURE_AND_RETHROW( (name)(creator) )
}

const account_object& database_fixture::account_create(
   const string& name,
   const public_key_type& key,
   const public_key_type& post_key
)
{
   try
   {
      return account_create(
         name,
         STEEMIT_INIT_MINER_NAME,
         init_account_priv_key,
         100,
         key,
         post_key,
         "" );
   }
   FC_CAPTURE_AND_RETHROW( (name) );
}

const account_object& database_fixture::account_create(
   const string& name,
   const public_key_type& key
)
{
   return account_create( name, key, key );
}

const witness_object& database_fixture::witness_create(
   const string& owner,
   const private_key_type& owner_key,
   const string& url,
   const public_key_type& signing_key,
   const share_type& fee )
{
   try
   {
      witness_update_operation op;
      op.owner = owner;
      op.url = url;
      op.block_signing_key = signing_key;
      op.fee = asset( fee, STEEM_SYMBOL );

      trx.operations.push_back( op );
      trx.set_expiration( db.head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION );
      trx.sign( owner_key, db.get_chain_id() );
      trx.validate();
      db.push_transaction( trx, 0 );
      trx.operations.clear();
      trx.signatures.clear();

      return db.get_witness( owner );
   }
   FC_CAPTURE_AND_RETHROW( (owner)(url) )
}

void database_fixture::update_object( const fc::variant_object& vo )
{
   db_plugin->debug_update( vo, default_skip );
}

void database_fixture::fund(
   const string& account_name,
   const share_type& amount
   )
{
   try
   {
      transfer( STEEMIT_INIT_MINER_NAME, account_name, amount );

   } FC_CAPTURE_AND_RETHROW( (account_name)(amount) )
}

void database_fixture::fund(
   const string& account_name,
   const asset& amount
   )
{
   try
   {
      const auto& account = db.get_account( account_name );

      fc::mutable_variant_object vo;
      vo("_action", "update")("id", account.id);

      if( amount.symbol == STEEM_SYMBOL )
      {
         vo("balance", account.balance + amount);
         update_object( vo );

         const auto& gpo = db.get_dynamic_global_properties();
         vo = fc::mutable_variant_object();
         vo("_action", "update")("id", gpo.id)("current_supply", gpo.current_supply + amount );
         update_object( vo );
      }
      else if( amount.symbol == SBD_SYMBOL )
      {
         vo("sbd_balance", account.sbd_balance + amount );
         update_object( vo );

         const auto& gpo = db.get_dynamic_global_properties();
         vo = fc::mutable_variant_object();
         vo("_action", "update")("id", gpo.id)("current_sbd_supply", gpo.current_sbd_supply + amount );
         update_object( vo );

         const auto& median_feed = db.get_feed_history();
         if( median_feed.current_median_history.is_null() )
         {
            fc::mutable_variant_object vo;
            vo("_action", "update")("id", median_feed.id)("current_median_history", price( asset( 1, SBD_SYMBOL ), asset( 1, STEEM_SYMBOL) ) );
            update_object( vo );
         }
      }
      else
      {
         FC_ASSERT( false, "Can only fund account with TESTS or TBD" );
      }

      db.update_virtual_supply();
   }
   FC_CAPTURE_AND_RETHROW( (account_name)(amount) )
}

void database_fixture::convert(
   const string& account_name,
   const asset& amount )
{
   try
   {
      const account_object& account = db.get_account( account_name );


      if ( amount.symbol == STEEM_SYMBOL )
      {
         db.adjust_balance( account, -amount );
         db.adjust_balance( account, db.to_sbd( amount ) );
         db.adjust_supply( -amount );
         db.adjust_supply( db.to_sbd( amount ) );
      }
      else if ( amount.symbol == SBD_SYMBOL )
      {
         db.adjust_balance( account, -amount );
         db.adjust_balance( account, db.to_steem( amount ) );
         db.adjust_supply( -amount );
         db.adjust_supply( db.to_steem( amount ) );
      }
   } FC_CAPTURE_AND_RETHROW( (account_name)(amount) )
}

void database_fixture::transfer(
   const string& from,
   const string& to,
   const share_type& amount )
{
   try
   {
      transfer_operation op;
      op.from = from;
      op.to = to;
      op.amount = amount;

      trx.operations.push_back( op );
      trx.set_expiration( db.head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION );
      trx.validate();
      db.push_transaction( trx, ~0 );
      trx.operations.clear();
   } FC_CAPTURE_AND_RETHROW( (from)(to)(amount) )
}

void database_fixture::vest( const string& from, const share_type& amount )
{
   try
   {
      transfer_to_vesting_operation op;
      op.from = from;
      op.to = "";
      op.amount = asset( amount, STEEM_SYMBOL );

      trx.operations.push_back( op );
      trx.set_expiration( db.head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION );
      trx.validate();
      db.push_transaction( trx, ~0 );
      trx.operations.clear();
   } FC_CAPTURE_AND_RETHROW( (from)(amount) )
}

void database_fixture::proxy( const string& account, const string& proxy )
{
   try
   {
      account_witness_proxy_operation op;
      op.account = account;
      op.proxy = proxy;
      trx.operations.push_back( op );
      db.push_transaction( trx, ~0 );
      trx.operations.clear();
   } FC_CAPTURE_AND_RETHROW( (account)(proxy) )
}

void database_fixture::set_price_feed( const price& new_price )
{
   try
   {
      for ( int i = 1; i < 8; i++ )
      {
         feed_publish_operation op;
         op.publisher = STEEMIT_INIT_MINER_NAME + fc::to_string( i );
         op.exchange_rate = new_price;
         trx.operations.push_back( op );
         trx.set_expiration( db.head_block_time() + STEEMIT_MAX_TIME_UNTIL_EXPIRATION );
         db.push_transaction( trx, ~0 );
         trx.operations.clear();
      }
   } FC_CAPTURE_AND_RETHROW( (new_price) )

   generate_blocks( STEEMIT_BLOCKS_PER_HOUR );
   BOOST_REQUIRE(
#ifdef IS_TEST_NET
      !db.skip_price_feed_limit_check ||
#endif
      feed_history_id_type()( db ).current_median_history == new_price
   );
}

const asset& database_fixture::get_balance( const string& account_name )const
{
  return db.get_account( account_name ).balance;
}

void database_fixture::sign(signed_transaction& trx, const fc::ecc::private_key& key)
{
   trx.sign( key, db.get_chain_id() );
}

vector< operation > database_fixture::get_last_operations( uint32_t num_ops )
{
   vector< operation > ops;
   const auto& acc_hist_idx = db.get_index_type< account_history_index >().indices().get< by_id >();
   auto itr = acc_hist_idx.end();

   while( itr != acc_hist_idx.begin() && ops.size() < num_ops )
   {
      itr--;
      ops.push_back( itr->op(db).op );
   }

   return ops;
}

void database_fixture::validate_database( void )
{
   try
   {
      db.validate_invariants();
   }
   FC_LOG_AND_RETHROW();
}

namespace test {

bool _push_block( database& db, const signed_block& b, uint32_t skip_flags /* = 0 */ )
{
   return db.push_block( b, skip_flags);
}

void _push_transaction( database& db, const signed_transaction& tx, uint32_t skip_flags /* = 0 */ )
{ try {
   db.push_transaction( tx, skip_flags );
} FC_CAPTURE_AND_RETHROW((tx)) }

} // steemit::chain::test

} } // steemit::chain
