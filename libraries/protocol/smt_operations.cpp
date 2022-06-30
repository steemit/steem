
#include <steem/protocol/smt_operations.hpp>
#include <steem/protocol/validation.hpp>
#include <steem/protocol/smt_util.hpp>

namespace steem { namespace protocol {

template < class Operation >
void smt_admin_operation_validate( const Operation& o )
{
   validate_account_name( o.control_account );
   validate_smt_symbol( o.symbol );
}

void smt_create_operation::validate()const
{
   smt_admin_operation_validate( *this );

   if( desired_ticker != smt_ticker_type() )
   {
      FC_ASSERT( desired_ticker != STEEM_SYMBOL_STR, "SMT Ticker Symbol cannot be ${s}", ("s", STEEM_SYMBOL_STR) );
      FC_ASSERT( desired_ticker != SBD_SYMBOL_STR, "SMT Ticker Symbol cannot be ${s}", ("s", SBD_SYMBOL_STR) );
      FC_ASSERT( desired_ticker != VESTS_SYMBOL_STR, "SMT Ticker Symbol cannot be ${s}", ("s", VESTS_SYMBOL_STR) );

      fc::string ticker_str = desired_ticker;

      FC_ASSERT( ticker_str.length() >= 3, "SMT Ticker Symbol must be at least 3 characters long." );
      for( size_t i = 0; i < ticker_str.length(); i++ )
      {
         FC_ASSERT( ticker_str[i] >= 'A' && ticker_str[i] <= 'Z', "SMT Ticker Symbol can only contain capital letters." );
      }
   }

   FC_ASSERT( smt_creation_fee.amount >= 0, "fee cannot be negative" );
   FC_ASSERT( smt_creation_fee.amount <= STEEM_MAX_SHARE_SUPPLY, "Fee must be smaller than STEEM_MAX_SHARE_SUPPLY" );
   FC_ASSERT( is_asset_type( smt_creation_fee, STEEM_SYMBOL ) || is_asset_type( smt_creation_fee, SBD_SYMBOL ), "Fee must be STEEM or SBD" );
   FC_ASSERT( symbol.decimals() == precision, "Mismatch between redundantly provided precision ${prec1} vs ${prec2}",
      ("prec1",symbol.decimals())("prec2",precision) );
}

bool is_valid_unit_target( const unit_target_type& unit_target )
{
   if ( is_valid_account_name( unit_target ) )
      return true;
   if ( smt::unit_target::is_contributor( unit_target ) )
      return true;
   if ( smt::unit_target::is_market_maker( unit_target ) )
      return true;
   if ( smt::unit_target::is_rewards( unit_target ) )
      return true;
   if ( smt::unit_target::is_vesting( unit_target ) )
      return true;
   if ( smt::unit_target::is_founder_vesting( unit_target ) )
      return true;
   return false;
}

bool is_valid_smt_ico_steem_destination( const unit_target_type& unit_target )
{
   if ( is_valid_account_name( unit_target ) )
      return true;
   if ( smt::unit_target::is_market_maker( unit_target ) )
      return true;
   if ( smt::unit_target::is_founder_vesting( unit_target ) )
      return true;
   return false;
}

bool is_valid_smt_ico_token_destination( const unit_target_type& unit_target )
{
   if ( is_valid_account_name( unit_target ) )
      return true;
   if ( smt::unit_target::is_contributor( unit_target ) )
      return true;
   if ( smt::unit_target::is_rewards( unit_target ) )
      return true;
   if ( smt::unit_target::is_market_maker( unit_target ) )
      return true;
   if ( smt::unit_target::is_founder_vesting( unit_target ) )
      return true;
   return false;
}

uint32_t smt_generation_unit::steem_unit_sum()const
{
   uint32_t result = 0;
   for(const std::pair< unit_target_type, uint16_t >& e : steem_unit )
      result += e.second;
   return result;
}

uint32_t smt_generation_unit::token_unit_sum()const
{
   uint32_t result = 0;
   for(const std::pair< unit_target_type, uint16_t >& e : token_unit )
      result += e.second;
   return result;
}

void smt_generation_unit::validate()const
{
   FC_ASSERT( steem_unit.size() <= SMT_MAX_UNIT_ROUTES );
   for(const std::pair< unit_target_type, uint16_t >& e : steem_unit )
   {
      FC_ASSERT( is_valid_unit_target( e.first ) );
      FC_ASSERT( e.second > 0 );
   }
   FC_ASSERT( token_unit.size() <= SMT_MAX_UNIT_ROUTES );
   for(const std::pair< unit_target_type, uint16_t >& e : token_unit )
   {
      FC_ASSERT( is_valid_unit_target( e.first ) );
      FC_ASSERT( e.second > 0 );
   }
}

void smt_emissions_unit::validate() const
{
   FC_ASSERT( token_unit.empty() == false, "Emissions token unit cannot be empty" );
   for ( const auto& e : token_unit )
   {
      FC_ASSERT( smt::unit_target::is_valid_emissions_destination( e.first ),
         "Emissions token unit destination ${n} is invalid", ("n", e.first) );
      FC_ASSERT( e.second > 0, "Emissions token unit must be greater than 0" );
   }
}

uint32_t smt_emissions_unit::token_unit_sum() const
{
   uint32_t result = 0;
   for( const std::pair< unit_target_type, uint16_t >& e : token_unit )
      result += e.second;
   return result;
}

void smt_capped_generation_policy::validate()const
{
   generation_unit.validate();

   FC_ASSERT( generation_unit.steem_unit.size() > 0 );
   FC_ASSERT( generation_unit.token_unit.size() >= 0 );
   FC_ASSERT( generation_unit.steem_unit.size() <= SMT_MAX_UNIT_COUNT );
   FC_ASSERT( generation_unit.token_unit.size() <= SMT_MAX_UNIT_COUNT );

   for ( auto& unit : generation_unit.steem_unit )
      FC_ASSERT( is_valid_smt_ico_steem_destination( unit.first ),
         "${unit_target} is not a valid STEEM unit target.", ("unit_target", unit.first) );

   for ( auto& unit : generation_unit.token_unit )
      FC_ASSERT( is_valid_smt_ico_token_destination( unit.first ),
         "${unit_target} is not a valid token unit target.", ("unit_target", unit.first) );
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

void smt_setup_ico_tier_operation::validate()const
{
   smt_admin_operation_validate( *this );

   FC_ASSERT( steem_satoshi_cap >= 0, "Steem satoshi cap must be greater than or equal to 0" );
   FC_ASSERT( steem_satoshi_cap <= STEEM_MAX_SHARE_SUPPLY, "Steem satoshi cap must be less than or equal to ${n}", ("n", STEEM_MAX_SHARE_SUPPLY) );

   validate_visitor vtor;
   generation_policy.visit( vtor );
}

void smt_setup_emissions_operation::validate()const
{
   smt_admin_operation_validate( *this );

   FC_ASSERT( schedule_time > STEEM_GENESIS_TIME );
   emissions_unit.validate();

   FC_ASSERT( interval_seconds >= SMT_EMISSION_MIN_INTERVAL_SECONDS,
      "Interval seconds must be greater than or equal to ${seconds}", ("seconds", SMT_EMISSION_MIN_INTERVAL_SECONDS) );

   FC_ASSERT( emission_count > 0, "Emission count must be greater than 0" );

   FC_ASSERT( lep_time <= rep_time, "Left endpoint time must be less than or equal to right endpoint time" );

   // If time modulation is enabled
   if ( lep_time != rep_time )
   {
      FC_ASSERT( lep_time >= schedule_time, "Left endpoint time cannot be before the schedule time" );

      // If we don't emit indefinitely
      if ( emission_count != SMT_EMIT_INDEFINITELY )
      {
         FC_ASSERT(
            uint64_t( interval_seconds ) * ( emission_count - 1 ) + uint64_t( schedule_time.sec_since_epoch() ) <= std::numeric_limits< int32_t >::max(),
            "Schedule end time overflow" );
         FC_ASSERT( rep_time <= schedule_time + fc::seconds( uint64_t( interval_seconds ) * ( emission_count - 1 ) ),
            "Right endpoint time cannot be after the schedule end time" );
      }
   }

   FC_ASSERT( symbol.is_vesting() == false, "Use liquid variant of SMT symbol to specify emission amounts" );
   FC_ASSERT( lep_abs_amount >= 0, "Left endpoint cannot have negative emission" );
   FC_ASSERT( rep_abs_amount >= 0, "Right endpoint cannot have negative emission" );

   FC_ASSERT( lep_abs_amount > 0 || lep_rel_amount_numerator > 0 || rep_abs_amount > 0 || rep_rel_amount_numerator > 0,
      "An emission operation must have positive non-zero emission" );

    for ( const auto& e : emissions_unit.token_unit )
    {
        if ( smt::unit_target::is_account_name_type( e.first ) )
        {
            std::string name = smt::unit_target::get_unit_target_account( e.first );
            FC_ASSERT( name != STEEM_TREASURY_ACCOUNT, "Invalid emission destination, cannot emit to the treasury account ${a}", ("a", STEEM_TREASURY_ACCOUNT) );
            FC_ASSERT( name != STEEM_NULL_ACCOUNT, "Invalid emission destination, cannot emit to ${a}", ("a", STEEM_NULL_ACCOUNT) );
        }
    }

   // rel_amount_denom_bits <- any value of unsigned int is OK
}

void smt_setup_operation::validate()const
{
   smt_admin_operation_validate( *this );

   FC_ASSERT( max_supply > 0, "Max supply must be greater than 0" );
   FC_ASSERT( max_supply <= STEEM_MAX_SHARE_SUPPLY, "Max supply must be less than ${n}", ("n", STEEM_MAX_SHARE_SUPPLY) );
   FC_ASSERT( contribution_begin_time > STEEM_GENESIS_TIME, "Contribution begin time must be greater than ${t}", ("t", STEEM_GENESIS_TIME) );
   FC_ASSERT( contribution_end_time >= contribution_begin_time, "Contribution end time must be equal to or later than contribution begin time" );
   FC_ASSERT( launch_time >= contribution_end_time, "Launch time must be equal to or later than the contribution end time" );
   FC_ASSERT( steem_satoshi_min >= 0, "Steem satoshi minimum must be greater than or equal to 0" );
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
         case quadratic:
         case linear:
         case square_root:
         case convergent_linear:
         case convergent_square_root:
         case bounded:
            break;
         default:
            FC_ASSERT( false, "Author Reward Curve must be quadratic, linear, square_root, convergent_linear, convergent_square_root or bounded." );
      }

      switch( param_rewards.curation_reward_curve )
      {
         case quadratic:
         case linear:
         case square_root:
         case convergent_linear:
         case convergent_square_root:
         case bounded:
            break;
         default:
            FC_ASSERT( false, "Curation Reward Curve must be quadratic, linear, square_root, convergent_linear, convergent_square_root or bounded." );
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
   validate_smt_symbol( symbol );
   FC_ASSERT( contribution.symbol == STEEM_SYMBOL, "Contributions must be made in STEEM" );
   FC_ASSERT( contribution.amount > 0, "Contribution amount must be greater than 0" );
}

} }
