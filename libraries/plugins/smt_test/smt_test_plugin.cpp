#include <steem/plugins/smt_test/smt_test_plugin.hpp>
#include <steem/plugins/smt_test/smt_test_objects.hpp>

#include <steem/chain/account_object.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/index.hpp>

#include <steem/protocol/smt_operations.hpp>

#define SMT_TEST_PLUGIN_NAI (SMT_MIN_NON_RESERVED_NAI * 10 + 6)

namespace steem { namespace plugins { namespace smt_test {

using namespace steem::protocol;

class smt_test_plugin_impl
{
   public:
      smt_test_plugin_impl( smt_test_plugin& _plugin ) :
         _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ),
         _self( _plugin ) {}

      void on_pre_apply_operation( const operation_notification& op_obj );
      void on_post_apply_operation( const operation_notification& op_obj );
      void clear_cache();
      void cache_auths( const account_authority_object& a );
      void update_key_lookup( const account_authority_object& a );

      flat_set< public_key_type >   cached_keys;
      chain::database&     _db;
      smt_test_plugin&              _self;
};

struct pre_operation_visitor
{
   smt_test_plugin_impl& _plugin;

   pre_operation_visitor( smt_test_plugin_impl& plugin ) : _plugin( plugin ) {}

   typedef void result_type;

   template< typename T >
   void operator()( const T& )const {}
};

struct post_operation_visitor
{
   smt_test_plugin_impl& _plugin;

   post_operation_visitor( smt_test_plugin_impl& plugin ) : _plugin( plugin ) {}

   typedef void result_type;

