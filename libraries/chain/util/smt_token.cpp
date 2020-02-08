#include <steem/chain/steem_fwd.hpp>
#include <steem/chain/util/smt_token.hpp>
#include <steem/chain/steem_object_types.hpp>
#include <steem/protocol/smt_util.hpp>

namespace steem { namespace chain { namespace util { namespace smt {

const smt_token_object* find_token( const database& db, uint32_t nai )
{
   const auto& idx = db.get_index< smt_token_index >().indices().get< by_symbol >();

   auto itr = idx.lower_bound( asset_symbol_type::from_nai( nai, 0 ) );
   for (; itr != idx.end(); ++itr )
   {
      if (itr->liquid_symbol.to_nai() != nai )
         break;
      return &*itr;
   }

   return nullptr;
}

const smt_token_object* find_token( const database& db, asset_symbol_type symbol, bool precision_agnostic )
{
   // Only liquid symbols are stored in the smt token index
   auto s = symbol.is_vesting() ? symbol.get_paired_symbol() : symbol;

   if ( precision_agnostic )
      return find_token( db, s.to_nai() );
   else
      return db.find< smt_token_object, by_symbol >( s );
}

const smt_token_emissions_object* last_emission( const database& db, const asset_symbol_type& symbol )
{
   const auto& idx = db.get_index< smt_token_emissions_index, by_symbol_time >();

   const auto range = idx.equal_range( symbol );

   /*
    * The last element in this range is our highest value 'schedule_time', so
    * we reverse the iterator range and take the first one.
    */
   auto itr = range.second;
   while ( itr != range.first )
   {
      --itr;
      return &*itr;
   }

   return nullptr;
}

fc::optional< time_point_sec > last_emission_time( const database& db, const asset_symbol_type& symbol )
{
   auto _last_emission = last_emission( db, symbol );

   if ( _last_emission != nullptr )
      return _last_emission->schedule_end_time();

   return {};
}

fc::optional< time_point_sec > next_emission_time( const database& db, const asset_symbol_type& symbol, time_point_sec last_emission )
{
   const auto& idx = db.get_index< smt_token_emissions_index, by_symbol_end_time >();
   auto emission = idx.lower_bound( boost::make_tuple( symbol, last_emission ) );

   if( emission != idx.end() && emission->symbol == symbol )
   {
      if( last_emission < emission->schedule_time ) return emission->schedule_time;

      last_emission -= ( last_emission - emission->schedule_time ).to_seconds() % emission->interval_seconds;
      fc::time_point_sec next_schedule = last_emission + fc::seconds( emission->interval_seconds );
      if( next_schedule <= emission->schedule_end_time() ) return next_schedule;

      ++emission;
      if( emission != idx.end() && emission->symbol == symbol )
      {
         next_schedule = emission->schedule_time;
         return emission->schedule_time;
      }
   }

   return {};
}

const smt_token_emissions_object* get_emission_object( const database& db, const asset_symbol_type& symbol, time_point_sec t )
{
   const auto& idx = db.get_index< smt_token_emissions_index, by_symbol_end_time >();
   auto emission = idx.lower_bound( boost::make_tuple( symbol, t ) );

   if ( emission != idx.end() && emission->symbol == symbol )
      if( t >= emission->schedule_time && t <= emission->schedule_end_time() )
         return &*emission;

   return nullptr;
}

flat_map< unit_target_type, share_type > generate_emissions( const smt_token_object& token, const smt_token_emissions_object& emission, time_point_sec emission_time )
{
   flat_map< unit_target_type, share_type > emissions;

   try
   {
      share_type abs_amount;
      uint64_t rel_amount_numerator;

      if ( emission_time <= emission.lep_time )
      {
         abs_amount = emission.lep_abs_amount;
         rel_amount_numerator = emission.lep_rel_amount_numerator;
      }
      else if ( emission_time >= emission.rep_time )
      {
         abs_amount = emission.rep_abs_amount;
         rel_amount_numerator = emission.rep_rel_amount_numerator;
      }
      else
      {
         fc::uint128 lep_abs_val{ emission.lep_abs_amount.value },
                     rep_abs_val{ emission.rep_abs_amount.value },
                     lep_rel_num{ emission.lep_rel_amount_numerator },
                     rep_rel_num{ emission.rep_rel_amount_numerator };

         uint32_t lep_dist    = emission_time.sec_since_epoch() - emission.lep_time.sec_since_epoch();
         uint32_t rep_dist    = emission.rep_time.sec_since_epoch() - emission_time.sec_since_epoch();
         uint32_t total_dist  = emission.rep_time.sec_since_epoch() - emission.lep_time.sec_since_epoch();
         abs_amount           = ( ( lep_abs_val * lep_dist + rep_abs_val * rep_dist ) / total_dist ).to_int64();
         rel_amount_numerator = ( ( lep_rel_num * lep_dist + rep_rel_num * rep_dist ) / total_dist ).to_uint64();
      }

      share_type rel_amount = ( fc::uint128( token.current_supply.value ) * rel_amount_numerator >> emission.rel_amount_denom_bits ).to_int64();

      share_type new_token_supply;
      if ( emission.floor_emissions )
         new_token_supply = std::min( abs_amount, rel_amount );
      else
         new_token_supply = std::max( abs_amount, rel_amount );

      if ( token.current_supply + new_token_supply > token.max_supply )
         new_token_supply = token.max_supply - token.current_supply;

      uint32_t unit_sum = emission.emissions_unit.token_unit_sum();

      if ( new_token_supply >= unit_sum )
      {
         for ( auto& e : emission.emissions_unit.token_unit )
            emissions[ e.first ] = ( ( fc::uint128( e.second ) * new_token_supply.value ) / unit_sum ).to_int64();
      }

   }
   FC_CAPTURE_AND_RETHROW( (token)(emission)(emission_time) );

   return emissions;
}

namespace ico {

share_type payout( database& db, const asset_symbol_type& symbol, const account_object& account, const std::vector< contribution_payout >& payouts )
{
   share_type additional_token_supply = 0;

   for ( auto& p : payouts )
   {
      if ( p.to_vesting )
         db.create_vesting( account, p.payout );
      else
         db.adjust_balance( account, p.payout );

      if ( p.payout.symbol.space() == asset_symbol_type::smt_nai_space )
         additional_token_supply += p.payout.amount;
   }

   return additional_token_supply;
}

bool schedule_next_refund( database& db, const asset_symbol_type& a )
{
   bool action_scheduled = false;
   auto& idx = db.get_index< smt_contribution_index, by_symbol_id >();
   auto itr = idx.lower_bound( a );

   if ( itr != idx.end() && itr->symbol == a )
   {
      smt_refund_action refund_action;
      refund_action.symbol = itr->symbol;
      refund_action.contributor = itr->contributor;
      refund_action.contribution_id = itr->contribution_id;
      refund_action.refund = itr->contribution;

      db.push_required_action( refund_action );
      action_scheduled = true;
   }

   return action_scheduled;
}

struct payout_vars
{
   share_type steem_units_sent;
   share_type unit_ratio;
};

static payout_vars calculate_payout_vars( database& db, const smt_ico_object& ico, const smt_generation_unit& generation_unit, share_type contribution_amount )
{
   payout_vars vars;

   auto hard_cap = util::smt::ico::get_ico_steem_hard_cap( db, ico.symbol );
   FC_ASSERT( hard_cap.valid(), "Unable to find ICO hard cap." );
   share_type steem_hard_cap = *hard_cap;

   vars.steem_units_sent = contribution_amount / generation_unit.steem_unit_sum();
   auto total_contributed_steem_units = ico.contributed.amount / generation_unit.steem_unit_sum();
   auto steem_units_hard_cap = steem_hard_cap / generation_unit.steem_unit_sum();

   auto total_generated_token_units = std::min(
      total_contributed_steem_units * ico.max_unit_ratio,
      steem_units_hard_cap * ico.min_unit_ratio
   );

   vars.unit_ratio = total_generated_token_units / total_contributed_steem_units;

   return vars;
}

bool schedule_next_contributor_payout( database& db, const asset_symbol_type& a )
{
   bool action_scheduled = false;
   auto& idx = db.get_index< smt_contribution_index, by_symbol_id >();
   auto itr = idx.lower_bound( a );

   if ( itr != idx.end() && itr->symbol == a )
   {
      smt_contributor_payout_action payout_action;
      payout_action.contributor = itr->contributor;
      payout_action.contribution_id = itr->contribution_id;
      payout_action.contribution = itr->contribution;
      payout_action.symbol = itr->symbol;

      const auto& ico = db.get< smt_ico_object, by_symbol >( itr->symbol );
      const auto& ico_tier_idx = db.get_index< smt_ico_tier_index, by_symbol_steem_units_cap >();

      using generation_unit_share = std::tuple< smt_generation_unit, share_type >;

      std::vector< generation_unit_share > generation_unit_shares;

      auto begin_key = boost::make_tuple( itr->symbol, ico.processed_contributions );
      share_type unprocessed_contributions = itr->contribution.amount;
      for ( auto ico_tier = ico_tier_idx.lower_bound( begin_key ); unprocessed_contributions > 0 && ico_tier != ico_tier_idx.end(); ++ico_tier )
      {
         auto tier_contributions = unprocessed_contributions;

         if ( ico.processed_contributions + unprocessed_contributions > ico_tier->steem_units_cap )
            tier_contributions = ico_tier->steem_units_cap - ico.processed_contributions;

         generation_unit_shares.push_back( std::make_tuple( ico_tier->generation_unit, tier_contributions ) );

         unprocessed_contributions -= tier_contributions;
      }

      std::map< asset_symbol_type, share_type > payout_map;

      for ( auto& effective_generation_unit_shares : generation_unit_shares )
      {
         const auto& effective_generation_unit = std::get< 0 >( effective_generation_unit_shares );
         const auto& contributed_amount        = std::get< 1 >( effective_generation_unit_shares );

         const auto& token_unit = effective_generation_unit.token_unit;
         const auto& steem_unit = effective_generation_unit.steem_unit;

         auto vars = calculate_payout_vars( db, ico, effective_generation_unit, contributed_amount );

         for ( auto& e : token_unit )
         {
            if ( !protocol::smt::unit_target::is_contributor( e.first ) )
               continue;

            auto token_shares = e.second * vars.steem_units_sent * vars.unit_ratio;

            asset_symbol_type symbol = itr->symbol;

            if ( protocol::smt::unit_target::is_vesting_type( e.first ) )
               symbol = symbol.get_paired_symbol();

            if ( payout_map.count( symbol ) )
               payout_map[ symbol ] += token_shares;
            else
               payout_map[ symbol ] = token_shares;
         }

         for ( auto& e : steem_unit )
         {
            if ( !protocol::smt::unit_target::is_contributor( e.first ) )
               continue;

            auto steem_shares = e.second * vars.steem_units_sent;

            asset_symbol_type symbol = STEEM_SYMBOL;

            if ( protocol::smt::unit_target::is_vesting_type( e.first ) )
               symbol = symbol.get_paired_symbol();

            if ( payout_map.count( symbol ) )
               payout_map[ symbol ] += steem_shares;
            else
               payout_map[ symbol ] = steem_shares;
         }
      }

      for ( auto it = payout_map.begin(); it != payout_map.end(); ++it )
         payout_action.payouts.push_back( { asset( it->second, it->first.get_liquid_symbol() ), it->first.is_vesting() } );

      db.push_required_action( payout_action );
      action_scheduled = true;
   }

   return action_scheduled;
}

bool schedule_founder_payout( database& db, const asset_symbol_type& a )
{
   bool action_scheduled = false;
   const auto& ico = db.get< smt_ico_object, by_symbol >( a );
   const auto& ico_tier_idx = db.get_index< smt_ico_tier_index, by_symbol_steem_units_cap >();

   using generation_unit_share = std::tuple< smt_generation_unit, share_type >;

   std::vector< generation_unit_share > generation_unit_shares;

   share_type unprocessed_contributions = ico.contributed.amount;
   auto last_ico_tier = ico_tier_idx.end();
   for ( auto ico_tier = ico_tier_idx.begin(); unprocessed_contributions > 0 && ico_tier != ico_tier_idx.end(); ++ico_tier )
   {
      auto tier_contributions = unprocessed_contributions;

      if ( ico.contributed.amount > ico_tier->steem_units_cap )
      {
         if ( last_ico_tier != ico_tier_idx.end() )
            tier_contributions = last_ico_tier->steem_units_cap - ico_tier->steem_units_cap;
         else
            tier_contributions = ico_tier->steem_units_cap;
      }

      generation_unit_shares.push_back( std::make_tuple( ico_tier->generation_unit, tier_contributions ) );

      unprocessed_contributions -= tier_contributions;
      last_ico_tier = ico_tier;
   }


   using account_asset_symbol_type = std::tuple< account_name_type, asset_symbol_type >;
   std::map< account_asset_symbol_type, share_type > account_payout_map;
   share_type                                        market_maker_tokens = 0;
   share_type                                        market_maker_steem  = 0;
   share_type                                        rewards             = 0;

   for ( auto& effective_generation_unit_shares : generation_unit_shares )
   {
      const auto& effective_generation_unit = std::get< 0 >( effective_generation_unit_shares );
      const auto& contributed_amount        = std::get< 1 >( effective_generation_unit_shares );

      const auto& token_unit = effective_generation_unit.token_unit;
      const auto& steem_unit = effective_generation_unit.steem_unit;

      auto vars = calculate_payout_vars( db, ico, effective_generation_unit, contributed_amount );

      for ( auto& e : token_unit )
      {
         if ( protocol::smt::unit_target::is_contributor( e.first ) )
            continue;

         auto token_shares = e.second * vars.steem_units_sent * vars.unit_ratio;

         if ( protocol::smt::unit_target::is_market_maker( e.first ) )
         {
            market_maker_tokens += token_shares;
         }
         else if ( protocol::smt::unit_target::is_rewards( e.first ) )
         {
            rewards += token_shares;
         }
         else
         {
            asset_symbol_type symbol = a;

            if ( protocol::smt::unit_target::is_vesting_type( e.first ) )
               symbol = symbol.get_paired_symbol();

            account_name_type account_name = protocol::smt::unit_target::get_unit_target_account( e.first );
            auto map_key = std::make_tuple( account_name, symbol );
            if ( account_payout_map.count( map_key ) )
               account_payout_map[ map_key ] += token_shares;
            else
               account_payout_map[ map_key ] = token_shares;
         }
      }

      for ( auto& e : steem_unit )
      {
         if ( protocol::smt::unit_target::is_contributor( e.first ) )
            continue;

         auto steem_shares = e.second * vars.steem_units_sent;

         if ( protocol::smt::unit_target::is_market_maker( e.first ) )
         {
            market_maker_steem += steem_shares;
         }
         else
         {
            asset_symbol_type symbol = STEEM_SYMBOL;

            if ( protocol::smt::unit_target::is_vesting_type( e.first ) )
               symbol = symbol.get_paired_symbol();

            account_name_type account_name = protocol::smt::unit_target::get_unit_target_account( e.first );
            auto map_key = std::make_tuple( account_name, symbol );
            if ( account_payout_map.count( map_key ) )
               account_payout_map[ map_key ] += steem_shares;
            else
               account_payout_map[ map_key ] = steem_shares;
         }
      }
   }

   if ( account_payout_map.size() > 0 || market_maker_steem > 0 || market_maker_tokens > 0 || rewards > 0 )
   {
      smt_founder_payout_action payout_action;
      payout_action.symbol = a;

      for ( auto it = account_payout_map.begin(); it != account_payout_map.end(); ++it )
         payout_action.account_payouts[ std::get< 0 >( it->first ) ].push_back( { asset( it->second, std::get< 1 >( it->first ).get_liquid_symbol() ), std::get< 1 >( it->first ).is_vesting() } );

      payout_action.market_maker_tokens = market_maker_tokens;
      payout_action.market_maker_steem  = market_maker_steem;
      payout_action.reward_balance = rewards;

      db.push_required_action( payout_action );
      action_scheduled = true;
   }

   return action_scheduled;
}

fc::optional< share_type > get_ico_steem_hard_cap( database& db, const asset_symbol_type& a )
{
   const auto& idx = db.get_index< smt_ico_tier_index, by_symbol_steem_units_cap >();

   const auto range = idx.equal_range( a );

   auto itr = range.second;
   while ( itr != range.first )
   {
      --itr;
      return itr->steem_units_cap;
   }

   return {};
}

void remove_ico_objects( database& db, const asset_symbol_type& symbol )
{
   db.remove( db.get< smt_ico_object, by_symbol >( symbol ) );

   const auto& ico_tier_idx = db.get_index< smt_ico_tier_index, by_symbol_steem_units_cap >();
   auto itr = ico_tier_idx.lower_bound( symbol );
   while( itr != ico_tier_idx.end() && itr->symbol == symbol )
   {
      const auto& current = *itr;
      ++itr;
      db.remove( current );
   }
}

// If we increase the value of SMT_MAX_ICO_TIERS, we should track the size in the ICO object
std::size_t ico_tier_size( database& db, const asset_symbol_type& symbol )
{
   std::size_t num_ico_tiers = 0;

   const auto& ico_tier_idx = db.get_index< smt_ico_tier_index, by_symbol_steem_units_cap >();
   auto ico_tier_itr = ico_tier_idx.lower_bound( symbol );
   while ( ico_tier_itr != ico_tier_idx.end() && ico_tier_itr->symbol == symbol )
   {
      num_ico_tiers++;
      ++ico_tier_itr;
   }

   return num_ico_tiers;
}

} // steem::chain::util::smt::ico

} } } } // steem::chain::util::smt

