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

account_name_type get_account( const account_name_type& unit_target, const account_name_type& from )
{
   if ( unit_target == SMT_DESTINATION_FROM )
      return from;
   if ( unit_target == SMT_DESTINATION_FROM_VESTING )
      return from;

   for ( auto& e : { SMT_DESTINATION_MARKET_MAKER, SMT_DESTINATION_VESTING, SMT_DESTINATION_REWARDS } )
      FC_ASSERT( unit_target != e,
         "The provided unit target '${unit_target}' is not an account.", ("unit_target", e) );

   return unit_target;
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

void launch( database& db, const asset_symbol_type& a )
{
   const smt_token_object& token = db.get< smt_token_object, by_symbol >( a );
   const smt_ico_object& ico = db.get< smt_ico_object, by_symbol >( token.liquid_symbol );

   db.modify( token, []( smt_token_object& o )
   {
      o.phase = smt_phase::contribution_begin_time_completed;
   } );

   schedule_next_event( db, token.liquid_symbol, protocol::smt_event::ico_evaluation, ico.contribution_end_time );
}

void evaluate( database& db, const asset_symbol_type& a )
{
   const smt_token_object& token = db.get< smt_token_object, by_symbol >( a );
   const smt_ico_object& ico = db.get< smt_ico_object, by_symbol >( token.liquid_symbol );

   // ICO Success
   if ( ico.contributed.amount >= ico.steem_units_min )
   {
      db.modify( token, []( smt_token_object& o )
      {
         o.phase = smt_phase::contribution_end_time_completed;
      } );

      schedule_next_event( db, token.liquid_symbol, protocol::smt_event::token_launch, ico.launch_time );
   }
   // ICO Failure
   else
   {
      db.modify( token, []( smt_token_object& o )
      {
         o.phase = smt_phase::launch_failed;
         o.next_event = smt_event::none;
      } );

      db.remove( ico );

      schedule_next_refund( db, token.liquid_symbol );
   }
}

void launch_token( database& db, const asset_symbol_type& a )
{
   const smt_token_object& token = db.get< smt_token_object, by_symbol >( a );

   db.modify( token, []( smt_token_object& o )
   {
      o.phase = smt_phase::launch_success;
      o.next_event = smt_event::none;
   } );

   /*
    * If there are no contributions to schedule payouts for we no longer require
    * the ICO object.
    */
   if ( !schedule_next_payout( db, token.liquid_symbol ) )
   {
      db.remove( db.get< smt_ico_object, by_symbol >( token.liquid_symbol ) );
   }
}

using cascading_action_func = std::function< const required_automated_action&( const smt_contribution_object& ) >;

static bool cascading_action(
   database& db,
   const asset_symbol_type& a,
   cascading_action_func f,
   uint32_t interval )
{
   bool action_scheduled = false;
   auto& idx = db.get_index< smt_contribution_index, by_symbol_id >();
   auto itr = idx.lower_bound( boost::make_tuple( a, 0 ) );

   if ( itr != idx.end() && itr->symbol == a )
   {
      db.push_required_action( f( *itr ), db.head_block_time() + interval );
      action_scheduled = true;
   }

   return action_scheduled;
}

void schedule_next_event( database& db, const asset_symbol_type& a, smt_event event, fc::time_point_sec time )
{
   const auto& token = db.get< smt_token_object, by_symbol >( a );

   smt_event_action action;
   action.symbol = a;
   action.event = event;

   db.push_required_action( action, time );

   db.modify( token, [&]( smt_token_object& o )
   {
      o.next_event = event;
   } );
}

bool schedule_next_refund( database& db, const asset_symbol_type& a )
{
   return cascading_action( db, a, []( const smt_contribution_object& o) {
      smt_refund_action refund;
      refund.symbol = o.symbol;
      refund.contributor = o.contributor;
      refund.contribution_id = o.contribution_id;
      return refund;
   }, SMT_REFUND_INTERVAL );
}

bool schedule_next_payout( database& db, const asset_symbol_type& a )
{
   return cascading_action( db, a, []( const smt_contribution_object& o) {
      smt_contributor_payout_action contributor_payout;
      contributor_payout.symbol = o.symbol;
      contributor_payout.contributor = o.contributor;
      contributor_payout.contribution_id = o.contribution_id;
      return contributor_payout;
   }, SMT_CONTRIBUTOR_PAYOUT_INTERVAL );
}

} // steem::chain::util::smt::ico

} } } } // steem::chain::util::smt

#endif