   template< typename T >
   void operator()( const T& )const {}
};

void smt_test_plugin_impl::on_pre_apply_operation( const operation_notification& note )
{
   note.op.visit( pre_operation_visitor( *this ) );
}

void smt_test_plugin_impl::on_post_apply_operation( const operation_notification& note )
{
   note.op.visit( post_operation_visitor( *this ) );
}

#ifdef STEEM_ENABLE_SMT

void test_alpha()
{
   vector<operation>  operations;

   smt_capped_generation_policy gpolicy;
   uint64_t max_supply = STEEM_MAX_SHARE_SUPPLY / 6000;

   // set steem unit, total is 100 STEEM-satoshis = 0.1 STEEM
   gpolicy.pre_soft_cap_unit.steem_unit.emplace( "founder-a",   7 );
   gpolicy.pre_soft_cap_unit.steem_unit.emplace( "founder-b",  23 );
   gpolicy.pre_soft_cap_unit.steem_unit.emplace( "founder-c",  70 );

   // set token unit, total is 6 token-satoshis = 0.0006 ALPHA
   gpolicy.pre_soft_cap_unit.token_unit.emplace( "$from", 5 );
   gpolicy.pre_soft_cap_unit.token_unit.emplace( "founder-d", 1 );

   // no soft cap -> no soft cap unit
   gpolicy.post_soft_cap_unit.steem_unit.clear();
   gpolicy.post_soft_cap_unit.token_unit.clear();

   gpolicy.min_steem_units_commitment.fillin_nonhidden_value( 1 );
   gpolicy.hard_cap_steem_units_commitment.fillin_nonhidden_value( max_supply );

   gpolicy.soft_cap_percent = STEEM_100_PERCENT;

   // .0006 ALPHA / 0.1 STEEM -> 1000 token-units / steem-unit
   gpolicy.min_unit_ratio = 1000;
   gpolicy.max_unit_ratio = 1000;


   smt_setup_operation setup_op;
   setup_op.control_account = "alpha";

   asset_symbol_type alpha_symbol = asset_symbol_type::from_nai( SMT_TEST_PLUGIN_NAI, 4 );
   setup_op.decimal_places = alpha_symbol.decimals();
   setup_op.symbol = alpha_symbol;

   setup_op.initial_generation_policy = gpolicy;

   setup_op.generation_begin_time  = fc::variant( "2017-08-10T00:00:00" ).as< fc::time_point_sec >();
   setup_op.generation_end_time    = fc::variant( "2017-08-17T00:00:00" ).as< fc::time_point_sec >();
   setup_op.announced_launch_time  = fc::variant( "2017-08-21T00:00:00" ).as< fc::time_point_sec >();
   setup_op.launch_expiration_time = fc::variant( "2017-08-22T00:00:00" ).as< fc::time_point_sec >();

   setup_op.validate();

   smt_cap_reveal_operation reveal_min_op;
   reveal_min_op.control_account = "alpha";
   reveal_min_op.cap.fillin_nonhidden_value( 1 );
   smt_cap_reveal_operation reveal_cap_op;
   reveal_cap_op.control_account = "alpha";
   reveal_cap_op.cap.fillin_nonhidden_value( max_supply );

   operations.push_back( setup_op );
   operations.push_back( reveal_min_op );
   operations.push_back( reveal_cap_op );

   ilog( "ALPHA example tx: ${tx}", ("tx", operations) );
}

void test_beta()
{
   vector<operation>  operations;

   smt_capped_generation_policy gpolicy;

   // set steem unit, total is 100 STEEM-satoshis = 0.1 STEEM
   gpolicy.pre_soft_cap_unit.steem_unit.emplace( "fred"  , 3 );
   gpolicy.pre_soft_cap_unit.steem_unit.emplace( "george", 2 );

   // set token unit, total is 6 token-satoshis = 0.0006 ALPHA
   gpolicy.pre_soft_cap_unit.token_unit.emplace( "$from" , 7 );
   gpolicy.pre_soft_cap_unit.token_unit.emplace( "george", 1 );
   gpolicy.pre_soft_cap_unit.token_unit.emplace( "henry" , 2 );

   // no soft cap -> no soft cap unit
   gpolicy.post_soft_cap_unit.steem_unit.clear();
   gpolicy.post_soft_cap_unit.token_unit.clear();

   gpolicy.min_steem_units_commitment.fillin_nonhidden_value( 5000000 );
   gpolicy.hard_cap_steem_units_commitment.fillin_nonhidden_value( 30000000 );

   gpolicy.soft_cap_percent = STEEM_100_PERCENT;

   // .0006 ALPHA / 0.1 STEEM -> 1000 token-units / steem-unit
   gpolicy.min_unit_ratio = 50;
   gpolicy.max_unit_ratio = 100;

   smt_setup_operation setup_op;
   setup_op.control_account = "beta";

   asset_symbol_type alpha_symbol = asset_symbol_type::from_nai( SMT_TEST_PLUGIN_NAI, 4 );
   setup_op.decimal_places = alpha_symbol.decimals();
   setup_op.symbol = alpha_symbol;

   setup_op.initial_generation_policy = gpolicy;

   setup_op.generation_begin_time  = fc::variant( "2017-06-01T00:00:00" ).as< fc::time_point_sec >();
   setup_op.generation_end_time    = fc::variant( "2017-06-30T00:00:00" ).as< fc::time_point_sec >();
   setup_op.announced_launch_time  = fc::variant( "2017-07-01T00:00:00" ).as< fc::time_point_sec >();
   setup_op.launch_expiration_time = fc::variant( "2017-07-02T00:00:00" ).as< fc::time_point_sec >();

   setup_op.validate();

   smt_cap_reveal_operation reveal_min_op;
   reveal_min_op.control_account = "beta";
   reveal_min_op.cap.fillin_nonhidden_value( 5000000 );
   smt_cap_reveal_operation reveal_cap_op;
   reveal_cap_op.control_account = "beta";
   reveal_cap_op.cap.fillin_nonhidden_value( 30000000 );

   operations.push_back( setup_op );
   operations.push_back( reveal_min_op );
   operations.push_back( reveal_cap_op );

   ilog( "BETA example tx: ${tx}", ("tx", operations) );
}

void test_delta()
{
   vector<operation>  operations;

   smt_capped_generation_policy gpolicy;

   // set steem unit, total is 1 STEEM-satoshi = 0.001 STEEM
   gpolicy.pre_soft_cap_unit.steem_unit.emplace( "founder", 1 );

   // set token unit, total is 10,000 token-satoshis = 0.10000 DELTA
   gpolicy.pre_soft_cap_unit.token_unit.emplace( "founder" , 10000 );

   // no soft cap -> no soft cap unit
   gpolicy.post_soft_cap_unit.steem_unit.clear();
   gpolicy.post_soft_cap_unit.token_unit.clear();

   gpolicy.min_steem_units_commitment.fillin_nonhidden_value(      10000000 );
   gpolicy.hard_cap_steem_units_commitment.fillin_nonhidden_value( 10000000 );

   gpolicy.soft_cap_percent = STEEM_100_PERCENT;

   // .001 STEEM / .100000 DELTA -> 100 DELTA / STEEM
   gpolicy.min_unit_ratio = 1000;
   gpolicy.max_unit_ratio = 1000;

   smt_setup_operation setup_op;
   setup_op.control_account = "delta";

   asset_symbol_type delta_symbol = asset_symbol_type::from_nai( SMT_TEST_PLUGIN_NAI, 5 );
   setup_op.decimal_places = delta_symbol.decimals();
   setup_op.symbol = delta_symbol;

   setup_op.initial_generation_policy = gpolicy;

   setup_op.generation_begin_time  = fc::variant( "2017-06-01T00:00:00" ).as< fc::time_point_sec >();
   setup_op.generation_end_time    = fc::variant( "2017-06-30T00:00:00" ).as< fc::time_point_sec >();
   setup_op.announced_launch_time  = fc::variant( "2017-07-01T00:00:00" ).as< fc::time_point_sec >();
   setup_op.launch_expiration_time = fc::variant( "2017-07-02T00:00:00" ).as< fc::time_point_sec >();

   setup_op.validate();

   smt_cap_reveal_operation reveal_min_op;
   reveal_min_op.control_account = "delta";
   reveal_min_op.cap.fillin_nonhidden_value( 10000000 );
   smt_cap_reveal_operation reveal_cap_op;
   reveal_cap_op.control_account = "delta";
   reveal_cap_op.cap.fillin_nonhidden_value( 10000000 );

   operations.push_back( setup_op );
   operations.push_back( reveal_min_op );
   operations.push_back( reveal_cap_op );

   ilog( "DELTA example tx: ${tx}", ("tx", operations) );
}

void dump_operation_json()
{
   test_alpha();
   test_beta();
   test_delta();
}
#else
void dump_operation_json()
{
}
#endif

smt_test_plugin::smt_test_plugin()
{
   try
   {
      dump_operation_json();
   }
   catch( const fc::exception& e )
   {
      elog( "Caught exception:\n${e}", ("e", e) );
   }
}

smt_test_plugin::~smt_test_plugin() {}

void smt_test_plugin::set_program_options( options_description& cli, options_description& cfg ){}

void smt_test_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   my = std::make_unique< smt_test_plugin_impl >( *this );
   try
   {
      ilog( "Initializing smt_test plugin" );
      chain::database& db = appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db();

      db.add_pre_apply_operation_handler( [&]( const operation_notification& note ){ my->on_pre_apply_operation( note ); }, *this, 0 );
      db.add_post_apply_operation_handler( [&]( const operation_notification& note ){ my->on_post_apply_operation( note ); }, *this, 0 );

      // add_plugin_index< key_lookup_index >(db);
   }
   FC_CAPTURE_AND_RETHROW()
}

void smt_test_plugin::plugin_startup() {}

void smt_test_plugin::plugin_shutdown() {}

} } } // steem::plugins::smt_test
