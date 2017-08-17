#include <steemit/plugins/smt_test/smt_test_plugin.hpp>
#include <steemit/plugins/smt_test/smt_test_objects.hpp>

#include <steemit/chain/account_object.hpp>
#include <steemit/chain/database.hpp>
#include <steemit/chain/index.hpp>
#include <steemit/chain/operation_notification.hpp>

#include <graphene/schema/schema.hpp>
#include <graphene/schema/schema_impl.hpp>

#include <steemit/protocol/smt_operations.hpp>

namespace steemit { namespace plugins { namespace smt_test {

class smt_test_plugin_impl
{
   public:
      smt_test_plugin_impl( smt_test_plugin& _plugin ) :
         _db( appbase::app().get_plugin< steemit::plugins::chain::chain_plugin >().db() ),
         _self( _plugin ) {}

      void pre_operation( const operation_notification& op_obj );
      void post_operation( const operation_notification& op_obj );
      void clear_cache();
      void cache_auths( const account_authority_object& a );
      void update_key_lookup( const account_authority_object& a );

      flat_set< public_key_type >   cached_keys;
      steemit::chain::database&     _db;
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

void smt_test_plugin_impl::pre_operation( const operation_notification& note )
{
   note.op.visit( pre_operation_visitor( *this ) );
}

void smt_test_plugin_impl::post_operation( const operation_notification& note )
{
   note.op.visit( post_operation_visitor( *this ) );
}

void test_alpha()
{
   vector<operation>  operations;

   smt_capped_generation_policy gpolicy;
   uint64_t max_supply = STEEMIT_MAX_SHARE_SUPPLY / 6000;

   // set steem unit, total is 100 STEEM-satoshis = 0.1 STEEM
   gpolicy.pre_soft_cap_unit.steem_unit.emplace( "founder_a",   7 );
   gpolicy.pre_soft_cap_unit.steem_unit.emplace( "founder_b",  23 );
   gpolicy.pre_soft_cap_unit.steem_unit.emplace( "founder_c",  70 );

   // set token unit, total is 6 token-satoshis = 0.0006 ALPHA
   gpolicy.pre_soft_cap_unit.token_unit.emplace( "$from", 5 );
   gpolicy.pre_soft_cap_unit.token_unit.emplace( "founder_d", 1 );

   // no soft cap -> no soft cap unit
   gpolicy.post_soft_cap_unit.steem_unit.clear();
   gpolicy.post_soft_cap_unit.token_unit.clear();

   gpolicy.min_steem_units_commitment.fillin_nonhidden_value( 1 );
   gpolicy.hard_cap_steem_units_commitment.fillin_nonhidden_value( max_supply );

   gpolicy.soft_cap_percent = STEEMIT_100_PERCENT;

   // .0006 ALPHA / 0.1 STEEM -> 1000 token-units / steem-unit
   gpolicy.min_unit_ratio = 1000;
   gpolicy.max_unit_ratio = 1000;


   smt_setup_operation setup_op;
   setup_op.control_account = "alpha";
   setup_op.decimal_places = 4;

   setup_op.initial_generation_policy = gpolicy;

   setup_op.generation_begin_time = fc::variant( "2017-08-10T00:00:00" ).as< fc::time_point_sec >();
   setup_op.generation_end_time   = fc::variant( "2017-08-17T00:00:00" ).as< fc::time_point_sec >();
   setup_op.announced_launch_time = fc::variant( "2017-08-21T00:00:00" ).as< fc::time_point_sec >();

   setup_op.smt_creation_fee = asset( 1000000, SBD_SYMBOL );

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

   gpolicy.soft_cap_percent = STEEMIT_100_PERCENT;

   // .0006 ALPHA / 0.1 STEEM -> 1000 token-units / steem-unit
   gpolicy.min_unit_ratio = 50;
   gpolicy.max_unit_ratio = 100;

   smt_setup_operation setup_op;
   setup_op.control_account = "beta";
   setup_op.decimal_places = 4;

   setup_op.initial_generation_policy = gpolicy;

   setup_op.generation_begin_time = fc::variant( "2017-06-01T00:00:00" ).as< fc::time_point_sec >();
   setup_op.generation_end_time   = fc::variant( "2017-06-30T00:00:00" ).as< fc::time_point_sec >();
   setup_op.announced_launch_time = fc::variant( "2017-07-01T00:00:00" ).as< fc::time_point_sec >();

   setup_op.smt_creation_fee = asset( 1000000, SBD_SYMBOL );

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

   gpolicy.soft_cap_percent = STEEMIT_100_PERCENT;

   // .001 STEEM / .100000 DELTA -> 100 DELTA / STEEM
   gpolicy.min_unit_ratio = 1000;
   gpolicy.max_unit_ratio = 1000;

   smt_setup_operation setup_op;
   setup_op.control_account = "delta";
   setup_op.decimal_places = 5;

   setup_op.initial_generation_policy = gpolicy;

   setup_op.generation_begin_time = fc::variant( "2017-06-01T00:00:00" ).as< fc::time_point_sec >();
   setup_op.generation_end_time   = fc::variant( "2017-06-30T00:00:00" ).as< fc::time_point_sec >();
   setup_op.announced_launch_time = fc::variant( "2017-07-01T00:00:00" ).as< fc::time_point_sec >();

   setup_op.smt_creation_fee = asset( 1000000, SBD_SYMBOL );

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
      chain::database& db = appbase::app().get_plugin< steemit::plugins::chain::chain_plugin >().db();

      db.pre_apply_operation.connect( [&]( const operation_notification& o ){ my->pre_operation( o ); } );
      db.post_apply_operation.connect( [&]( const operation_notification& o ){ my->post_operation( o ); } );

      // add_plugin_index< key_lookup_index >(db);
   }
   FC_CAPTURE_AND_RETHROW()
}

void smt_test_plugin::plugin_startup() {}

void smt_test_plugin::plugin_shutdown() {}

} } } // steemit::plugins::smt_test

