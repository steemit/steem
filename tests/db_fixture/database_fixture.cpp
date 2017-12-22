#include <boost/test/unit_test.hpp>
#include <boost/program_options.hpp>

#include <steem/utilities/tempdir.hpp>

#include <steem/chain/steem_objects.hpp>
#include <steem/chain/history_object.hpp>
#include <steem/plugins/account_history/account_history_plugin.hpp>
#include <steem/plugins/witness/witness_plugin.hpp>
#include <steem/plugins/chain/chain_plugin.hpp>
#include <steem/plugins/webserver/webserver_plugin.hpp>
#include <steem/plugins/condenser_api/condenser_api_plugin.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/smart_ref_impl.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>

#include "database_fixture.hpp"

//using namespace steem::chain::test;

uint32_t STEEM_TESTING_GENESIS_TIMESTAMP = 1431700000;

using namespace steem::plugins::webserver;
using namespace steem::plugins::database_api;
using namespace steem::plugins::block_api;
using steem::plugins::condenser_api::condenser_api_plugin;

namespace steem { namespace chain {

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

   appbase::app().register_plugin< steem::plugins::account_history::account_history_plugin >();
   db_plugin = &appbase::app().register_plugin< steem::plugins::debug_node::debug_node_plugin >();
   appbase::app().register_plugin< steem::plugins::witness::witness_plugin >();

   db_plugin->logging = false;
   appbase::app().initialize<
      steem::plugins::account_history::account_history_plugin,
      steem::plugins::debug_node::debug_node_plugin,
      steem::plugins::witness::witness_plugin
      >( argc, argv );

   db = &appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db();
   BOOST_REQUIRE( db );

   init_account_pub_key = init_account_priv_key.get_public_key();

   open_database();

   generate_block();
   db->set_hardfork( STEEM_BLOCKCHAIN_VERSION.minor() );
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
      BOOST_CHECK( db->get_node_properties().skip_flags == database::skip_nothing );
   }

   if( data_dir )
      db->wipe( data_dir->path(), data_dir->path(), true );
   return;
} FC_CAPTURE_AND_LOG( () )
   exit(1);
}

void clean_database_fixture::resize_shared_mem( uint64_t size )
{
   db->wipe( data_dir->path(), data_dir->path(), true );
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
   init_account_pub_key = init_account_priv_key.get_public_key();

   {
      database::open_args args;
      args.data_dir = data_dir->path();
      args.shared_mem_dir = args.data_dir;
      args.initial_supply = INITIAL_TEST_SUPPLY;
      args.shared_file_size = size;
      db->open( args );
   }

   boost::program_options::variables_map options;


   generate_block();
   db->set_hardfork( STEEM_BLOCKCHAIN_VERSION.minor() );
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
}

live_database_fixture::live_database_fixture()
{
   try
   {
      int argc = boost::unit_test::framework::master_test_suite().argc;
      char** argv = boost::unit_test::framework::master_test_suite().argv;

      ilog( "Loading saved chain" );
      _chain_dir = fc::current_path() / "test_blockchain";
      FC_ASSERT( fc::exists( _chain_dir ), "Requires blockchain to test on in ./test_blockchain" );

      appbase::app().register_plugin< steem::plugins::account_history::account_history_plugin >();
      appbase::app().initialize<
         steem::plugins::account_history::account_history_plugin
         >( argc, argv );

      db = &appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db();
      BOOST_REQUIRE( db );

      {
         database::open_args args;
         args.data_dir = _chain_dir;
         args.shared_mem_dir = args.data_dir;
         db->open( args );
      }

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
         BOOST_CHECK( db->get_node_properties().skip_flags == database::skip_nothing );
      }

      db->pop_block();
      db->close();
      return;
   }
   FC_CAPTURE_AND_LOG( () )
   exit(1);
}

