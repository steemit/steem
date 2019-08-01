#include <steem/chain/steem_fwd.hpp>
#include <steem/chain/util/smt_token.hpp>
#include <steem/chain/steem_object_types.hpp>

#ifdef STEEM_ENABLE_SMT

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
   {
      // A maximum interval count indicates we should emit indefinitely
      if ( _last_emission->interval_count == SMT_EMIT_INDEFINITELY )
         return time_point_sec::maximum();
      else
         // This potential time_point overflow is protected by smt_setup_emissions_operation::validate
         return _last_emission->schedule_time + fc::seconds( uint64_t( _last_emission->interval_seconds ) * uint64_t( _last_emission->interval_count ) );
   }

   return {};
}

namespace generation_unit {

bool is_founder( const account_name_type& unit_target )
{
   return unit_target != SMT_DESTINATION_FROM && unit_target != SMT_DESTINATION_FROM_VESTING;
}

bool is_contributor( const account_name_type& unit_target )
{
   return !is_founder( unit_target );
}

bool is_vesting( const account_name_type& name )
{
   if ( name == SMT_DESTINATION_FROM_VESTING )
      return true;
   if ( name == SMT_DESTINATION_VESTING )
      return true;

   return false;
}

} // steem::chain::util::smt::generation_unit

namespace ico {

void payout( database& db, const asset_symbol_type& symbol, const account_object& account, const std::vector< asset >& assets )
{
   for ( auto& _asset : assets )
   {
      if ( _asset.symbol.is_vesting() )
      {
         db.create_vesting( account, asset( _asset.amount, _asset.symbol.get_paired_symbol() ) );
      }
      else
      {
         db.adjust_balance( account, _asset );

         if ( _asset.symbol.space() == asset_symbol_type::smt_nai_space )
            db.adjust_supply( _asset );
      }

      if ( _asset.symbol == STEEM_SYMBOL )
      {
         db.modify( db.get< smt_ico_object, by_symbol >( symbol ), [&]( smt_ico_object& obj )
         {
            obj.processed_contributions += _asset.amount;
         } );
      }
   }
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

struct payout_vars {
   share_type steem_units_sent;
   share_type unit_ratio;
};

static payout_vars calculate_payout_vars( const smt_ico_object& ico, const smt_generation_unit& generation_unit, share_type contribution_amount )
{
   payout_vars vars;

   vars.steem_units_sent = contribution_amount / generation_unit.steem_unit_sum();
   auto contributed_steem_units = ico.contributed.amount / generation_unit.steem_unit_sum();
   auto steem_units_hard_cap = ico.steem_units_hard_cap / generation_unit.steem_unit_sum();

   auto generated_token_units = std::min(
      contributed_steem_units * ico.capped_generation_policy.max_unit_ratio,
      steem_units_hard_cap * ico.capped_generation_policy.min_unit_ratio
   );

   vars.unit_ratio = generated_token_units / contributed_steem_units;

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
      payout_action.symbol = itr->symbol;

      const auto& ico = db.get< smt_ico_object, by_symbol >( itr->symbol );

      smt_generation_unit effective_generation_unit;

      if ( ico.processed_contributions < ico.steem_units_soft_cap )
         effective_generation_unit = ico.capped_generation_policy.pre_soft_cap_unit;
      else
         effective_generation_unit = ico.capped_generation_policy.post_soft_cap_unit;

      const auto& token_unit = effective_generation_unit.token_unit;
      const auto& steem_unit = effective_generation_unit.steem_unit;

      auto vars = calculate_payout_vars( ico, effective_generation_unit, itr->contribution.amount );

      for ( auto& e : token_unit )
      {
         if ( !smt::generation_unit::is_contributor( e.first ) )
            continue;

         auto token_shares = e.second * vars.steem_units_sent * vars.unit_ratio;

         asset_symbol_type symbol = itr->symbol;

         if ( smt::generation_unit::is_vesting( e.first ) )
            symbol = symbol.get_paired_symbol();

         payout_action.payouts.push_back( asset( token_shares, symbol ) );
      }

      for ( auto& e : steem_unit )
      {
         if ( !smt::generation_unit::is_contributor( e.first ) )
            continue;

         auto steem_shares = e.second * vars.steem_units_sent;

         asset_symbol_type symbol = STEEM_SYMBOL;

         if ( smt::generation_unit::is_vesting( e.first ) )
            symbol = symbol.get_paired_symbol();

         payout_action.payouts.push_back( asset( steem_shares, symbol ) );
      }

      db.push_required_action( payout_action );
      action_scheduled = true;
   }

   return action_scheduled;
}

bool schedule_founder_payout( database& db, const asset_symbol_type& a )
{
   bool action_scheduled = false;
   const auto& ico = db.get< smt_ico_object, by_symbol >( a );

   using generation_unit_share = std::tuple< smt_generation_unit, share_type >;

   std::vector< generation_unit_share > generation_unit_shares;
   if ( ico.contributed.amount > ico.steem_units_soft_cap )
   {
      generation_unit_shares.push_back( std::make_tuple( ico.capped_generation_policy.pre_soft_cap_unit, ico.steem_units_soft_cap ) );
      generation_unit_shares.push_back( std::make_tuple( ico.capped_generation_policy.post_soft_cap_unit, ico.contributed.amount - ico.steem_units_soft_cap ) );
   }
   else
   {
      generation_unit_shares.push_back( std::make_tuple( ico.capped_generation_policy.pre_soft_cap_unit, ico.contributed.amount ) );
   }

   using founder_asset_symbol_types = std::tuple< account_name_type, asset_symbol_type >;
   std::map< founder_asset_symbol_types, share_type > founder_payout_map;

   for ( auto& effective_generation_unit_shares : generation_unit_shares )
   {
      const auto& effective_generation_unit = std::get< 0 >( effective_generation_unit_shares );
      const auto& contributed_amount        = std::get< 1 >( effective_generation_unit_shares );

      const auto& token_unit = effective_generation_unit.token_unit;
      const auto& steem_unit = effective_generation_unit.steem_unit;

      auto vars = calculate_payout_vars( ico, effective_generation_unit, contributed_amount );

      for ( auto& e : token_unit )
      {
         if ( !smt::generation_unit::is_founder( e.first ) )
            continue;

         auto token_shares = e.second * vars.steem_units_sent * vars.unit_ratio;

         asset_symbol_type symbol = a;

         if ( smt::generation_unit::is_vesting( e.first ) )
            symbol = symbol.get_paired_symbol();

         auto map_key = std::make_tuple( e.first, symbol );
         if ( founder_payout_map.find ( map_key ) != founder_payout_map.end() )
            founder_payout_map[ map_key ] += token_shares;
         else
            founder_payout_map[ map_key ] = token_shares;
      }

      for ( auto& e : steem_unit )
      {
         if ( !smt::generation_unit::is_founder( e.first ) )
            continue;

         auto steem_shares = e.second * vars.steem_units_sent;

         asset_symbol_type symbol = STEEM_SYMBOL;

         if ( smt::generation_unit::is_vesting( e.first ) )
            symbol = symbol.get_paired_symbol();

         auto map_key = std::make_tuple( e.first, symbol );
         if ( founder_payout_map.find ( map_key ) != founder_payout_map.end() )
            founder_payout_map[ map_key ] += steem_shares;
         else
            founder_payout_map[ map_key ] = steem_shares;
      }
   }

   if ( founder_payout_map.size() > 0 )
   {
      smt_founder_payout_action payout_action;
      payout_action.symbol = a;

      for ( auto it = founder_payout_map.begin(); it != founder_payout_map.end(); ++it )
         payout_action.payouts[ std::get< 0 >( it->first ) ].push_back( asset( it->second, std::get< 1 >( it->first ) ) );

      db.push_required_action( payout_action );
      action_scheduled = true;
   }

   return action_scheduled;
}

} // steem::chain::util::smt::ico

} } } } // steem::chain::util::smt

#endif