namespace steemit { namespace protocol {

uint32_t smt_generation_unit::steem_unit_sum()const
{
   uint32_t result = 0;
   for(const std::pair< account_name_type, uint16_t >& e : steem_unit )
      result += e.second;
   return result;
}

uint32_t smt_generation_unit::token_unit_sum()const
{
   uint32_t result = 0;
   for(const std::pair< account_name_type, uint16_t >& e : token_unit )
      result += e.second;
   return result;
}

void smt_generation_unit::validate()const
{
   for(const std::pair< account_name_type, uint16_t >& e : steem_unit )
   {
      FC_ASSERT( e.second > 0 );
   }
   for(const std::pair< account_name_type, uint16_t >& e : token_unit )
   {
      FC_ASSERT( e.second > 0 );
   }
}

void smt_cap_commitment::fillin_nonhidden_value_hash( fc::sha256& result, share_type amount )
{
   smt_revealed_cap rc;
   rc.fillin_nonhidden_value( amount );
   result = fc::sha256::hash(rc);
}

void smt_cap_commitment::fillin_nonhidden_value( share_type value )
{
   lower_bound = value;
   upper_bound = value;
   fillin_nonhidden_value_hash( hash, value );
}

void smt_cap_commitment::validate()const
{
   FC_ASSERT( lower_bound > 0 );
   FC_ASSERT( upper_bound <= STEEMIT_MAX_SHARE_SUPPLY );
   FC_ASSERT( lower_bound <= upper_bound );
   if( lower_bound == upper_bound )
   {
      fc::sha256 h;
      fillin_nonhidden_value_hash( h, lower_bound );
      FC_ASSERT( hash == h );
   }
}

#define SMT_MAX_UNIT_COUNT                  20
#define SMT_MAX_DECIMAL_PLACES               8
#define SMT_MIN_HARD_CAP_STEEM_UNITS     10000
#define SMT_MIN_SATURATION_STEEM_UNITS    1000
#define SMT_MIN_SOFT_CAP_STEEM_UNITS      1000

void smt_capped_generation_policy::validate()const
{
   pre_soft_cap_unit.validate();
   post_soft_cap_unit.validate();

   FC_ASSERT( soft_cap_percent > 0 );
   FC_ASSERT( soft_cap_percent <= STEEMIT_100_PERCENT );

   FC_ASSERT( pre_soft_cap_unit.steem_unit.size() > 0 );
   FC_ASSERT( pre_soft_cap_unit.token_unit.size() > 0 );

   FC_ASSERT( pre_soft_cap_unit.steem_unit.size() <= SMT_MAX_UNIT_COUNT );
   FC_ASSERT( pre_soft_cap_unit.token_unit.size() <= SMT_MAX_UNIT_COUNT );
   FC_ASSERT( post_soft_cap_unit.steem_unit.size() <= SMT_MAX_UNIT_COUNT );
   FC_ASSERT( post_soft_cap_unit.token_unit.size() <= SMT_MAX_UNIT_COUNT );

   // TODO : Check account name

   if( soft_cap_percent == STEEMIT_100_PERCENT )
   {
      FC_ASSERT( post_soft_cap_unit.steem_unit.size() == 0 );
      FC_ASSERT( post_soft_cap_unit.token_unit.size() == 0 );
   }
   else
   {
      FC_ASSERT( post_soft_cap_unit.steem_unit.size() > 0 );
   }

   min_steem_units_commitment.validate();
   hard_cap_steem_units_commitment.validate();

   FC_ASSERT( min_steem_units_commitment.lower_bound <= hard_cap_steem_units_commitment.lower_bound );
   FC_ASSERT( min_steem_units_commitment.upper_bound <= hard_cap_steem_units_commitment.upper_bound );

   // Following are non-trivial numerical bounds
   // TODO:  Discuss these restrictions in the whitepaper

   // we want hard cap to be large enough we don't see quantization effects
   FC_ASSERT( hard_cap_steem_units_commitment.lower_bound >= SMT_MIN_HARD_CAP_STEEM_UNITS );

   // we want saturation point to be large enough we don't see quantization effects
   FC_ASSERT( hard_cap_steem_units_commitment.lower_bound >= SMT_MIN_SATURATION_STEEM_UNITS * uint64_t( max_unit_ratio ) );

   // this static_assert checks to be sure min_soft_cap / max_soft_cap computation can't overflow uint64_t
   static_assert( uint64_t( STEEMIT_MAX_SHARE_SUPPLY ) < (std::numeric_limits< uint64_t >::max() / STEEMIT_100_PERCENT), "Overflow check failed" );
   uint64_t min_soft_cap = (uint64_t( hard_cap_steem_units_commitment.lower_bound.value ) * soft_cap_percent) / STEEMIT_100_PERCENT;
   uint64_t max_soft_cap = (uint64_t( hard_cap_steem_units_commitment.upper_bound.value ) * soft_cap_percent) / STEEMIT_100_PERCENT;

   // we want soft cap to be large enough we don't see quantization effects
   FC_ASSERT( min_soft_cap >= SMT_MIN_SOFT_CAP_STEEM_UNITS );

   // We want to prevent the following from overflowing STEEMIT_MAX_SHARE_SUPPLY:
   // max_tokens_created = (u1.tt * sc + u2.tt * (hc-sc)) * min_unit_ratio
   // max_steem_accepted =  u1.st * sc + u2.st * (hc-sc)

   // hc / max_unit_ratio is the saturation point

   uint128_t sc = max_soft_cap;
   uint128_t hc_sc = hard_cap_steem_units_commitment.upper_bound.value - max_soft_cap;

   uint128_t max_tokens_created = (pre_soft_cap_unit.token_unit_sum() * sc + post_soft_cap_unit.token_unit_sum() * hc_sc) * min_unit_ratio;
   uint128_t max_share_supply_u128 = uint128_t( STEEMIT_MAX_SHARE_SUPPLY );

   FC_ASSERT( max_tokens_created <= max_share_supply_u128 );

   uint128_t max_steem_accepted = (pre_soft_cap_unit.steem_unit_sum() * sc + post_soft_cap_unit.steem_unit_sum() * hc_sc);
   FC_ASSERT( max_steem_accepted <= max_share_supply_u128 );
}

struct validate_visitor
{
   typedef void result_type;

   template< typename T >
   void operator()( const T& x )
   {
      x.validate();
   }
};

void smt_setup_operation::validate()const
{
   FC_ASSERT( is_valid_account_name( control_account ) );
   FC_ASSERT( decimal_places <= SMT_MAX_DECIMAL_PLACES );
   FC_ASSERT( max_supply > 0 );
   FC_ASSERT( max_supply <= STEEMIT_MAX_SHARE_SUPPLY );
   validate_visitor vtor;
   initial_generation_policy.visit( vtor );
   FC_ASSERT( generation_begin_time > STEEMIT_GENESIS_TIME );
   FC_ASSERT( generation_end_time > generation_begin_time );
   FC_ASSERT( announced_launch_time >= generation_end_time );

   // TODO:  Support using STEEM as well
   // TODO:  Move amount check to evaluator, symbol check should remain here
   FC_ASSERT( smt_creation_fee == asset( 1000000, SBD_SYMBOL ) );
}

// TODO: These validators
void smt_cap_reveal_operation::validate()const {}
void smt_refund_operation::validate()const {}
void smt_setup_inflation_operation::validate()const {}
void smt_set_setup_parameters_operation::validate()const {}
void smt_set_runtime_parameters_operation::validate()const {}

} }