fc::ecc::private_key database_fixture::generate_private_key(string seed)
{
   static const fc::ecc::private_key committee = fc::ecc::private_key::regenerate( fc::sha256::hash( string( "init_key" ) ) );
   if( seed == "init_key" )
      return committee;
   return fc::ecc::private_key::regenerate( fc::sha256::hash( seed ) );
}

asset_symbol_type database_fixture::name_to_asset_symbol( const std::string& name, uint8_t decimal_places )
{
   // Deterministically turn a name into an asset symbol
   // Example:
   // alice -> sha256(alice) -> 2bd806c9... -> 2bd806c9 -> low 27 bits is 64489161 -> add check digit -> @@644891612

   uint32_t h0 = (boost::endian::native_to_big( fc::sha256::hash( name )._hash[0] ) >> 32) & 0x7FFFFFF;
   FC_ASSERT( decimal_places <= STEEM_ASSET_MAX_DECIMALS, "Invalid decimal_places" );
   while( h0 > SMT_MAX_NAI )
      h0 -= SMT_MAX_NAI;
   while( h0 < SMT_MIN_NAI )
      h0 += SMT_MIN_NAI;
   uint32_t asset_num = (h0 << 5) | 0x10 | decimal_places;
   return asset_symbol_type::from_asset_num( asset_num );
}

string database_fixture::generate_anon_acct_name()
{
   // names of the form "anon-acct-x123" ; the "x" is necessary
   //    to workaround issue #46
   return "anon-acct-x" + std::to_string( anon_acct_count++ );
}

void database_fixture::open_database()
{
   if( !data_dir )
   {
      data_dir = fc::temp_directory( steem::utilities::temp_directory_path() );
      db->_log_hardforks = false;

      database::open_args args;
      args.data_dir = data_dir->path();
      args.shared_mem_dir = args.data_dir;
      args.initial_supply = INITIAL_TEST_SUPPLY;
      args.shared_file_size = 1024 * 1024 * 8;     // 8MB file for testing
      db->open(args);
   }
}

void database_fixture::generate_block(uint32_t skip, const fc::ecc::private_key& key, int miss_blocks)
{
   skip |= default_skip;
   db_plugin->debug_generate_blocks( steem::utilities::key_to_wif( key ), 1, skip, miss_blocks );
}

void database_fixture::generate_blocks( uint32_t block_count )
{
   auto produced = db_plugin->debug_generate_blocks( debug_key, block_count, default_skip, 0 );
   BOOST_REQUIRE( produced == block_count );
}

void database_fixture::generate_blocks(fc::time_point_sec timestamp, bool miss_intermediate_blocks)
{
   db_plugin->debug_generate_blocks_until( debug_key, timestamp, miss_intermediate_blocks, default_skip );
   BOOST_REQUIRE( ( db->head_block_time() - timestamp ).to_seconds() < STEEM_BLOCK_INTERVAL );
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
      if( db->has_hardfork( STEEM_HARDFORK_0_17 ) )
      {
         account_create_with_delegation_operation op;
         op.new_account_name = name;
         op.creator = creator;
         op.fee = asset( fee, STEEM_SYMBOL );
         op.delegation = asset( 0, VESTS_SYMBOL );
         op.owner = authority( 1, key, 1 );
         op.active = authority( 1, key, 1 );
         op.posting = authority( 1, post_key, 1 );
         op.memo_key = key;
         op.json_metadata = json_metadata;

         trx.operations.push_back( op );
      }
      else
      {
         account_create_operation op;
         op.new_account_name = name;
         op.creator = creator;
         op.fee = asset( fee, STEEM_SYMBOL );
         op.owner = authority( 1, key, 1 );
         op.active = authority( 1, key, 1 );
         op.posting = authority( 1, post_key, 1 );
         op.memo_key = key;
         op.json_metadata = json_metadata;

         trx.operations.push_back( op );
      }

      trx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      trx.sign( creator_key, db->get_chain_id() );
      trx.validate();
      db->push_transaction( trx, 0 );
      trx.operations.clear();
      trx.signatures.clear();

      const account_object& acct = db->get_account( name );

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
         STEEM_INIT_MINER_NAME,
         init_account_priv_key,
         std::max( db->get_witness_schedule_object().median_props.account_creation_fee.amount * STEEM_CREATE_ACCOUNT_WITH_STEEM_MODIFIER, share_type( 100 ) ),
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
      trx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      trx.sign( owner_key, db->get_chain_id() );
      trx.validate();
      db->push_transaction( trx, 0 );
      trx.operations.clear();
      trx.signatures.clear();

      return db->get_witness( owner );
   }
   FC_CAPTURE_AND_RETHROW( (owner)(url) )
}

