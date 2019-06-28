
#include <steem/protocol/smt_operations.hpp>
#include <steem/protocol/validation.hpp>
#ifdef STEEM_ENABLE_SMT

#define SMT_DESTINATION_FROM          account_name_type( "$from" )
#define SMT_DESTINATION_FROM_VESTING  account_name_type( "$from.vesting" )
#define SMT_DESTINATION_MARKET_MAKER  account_name_type( "$market_maker" )
#define SMT_DESTINATION_REWARDS       account_name_type( "$rewards" )
#define SMT_DESTINATION_VESTING       account_name_type( "$vesting" )

namespace steem { namespace protocol {

void common_symbol_validation( const asset_symbol_type& symbol )
{
   symbol.validate();
   FC_ASSERT( symbol.space() == asset_symbol_type::smt_nai_space, "legacy symbol used instead of NAI" );
   FC_ASSERT( symbol.is_vesting() == false, "liquid variant of NAI expected");
}

template < class Operation >
void smt_admin_operation_validate( const Operation& o )
{
   validate_account_name( o.control_account );
   common_symbol_validation( o.symbol );
}

void smt_create_operation::validate()const
{
   smt_admin_operation_validate( *this );
   FC_ASSERT( smt_creation_fee.amount >= 0, "fee cannot be negative" );
   FC_ASSERT( smt_creation_fee.amount <= STEEM_MAX_SHARE_SUPPLY, "Fee must be smaller than STEEM_MAX_SHARE_SUPPLY" );
   FC_ASSERT( is_asset_type( smt_creation_fee, STEEM_SYMBOL ) || is_asset_type( smt_creation_fee, SBD_SYMBOL ), "Fee must be STEEM or SBD" );
   FC_ASSERT( symbol.decimals() == precision, "Mismatch between redundantly provided precision ${prec1} vs ${prec2}",
      ("prec1",symbol.decimals())("prec2",precision) );
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

bool is_valid_smt_emissions_unit_destination( const account_name_type& name )
{
   if ( is_valid_account_name( name ) )
      return true;
   if ( name == SMT_DESTINATION_REWARDS )
      return true;
   if ( name == SMT_DESTINATION_VESTING )
      return true;
   if ( name == SMT_DESTINATION_MARKET_MAKER )
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

void smt_setup_emissions_operation::validate()const
{
   smt_admin_operation_validate( *this );

   FC_ASSERT( schedule_time > STEEM_GENESIS_TIME );
   FC_ASSERT( emissions_unit.token_unit.empty() == false, "Emissions token unit cannot be empty" );

   for ( const auto& e : emissions_unit.token_unit )
   {
      FC_ASSERT( is_valid_smt_emissions_unit_destination( e.first ),
         "Emissions token unit destination ${n} is invalid", ("n", e.first) );
      FC_ASSERT( e.second > 0, "Emissions token unit must be greater than 0" );
   }

   FC_ASSERT( interval_seconds >= SMT_EMISSION_MIN_INTERVAL_SECONDS,
      "Interval seconds must be greater than or equal to ${seconds}", ("seconds", SMT_EMISSION_MIN_INTERVAL_SECONDS) );

   FC_ASSERT( interval_count > 0, "Interval count must be greater than 0" );

   FC_ASSERT( lep_time <= rep_time, "Left endpoint time must be less than or equal to right endpoint time" );

   // If time modulation is enabled
   if ( lep_time != rep_time )
   {
      FC_ASSERT( lep_time >= schedule_time, "Left endpoint time cannot be before the schedule time" );

      // If we don't emit indefinitely
      if ( interval_count != SMT_EMIT_INDEFINITELY )
      {
         FC_ASSERT(
            uint64_t( interval_seconds ) * uint64_t( interval_count ) + uint64_t( schedule_time.sec_since_epoch() ) <= std::numeric_limits< int32_t >::max(),
            "Schedule end time overflow" );
         FC_ASSERT( rep_time <= schedule_time + fc::seconds( uint64_t( interval_seconds ) * uint64_t( interval_count ) ),
            "Right endpoint time cannot be after the schedule end time" );
      }
   }

   FC_ASSERT( symbol.is_vesting() == false, "Use liquid variant of SMT symbol to specify emission amounts" );
   FC_ASSERT( symbol == lep_abs_amount.symbol, "Left endpoint symbol mismatch" );
   FC_ASSERT( symbol == rep_abs_amount.symbol, "Right endpoint symbol mismatch" );
   FC_ASSERT( lep_abs_amount.amount >= 0, "Left endpoint cannot have negative emission" );
   FC_ASSERT( rep_abs_amount.amount >= 0, "Right endpoint cannot have negative emission" );

   FC_ASSERT( lep_abs_amount.amount > 0 || lep_rel_amount_numerator > 0 || rep_abs_amount.amount > 0 || rep_rel_amount_numerator > 0,
      "An emission operation must have positive non-zero emission" );

   // rel_amount_denom_bits <- any value of unsigned int is OK
}

void smt_setup_operation::validate()const
{
   smt_admin_operation_validate( *this );

   FC_ASSERT( max_supply > 0, "Max supply must be greater than 0" );
   FC_ASSERT( max_supply <= STEEM_MAX_SHARE_SUPPLY, "Max supply must be less than ${n}", ("n", STEEM_MAX_SHARE_SUPPLY) );
   FC_ASSERT( contribution_begin_time > STEEM_GENESIS_TIME, "Contribution begin time must be greater than ${t}", ("t", STEEM_GENESIS_TIME) );
   FC_ASSERT( contribution_end_time > contribution_begin_time, "Contribution end time must be after contribution begin time" );
   FC_ASSERT( launch_time >= contribution_end_time, "SMT ICO launch time must be after the contribution end time" );
   FC_ASSERT( steem_units_soft_cap <= steem_units_hard_cap, "Steem units soft cap must less than or equal to steem units hard cap" );
   FC_ASSERT( steem_units_soft_cap >= SMT_MIN_SOFT_CAP_STEEM_UNITS, "Steem units soft cap must be greater than or equal to ${n}", ("n", SMT_MIN_SOFT_CAP_STEEM_UNITS) );
   FC_ASSERT( steem_units_hard_cap >= SMT_MIN_HARD_CAP_STEEM_UNITS, "Steem units hard cap must be greater than or equal to ${n}", ("n", SMT_MIN_HARD_CAP_STEEM_UNITS) );
   FC_ASSERT( steem_units_hard_cap <= STEEM_MAX_SHARE_SUPPLY, "Steem units hard cap must be less than or equal to ${n}", ("n", STEEM_MAX_SHARE_SUPPLY) );

   validate_visitor vtor;
   initial_generation_policy.visit( vtor );
}

struct smt_set_runtime_parameters_operation_visitor
{
   smt_set_runtime_parameters_operation_visitor(){}

   typedef void result_type;

   void operator()( const smt_param_windows_v1& param_windows )const
   {
      // 0 <= reverse_auction_window_seconds + SMT_UPVOTE_LOCKOUT < cashout_window_seconds < SMT_VESTING_WITHDRAW_INTERVAL_SECONDS
      uint64_t sum = ( param_windows.reverse_auction_window_seconds + SMT_UPVOTE_LOCKOUT );

      FC_ASSERT( sum < param_windows.cashout_window_seconds,
         "'reverse auction window + upvote lockout' interval must be less than cashout window (${c}). Was ${sum} seconds.",
         ("c", param_windows.cashout_window_seconds)
         ( "sum", sum ) );

      FC_ASSERT( param_windows.cashout_window_seconds <= SMT_VESTING_WITHDRAW_INTERVAL_SECONDS,
         "Cashout window second must be less than 'vesting withdraw' interval (${v}). Was ${val} seconds.",
         ("v", SMT_VESTING_WITHDRAW_INTERVAL_SECONDS)
         ("val", param_windows.cashout_window_seconds) );
   }

   void operator()( const smt_param_vote_regeneration_period_seconds_v1& vote_regeneration )const
   {
      // 0 < vote_regeneration_seconds < SMT_VESTING_WITHDRAW_INTERVAL_SECONDS
      FC_ASSERT( vote_regeneration.vote_regeneration_period_seconds > 0 &&
         vote_regeneration.vote_regeneration_period_seconds <= SMT_VESTING_WITHDRAW_INTERVAL_SECONDS,
         "Vote regeneration period must be greater than 0 and less than 'vesting withdraw' (${v}) interval. Was ${val} seconds.",
         ("v", SMT_VESTING_WITHDRAW_INTERVAL_SECONDS )
         ("val", vote_regeneration.vote_regeneration_period_seconds) );

      FC_ASSERT( vote_regeneration.votes_per_regeneration_period > 0 &&
         vote_regeneration.votes_per_regeneration_period <= SMT_MAX_VOTES_PER_REGENERATION,
         "Votes per regeneration period exceeds maximum (${max}). Was ${v}",
         ("max", SMT_MAX_VOTES_PER_REGENERATION)
         ("v", vote_regeneration.vote_regeneration_period_seconds) );

      // With previous assertion, this value will not overflow a 32 bit integer
      // This calculation is designed to round up
      uint32_t nominal_votes_per_day = ( vote_regeneration.votes_per_regeneration_period * 86400 + vote_regeneration.vote_regeneration_period_seconds - 1 )
         / vote_regeneration.vote_regeneration_period_seconds;

      FC_ASSERT( nominal_votes_per_day <= SMT_MAX_NOMINAL_VOTES_PER_DAY,
         "Nominal votes per day exceeds maximum (${max}). Was ${v}",
         ("max", SMT_MAX_NOMINAL_VOTES_PER_DAY)
         ("v", nominal_votes_per_day) );
   }

   void operator()( const smt_param_rewards_v1& param_rewards )const
   {
      FC_ASSERT( param_rewards.percent_curation_rewards <= STEEM_100_PERCENT,
         "Percent Curation Rewards must not exceed 10000. Was ${n}",
         ("n", param_rewards.percent_curation_rewards) );

      switch( param_rewards.author_reward_curve )
      {
         case linear:
         case quadratic:
            break;
         default:
            FC_ASSERT( false, "Author Reward Curve must be linear or quadratic" );
      }

      switch( param_rewards.curation_reward_curve )
      {
         case linear:
         case square_root:
         case bounded_curation:
            break;
         default:
            FC_ASSERT( false, "Curation Reward Curve must be linear, square_root, or bounded_curation." );
      }
   }

   void operator()( const smt_param_allow_downvotes& )const
   {
      //Nothing to do
   }
};

void smt_set_runtime_parameters_operation::validate()const
{
   smt_admin_operation_validate( *this );
   FC_ASSERT( !runtime_parameters.empty() );

   smt_set_runtime_parameters_operation_visitor visitor;
   for( auto& param: runtime_parameters )
      param.visit( visitor );
}

void smt_set_setup_parameters_operation::validate() const
{
   smt_admin_operation_validate( *this );
   FC_ASSERT( setup_parameters.empty() == false );
}

void smt_contribute_operation::validate() const
{
   validate_account_name( contributor );
   common_symbol_validation( symbol );
   FC_ASSERT( contribution.symbol == STEEM_SYMBOL, "Contributions must be made in STEEM" );
   FC_ASSERT( contribution.amount > 0, "Contribution amount must be greater than 0" );
}

} }
#endif
