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
} FC_CAPTURE_AND_RETHROW() }

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

   db->open( data_dir->path(), data_dir->path(), INITIAL_TEST_SUPPLY, size );

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

      db->open( _chain_dir, _chain_dir );

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
   FC_LOG_AND_RETHROW()
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
   if( !data_dir ) {
      data_dir = fc::temp_directory( steem::utilities::temp_directory_path() );
      db->_log_hardforks = false;
      db->open( data_dir->path(), data_dir->path(), INITIAL_TEST_SUPPLY, 1024 * 1024 * 8 ); // 8 MB file for testing
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
      transfer( STEEM_INIT_MINER_NAME, account_name, amount );

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
      const account_object& account = db->get_account( account_name );


      if ( amount.symbol == STEEM_SYMBOL )
      {
         db->adjust_balance( account, -amount );
         db->adjust_balance( account, db->to_sbd( amount ) );
         db->adjust_supply( -amount );
         db->adjust_supply( db->to_sbd( amount ) );
      }
      else if ( amount.symbol == SBD_SYMBOL )
      {
         db->adjust_balance( account, -amount );
         db->adjust_balance( account, db->to_steem( amount ) );
         db->adjust_supply( -amount );
         db->adjust_supply( db->to_steem( amount ) );
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
   try
   {
      for ( int i = 1; i < 8; i++ )
      {
         feed_publish_operation op;
         op.publisher = STEEM_INIT_MINER_NAME + fc::to_string( i );
         op.exchange_rate = new_price;
         trx.operations.push_back( op );
         trx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         db->push_transaction( trx, ~0 );
         trx.operations.clear();
      }
   } FC_CAPTURE_AND_RETHROW( (new_price) )

   generate_blocks( STEEM_BLOCKS_PER_HOUR );
   BOOST_REQUIRE(
#ifdef IS_TEST_NET
      !db->skip_price_feed_limit_check ||
#endif
      db->get(feed_history_id_type()).current_median_history == new_price
   );
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
      set_price_feed( price( ASSET( "1.000 TESTS" ), ASSET( "1.000 TBD" ) ) );

      fund( account_name, 10 * 1000 * 1000 );
      convert( account_name, ASSET( "5000.000 TESTS" ) );

      op.symbol = database_fixture::name_to_asset_symbol(account_name, token_decimal_places);
      op.precision = op.symbol.decimals();
      op.smt_creation_fee = ASSET( "1000.000 TBD" );
      op.control_account = account_name;

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( key, db->get_chain_id() );

      db->push_transaction( tx, 0 );
   }
   FC_LOG_AND_RETHROW();

   return op.symbol;
}

#endif

json_rpc_database_fixture::json_rpc_database_fixture()
                        : t( "json_rpc_test_server" )
{
   url = "127.0.0.1";
   port = "9876";
}

json_rpc_database_fixture::~json_rpc_database_fixture()
{
   appbase::app().quit();
}

void json_rpc_database_fixture::launch_server( int initial_argc, char** initial_argv )
{
   t.async
   (
      [&]()
      {
         BOOST_CHECK( initial_argc >= 1 );

         std::string url_param = "--webserver-http-endpoint=" + url + ":" + port;

         char* argv[] = {  initial_argv[0],
                           ( char* )( "-d" ),
                           ( char* )( "testnet" ),
                           ( char* )( url_param.c_str() ),
                           nullptr
                        };
         int argc = sizeof(argv) / sizeof(char*) - 1;

         appbase::app().register_plugin< block_api_plugin >();
         appbase::app().register_plugin< database_api_plugin >();
         appbase::app().register_plugin< webserver_plugin >();
         appbase::app().register_plugin< condenser_api_plugin >();

         appbase::app().initialize<
            block_api_plugin,
            database_api_plugin,
            webserver_plugin,
            condenser_api_plugin
         >( argc, argv );

         appbase::app().startup();
         appbase::app().exec();
      }
   );

   fc::usleep( fc::seconds( delay ) );
}

fc::variant json_rpc_database_fixture::get_answer( std::string& request )
{
   const std::string method = "POST";
   const std::string http = "http://" + url;

   std::string body;
   fc::http::reply reply;

   connection.connect_to( fc::ip::endpoint( fc::ip::endpoint::from_string( url + ":" + port ) ) );
   reply = connection.request( method, http, request );
   BOOST_REQUIRE( reply.status == 200 );
   body = std::string( reply.body.begin(), reply.body.end() );

   return fc::json::from_string( body );
}

void json_rpc_database_fixture::review_answer( fc::variant& answer, int64_t code, bool is_warning, bool is_fail )
{
   fc::variant_object error;
   int64_t answer_code;

   if( is_fail )
   {
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
      BOOST_REQUIRE( answer.get_object().contains( "result" ) );
}

void json_rpc_database_fixture::make_array_request( std::string& request, int64_t code, bool is_warning, bool is_fail )
{
   fc::variant answer = get_answer( request );
   BOOST_REQUIRE( answer.is_array() );

   fc::variants array = answer.get_array();
   for( fc::variant obj : array )
   {
      review_answer( obj, code, is_warning, is_fail );
   }
}

fc::variant json_rpc_database_fixture::make_request( std::string& request, int64_t code, bool is_warning, bool is_fail )
{
   fc::variant answer = get_answer( request );
   BOOST_REQUIRE( answer.is_object() );

   review_answer( answer, code, is_warning, is_fail );

   return answer;
}

void json_rpc_database_fixture::make_positive_request( std::string& request )
{
   make_request( request, 0/*code*/, false/*is_warning*/, false/*is_fail*/);
}

void json_rpc_database_fixture::make_positive_request_with_id_analysis( std::string& request, bool treat_id_as_string )
{
   fc::variant answer = make_request( request, 0/*code*/, false/*is_warning*/, false/*is_fail*/);

   fc::variant_object vo = answer.get_object();
   BOOST_REQUIRE( vo.contains( "id" ) );

   int _type = vo[ "id" ].get_type();

   BOOST_REQUIRE(
                  ( _type == fc::variant::int64_type ) ||
                  ( _type == fc::variant::uint64_type ) ||
                  ( _type == fc::variant::string_type )
               );

   bool answer_treat_id_as_string =  _type == fc::variant::string_type;

   BOOST_REQUIRE( answer_treat_id_as_string == treat_id_as_string );
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