void database_fixture::fund(
   const string& account_name,
   const share_type& amount
   )
{
   try
   {
      transfer( STEEM_INIT_MINER_NAME, account_name, asset( amount, STEEM_SYMBOL ) );

   } FC_CAPTURE_AND_RETHROW( (account_name)(amount) )
}

void database_fixture::fund(
   const string& account_name,
   const asset& amount
   )
{
   try
   {
      db_plugin->debug_update( [=]( database& db)
      {
         db.modify( db.get_account( account_name ), [&]( account_object& a )
         {
            if( amount.symbol == STEEM_SYMBOL )
               a.balance += amount;
            else if( amount.symbol == SBD_SYMBOL )
            {
               a.sbd_balance += amount;
               a.sbd_seconds_last_update = db.head_block_time();
            }
         });

         db.modify( db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo )
         {
            if( amount.symbol == STEEM_SYMBOL )
               gpo.current_supply += amount;
            else if( amount.symbol == SBD_SYMBOL )
               gpo.current_sbd_supply += amount;
         });

         if( amount.symbol == SBD_SYMBOL )
         {
            const auto& median_feed = db.get_feed_history();
            if( median_feed.current_median_history.is_null() )
               db.modify( median_feed, [&]( feed_history_object& f )
               {
                  f.current_median_history = price( asset( 1, SBD_SYMBOL ), asset( 1, STEEM_SYMBOL ) );
               });
         }

         db.update_virtual_supply();
      }, default_skip );
   }
   FC_CAPTURE_AND_RETHROW( (account_name)(amount) )
}

void database_fixture::convert(
   const string& account_name,
   const asset& amount )
{
   try
   {
      if ( amount.symbol == STEEM_SYMBOL )
      {
         db->adjust_balance( account_name, -amount );
         db->adjust_balance( account_name, db->to_sbd( amount ) );
         db->adjust_supply( -amount );
         db->adjust_supply( db->to_sbd( amount ) );
      }
      else if ( amount.symbol == SBD_SYMBOL )
      {
         db->adjust_balance( account_name, -amount );
         db->adjust_balance( account_name, db->to_steem( amount ) );
         db->adjust_supply( -amount );
         db->adjust_supply( db->to_steem( amount ) );
      }
   } FC_CAPTURE_AND_RETHROW( (account_name)(amount) )
}

void database_fixture::transfer(
   const string& from,
   const string& to,
   const asset& amount )
{
   try
   {
      transfer_operation op;
      op.from = from;
      op.to = to;
      op.amount = amount;

      trx.operations.push_back( op );
      trx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      trx.validate();
      db->push_transaction( trx, ~0 );
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
      trx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      trx.validate();
      db->push_transaction( trx, ~0 );
      trx.operations.clear();
   } FC_CAPTURE_AND_RETHROW( (from)(amount) )
}

void database_fixture::vest( const string& account, const asset& amount )
{
   if( amount.symbol != STEEM_SYMBOL )
      return;

   db_plugin->debug_update( [=]( database& db )
   {
      db.modify( db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo )
      {
         gpo.current_supply += amount;
      });

      db.create_vesting( db.get_account( account ), amount );

      db.update_virtual_supply();
   }, default_skip );
}

void database_fixture::proxy( const string& account, const string& proxy )
{
   try
   {
      account_witness_proxy_operation op;
      op.account = account;
      op.proxy = proxy;
      trx.operations.push_back( op );
      db->push_transaction( trx, ~0 );
      trx.operations.clear();
   } FC_CAPTURE_AND_RETHROW( (account)(proxy) )
}

