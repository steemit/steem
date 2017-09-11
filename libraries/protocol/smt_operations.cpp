
#include <steemit/protocol/smt_operations.hpp>
#include <steemit/protocol/validation.hpp>
#ifdef STEEM_ENABLE_SMT
#define SMT_MAX_UNIT_ROUTES       10

namespace steemit { namespace protocol {

void smt_elevate_account_operation::validate()const
{
   validate_account_name( account );
   FC_ASSERT( fee.amount >= 0, "fee cannot be negative" );
   FC_ASSERT( fee.amount <= STEEM_MAX_SHARE_SUPPLY, "Fee must be smaller than STEEM_MAX_SHARE_SUPPLY" );
   FC_ASSERT( is_asset_type( fee, STEEM_SYMBOL ) || is_asset_type( fee, SBD_SYMBOL ), "Fee must be STEEM or SBD" );
}

bool is_valid_unit_target( const account_name_type& name )
{
   if( is_valid_account_name(name) )
      return true;
   if( name == account_name_type("$from") )
      return true;
   if( name == account_name_type("$from.vesting") )
      return true;
   return false;
}

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
   FC_ASSERT( steem_unit.size() <= SMT_MAX_UNIT_ROUTES );
   for(const std::pair< account_name_type, uint16_t >& e : steem_unit )
   {
      FC_ASSERT( is_valid_unit_target( e.first ) );
      FC_ASSERT( e.second > 0 );
   }
   FC_ASSERT( token_unit.size() <= SMT_MAX_UNIT_ROUTES );
   for(const std::pair< account_name_type, uint16_t >& e : token_unit )
   {
      FC_ASSERT( is_valid_unit_target( e.first ) );
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
   FC_ASSERT( upper_bound <= STEEM_MAX_SHARE_SUPPLY );
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
   FC_ASSERT( soft_cap_percent <= STEEM_100_PERCENT );

   FC_ASSERT( pre_soft_cap_unit.steem_unit.size() > 0 );
   FC_ASSERT( pre_soft_cap_unit.token_unit.size() > 0 );

   FC_ASSERT( pre_soft_cap_unit.steem_unit.size() <= SMT_MAX_UNIT_COUNT );
   FC_ASSERT( pre_soft_cap_unit.token_unit.size() <= SMT_MAX_UNIT_COUNT );
   FC_ASSERT( post_soft_cap_unit.steem_unit.size() <= SMT_MAX_UNIT_COUNT );
   FC_ASSERT( post_soft_cap_unit.token_unit.size() <= SMT_MAX_UNIT_COUNT );

   // TODO : Check account name

   if( soft_cap_percent == STEEM_100_PERCENT )
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
   static_assert( uint64_t( STEEM_MAX_SHARE_SUPPLY ) < (std::numeric_limits< uint64_t >::max() / STEEM_100_PERCENT), "Overflow check failed" );
   uint64_t min_soft_cap = (uint64_t( hard_cap_steem_units_commitment.lower_bound.value ) * soft_cap_percent) / STEEM_100_PERCENT;
   uint64_t max_soft_cap = (uint64_t( hard_cap_steem_units_commitment.upper_bound.value ) * soft_cap_percent) / STEEM_100_PERCENT;

   // we want soft cap to be large enough we don't see quantization effects
   FC_ASSERT( min_soft_cap >= SMT_MIN_SOFT_CAP_STEEM_UNITS );

   // We want to prevent the following from overflowing STEEM_MAX_SHARE_SUPPLY:
   // max_tokens_created = (u1.tt * sc + u2.tt * (hc-sc)) * min_unit_ratio
   // max_steem_accepted =  u1.st * sc + u2.st * (hc-sc)

   // hc / max_unit_ratio is the saturation point

   uint128_t sc = max_soft_cap;
   uint128_t hc_sc = hard_cap_steem_units_commitment.upper_bound.value - max_soft_cap;

   uint128_t max_tokens_created = (pre_soft_cap_unit.token_unit_sum() * sc + post_soft_cap_unit.token_unit_sum() * hc_sc) * min_unit_ratio;
   uint128_t max_share_supply_u128 = uint128_t( STEEM_MAX_SHARE_SUPPLY );

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
   FC_ASSERT( max_supply <= STEEM_MAX_SHARE_SUPPLY );
   validate_visitor vtor;
   initial_generation_policy.visit( vtor );
   FC_ASSERT( generation_begin_time > STEEM_GENESIS_TIME );
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
#endif