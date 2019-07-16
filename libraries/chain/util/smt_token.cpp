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

void launch_ico( database& db, const smt_ico_launch_queue_object& ico_launch_obj )
{
   const smt_token_object& token = db.get< smt_token_object, by_symbol >( ico_launch_obj.symbol );
   const smt_ico_object& ico = db.get< smt_ico_object, by_symbol >( token.liquid_symbol );

   db.modify( token, []( smt_token_object& o )
   {
      o.phase = smt_phase::contribution_begin_time_completed;
   } );

   db.create< smt_ico_evaluation_queue_object >( [&]( smt_ico_evaluation_queue_object& obj )
   {
      obj.symbol = token.liquid_symbol;
      obj.contribution_end_time = ico.contribution_end_time;
   } );

   db.remove( ico_launch_obj );
}

void evaluate_ico( database& db, const smt_ico_evaluation_queue_object& ico_evaluation_obj )
{
   const smt_token_object& token = db.get< smt_token_object, by_symbol >( ico_evaluation_obj.symbol );
   const smt_ico_object& ico = db.get< smt_ico_object, by_symbol >( token.liquid_symbol );

   // ICO Success
   if ( ico.contributed.amount >= ico.steem_units_soft_cap )
   {
      db.modify( token, []( smt_token_object& o )
      {
         o.phase = smt_phase::contribution_end_time_completed;
      } );

      db.create< smt_token_launch_queue_object >( [&]( smt_token_launch_queue_object& o )
      {
         o.symbol = token.liquid_symbol;
         o.launch_time = ico.launch_time;
      } );
   }
   // ICO Failure
   else
   {
      db.modify( token, []( smt_token_object& o )
      {
         o.phase = smt_phase::launch_failed;
      } );

      db.remove( ico );

      schedule_next_refund( db, token.liquid_symbol );
   }

   db.remove( ico_evaluation_obj );
}

void launch_token( database& db, const smt_token_launch_queue_object& token_launch_obj )
{
   const smt_token_object& token = db.get< smt_token_object, by_symbol >( token_launch_obj.symbol );
   const smt_ico_object& ico = db.get< smt_ico_object, by_symbol >( token.liquid_symbol );

   FC_TODO( "Payout founders and contributors" );
   FC_TODO( "Issue #2732" );
   FC_TODO( "Issue #2733" );
   FC_TODO( "Issue #2734" );
   FC_TODO( "Issue #2735" );

   db.modify( token, []( smt_token_object& o )
   {
      o.phase = smt_phase::launch_success;
   } );

   db.remove( ico );
   db.remove( token_launch_obj );
}

static void cascading_contribution_action(
   database& db,
   const asset_symbol_type& a,
   std::function< const required_automated_action&( const smt_contribution_object& ) > f,
   uint32_t interval )
{
   auto& idx = db.get_index< smt_contribution_index, by_symbol_id >();
   auto itr = idx.lower_bound( boost::make_tuple( a, 0 ) );

   if ( itr != idx.end() && itr->symbol == a )
   {
      db.push_required_action( f( *itr ), db.head_block_time() + interval );
   }
}

void schedule_next_refund( database& db, const asset_symbol_type& a )
{
   cascading_contribution_action( db, a, []( const smt_contribution_object& o) {
      smt_refund_action refund;
      refund.symbol = o.symbol;
      refund.contributor = o.contributor;
      refund.contribution_id = o.contribution_id;
      return refund;
   }, SMT_REFUND_INTERVAL );
}

void schedule_next_contributor_payout( database& db, const asset_symbol_type& a )
{
//   cascading_contribution_action( db, a, []( const smt_contribution_object& o) {
//   }, SMT_CONTRIBUTOR_PAYOUT_INTERVAL );
}

} } } } // steem::chain::util::smt

#endif