void database_fixture::set_price_feed( const price& new_price )
{
   flat_map< string, vector< char > > props;
   props[ "sbd_exchange_rate" ] = fc::raw::pack( new_price );

   set_witness_props( props );

   BOOST_REQUIRE(
#ifdef IS_TEST_NET
      !db->skip_price_feed_limit_check ||
#endif
      db->get(feed_history_id_type()).current_median_history == new_price
   );
}

void database_fixture::set_witness_props( const flat_map< string, vector< char > >& props )
{
   for( size_t i = 1; i < 8; i++ )
   {
      witness_set_properties_operation op;
      op.owner = STEEM_INIT_MINER_NAME + fc::to_string( i );
      op.props = props;

      if( op.props.find( "key" ) == op.props.end() )
      {
         op.props[ "key" ] = fc::raw::pack( init_account_pub_key );
      }

      trx.operations.push_back( op );
      trx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      db->push_transaction( trx, ~0 );
      trx.operations.clear();
   }

   generate_blocks( STEEM_BLOCKS_PER_HOUR );
}

const asset& database_fixture::get_balance( const string& account_name )const
{
  return db->get_account( account_name ).balance;
}

void database_fixture::sign(signed_transaction& trx, const fc::ecc::private_key& key)
{
   trx.sign( key, db->get_chain_id() );
}

vector< operation > database_fixture::get_last_operations( uint32_t num_ops )
{
   vector< operation > ops;
   const auto& acc_hist_idx = db->get_index< account_history_index >().indices().get< by_id >();
   auto itr = acc_hist_idx.end();

   while( itr != acc_hist_idx.begin() && ops.size() < num_ops )
   {
      itr--;
      ops.push_back( fc::raw::unpack< steem::chain::operation >( db->get(itr->op).serialized_op ) );
   }

   return ops;
}

void database_fixture::validate_database( void )
{
   try
   {
      db->validate_invariants();
   }
   FC_LOG_AND_RETHROW();
}

#ifdef STEEM_ENABLE_SMT

smt_database_fixture::smt_database_fixture()
{

}

smt_database_fixture::~smt_database_fixture()
{

}

asset_symbol_type smt_database_fixture::create_smt( signed_transaction& tx, const string& account_name, const fc::ecc::private_key& key,
   uint8_t token_decimal_places )
{
   smt_create_operation op;

   try
   {
      fund( account_name, 10 * 1000 * 1000 );
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      convert( account_name, ASSET( "5000.000 TESTS" ) );

      // The list of available nais is not dependent on SMT desired precision (token_decimal_places).
      auto available_nais =  db->get_smt_next_identifier();
      FC_ASSERT( available_nais.size() > 0, "No available nai returned by get_smt_next_identifier." );
      const asset_symbol_type& new_nai = available_nais[0];
      // Note that token's precision is needed now, when creating actual symbol.
      op.symbol = asset_symbol_type::from_nai( new_nai.to_nai(), token_decimal_places );
      op.precision = op.symbol.decimals();
      op.smt_creation_fee = ASSET( "1000.000 TBD" );
      op.control_account = account_name;

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      generate_block();
   }
   FC_LOG_AND_RETHROW();

   return op.symbol;
}

void sub_set_create_op(smt_create_operation* op, account_name_type control_acount)
{
   op->precision = op->symbol.decimals();
   op->smt_creation_fee = ASSET( "1000.000 TBD" );
   op->control_account = control_acount;
}

void set_create_op(smt_create_operation* op, account_name_type control_account, std::string token_name, uint8_t token_decimal_places)
{
   op->symbol = database_fixture::name_to_asset_symbol(token_name, token_decimal_places);
   sub_set_create_op(op, control_account);
}

void set_create_op(smt_create_operation* op, account_name_type control_account, uint32_t token_nai, uint8_t token_decimal_places)
{
   op->symbol = asset_symbol_type::from_nai(token_nai, token_decimal_places);
   sub_set_create_op(op, control_account);
}

std::array<asset_symbol_type, 3>
smt_database_fixture::create_smt_3(const char* control_account_name, const fc::ecc::private_key& key)
{
   smt_create_operation op0;
   smt_create_operation op1;
   smt_create_operation op2;
   std::string token_name(control_account_name);
   token_name.resize(2);

   try
   {
      fund( control_account_name, 10 * 1000 * 1000 );
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );
      convert( control_account_name, ASSET( "5000.000 TESTS" ) );

      set_create_op(&op0, control_account_name, token_name + "0", 0);
      set_create_op(&op1, control_account_name, token_name + "1", 1);
      set_create_op(&op2, control_account_name, token_name + "2", 1);

      signed_transaction tx;
      tx.operations.push_back( op0 );
      tx.operations.push_back( op1 );
      tx.operations.push_back( op2 );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_block();

      std::array<asset_symbol_type, 3> retVal = {op0.symbol, op1.symbol, op2.symbol};
      return retVal;
   }
   FC_LOG_AND_RETHROW();
}

void push_invalid_operation(const operation& invalid_op, const fc::ecc::private_key& key, database* db)
{
   signed_transaction tx;
   tx.operations.push_back( invalid_op );
   tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   tx.sign( key, db->get_chain_id() );
   STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::assert_exception );
}

void smt_database_fixture::create_invalid_smt( const char* control_account_name, const fc::ecc::private_key& key )
{
   // Fail due to precision too big.
   smt_create_operation op_precision;
   STEEM_REQUIRE_THROW( set_create_op(&op_precision, control_account_name, "smt", STEEM_ASSET_MAX_DECIMALS + 1), fc::assert_exception );
}

void smt_database_fixture::create_conflicting_smt( const asset_symbol_type existing_smt, const char* control_account_name,
   const fc::ecc::private_key& key )
{
   // Fail due to the same nai & precision.
   smt_create_operation op_same;
   set_create_op(&op_same, control_account_name, existing_smt.to_nai(), existing_smt.decimals());
   push_invalid_operation(op_same, key, db);
   // Fail due to the same nai (though different precision).
   smt_create_operation op_same_nai;
   set_create_op(&op_same_nai, control_account_name, existing_smt.to_nai(), existing_smt.decimals() == 0 ? 1 : 0);
   push_invalid_operation(op_same_nai, key, db);
}

#endif

json_rpc_database_fixture::json_rpc_database_fixture()
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
   appbase::app().register_plugin< steem::plugins::witness::witness_plugin >();
   rpc_plugin = &appbase::app().register_plugin< steem::plugins::json_rpc::json_rpc_plugin >();
   appbase::app().register_plugin< steem::plugins::block_api::block_api_plugin >();
   appbase::app().register_plugin< steem::plugins::database_api::database_api_plugin >();
   appbase::app().register_plugin< steem::plugins::condenser_api::condenser_api_plugin >();

   db_plugin->logging = false;
   appbase::app().initialize<
      steem::plugins::account_history::account_history_plugin,
      steem::plugins::debug_node::debug_node_plugin,
      steem::plugins::witness::witness_plugin,
      steem::plugins::json_rpc::json_rpc_plugin,
      steem::plugins::block_api::block_api_plugin,
      steem::plugins::database_api::database_api_plugin,
      steem::plugins::condenser_api::condenser_api_plugin
      >( argc, argv );

   appbase::app().get_plugin< steem::plugins::condenser_api::condenser_api_plugin >().plugin_startup();

   db = &appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db();
   BOOST_REQUIRE( db );

   init_account_pub_key = init_account_priv_key.get_public_key();

   open_database();

   generate_block();
   db->set_hardfork( STEEM_BLOCKCHAIN_VERSION.minor() );
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
   } catch ( const fc::exception& e )
   {
      edump( (e.to_detail_string()) );
      throw;
   }

   return;
}

json_rpc_database_fixture::~json_rpc_database_fixture()
{ try {
   // If we're unwinding due to an exception, don't do any more checks.
   // This way, boost test's last checkpoint tells us approximately where the error was.
   if( !std::uncaught_exception() )
   {
      BOOST_CHECK( db->get_node_properties().skip_flags == database::skip_nothing );
   }

   if( data_dir )
      db->wipe( data_dir->path(), data_dir->path(), true );
   return;
} FC_CAPTURE_AND_RETHROW() }

fc::variant json_rpc_database_fixture::get_answer( std::string& request )
{
   return fc::json::from_string( rpc_plugin->call( request ) );
}

void check_id_equal( const fc::variant& id_a, const fc::variant& id_b )
{
   BOOST_REQUIRE( id_a.get_type() == id_b.get_type() );

   switch( id_a.get_type() )
   {
      case fc::variant::int64_type:
         BOOST_REQUIRE( id_a.as_int64() == id_b.as_int64() );
         break;
      case fc::variant::uint64_type:
         BOOST_REQUIRE( id_a.as_uint64() == id_b.as_uint64() );
         break;
      case fc::variant::string_type:
         BOOST_REQUIRE( id_a.as_string() == id_b.as_string() );
         break;
      case fc::variant::null_type:
         break;
      default:
         BOOST_REQUIRE( false );
   }
}

void json_rpc_database_fixture::review_answer( fc::variant& answer, int64_t code, bool is_warning, bool is_fail, fc::optional< fc::variant > id )
{
   fc::variant_object error;
   int64_t answer_code;

   if( is_fail )
   {
      if( id.valid() && code != JSON_RPC_INVALID_REQUEST )
      {
         BOOST_REQUIRE( answer.get_object().contains( "id" ) );
         check_id_equal( answer[ "id" ], *id );
      }

      BOOST_REQUIRE( answer.get_object().contains( "error" ) );
      BOOST_REQUIRE( answer["error"].is_object() );
      error = answer["error"].get_object();
      BOOST_REQUIRE( error.contains( "code" ) );
      BOOST_REQUIRE( error["code"].is_int64() );
      answer_code = error["code"].as_int64();
      BOOST_REQUIRE( answer_code == code );
      if( is_warning )
         BOOST_TEST_MESSAGE( error["message"].as_string() );
   }
   else
   {
      BOOST_REQUIRE( answer.get_object().contains( "result" ) );
      BOOST_REQUIRE( answer.get_object().contains( "id" ) );
      if( id.valid() )
         check_id_equal( answer[ "id" ], *id );
   }
}

void json_rpc_database_fixture::make_array_request( std::string& request, int64_t code, bool is_warning, bool is_fail )
{
   fc::variant answer = get_answer( request );
   BOOST_REQUIRE( answer.is_array() );

   fc::variants request_array = fc::json::from_string( request ).get_array();
   fc::variants array = answer.get_array();

   BOOST_REQUIRE( array.size() == request_array.size() );
   for( size_t i = 0; i < array.size(); ++i )
   {
      fc::optional< fc::variant > id;

      try
      {
         id = request_array[i][ "id" ];
      }
      catch( ... ) {}

      review_answer( array[i], code, is_warning, is_fail, id );
   }
}

fc::variant json_rpc_database_fixture::make_request( std::string& request, int64_t code, bool is_warning, bool is_fail )
{
   fc::variant answer = get_answer( request );
   BOOST_REQUIRE( answer.is_object() );
   fc::optional< fc::variant > id;

   try
   {
      id = fc::json::from_string( request ).get_object()[ "id" ];
   }
   catch( ... ) {}

   review_answer( answer, code, is_warning, is_fail, id );

   return answer;
}

void json_rpc_database_fixture::make_positive_request( std::string& request )
{
   make_request( request, 0/*code*/, false/*is_warning*/, false/*is_fail*/);
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

} // steem::chain::test

} } // steem::chain
