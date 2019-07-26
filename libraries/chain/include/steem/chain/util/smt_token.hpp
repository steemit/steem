#pragma once

#ifdef STEEM_ENABLE_SMT

#include <fc/optional.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/smt_objects/smt_token_object.hpp>
#include <steem/protocol/asset_symbol.hpp>

namespace steem { namespace chain { namespace util { namespace smt {

const smt_token_object* find_token( const database& db, uint32_t nai );
const smt_token_object* find_token( const database& db, asset_symbol_type symbol, bool precision_agnostic = false );
const smt_token_emissions_object* last_emission( const database& db, const asset_symbol_type& symbol );
fc::optional< time_point_sec > last_emission_time( const database& db, const asset_symbol_type& symbol );

namespace ico {

void launch( database& db, const asset_symbol_type& obj );
void evaluate( database& db, const asset_symbol_type& obj );
void launch_token( database& db, const asset_symbol_type& obj );
bool schedule_next_refund( database& db, const asset_symbol_type& a );
bool schedule_next_payout( database& db, const asset_symbol_type& a );
void schedule_next_event( database& db, const asset_symbol_type& a, smt_event event, fc::time_point_sec time );

using cascading_action_applier_do_func      = std::function< void( database&, const smt_contribution_object& ) >;
using cascading_action_applier_then_func    = std::function< bool( database&, const asset_symbol_type& ) >;
using cascading_action_applier_finally_func = std::function< void( database&, const asset_symbol_type& ) >;

template< class ActionType >
void cascading_action_applier(
   database &db,
   ActionType a,
   cascading_action_applier_do_func _do,
   cascading_action_applier_then_func _then,
   fc::optional< cascading_action_applier_finally_func > _finally = fc::optional< cascading_action_applier_finally_func >() )
{
   auto& idx = db.get_index< smt_contribution_index, by_symbol_contributor >();
   auto itr = idx.find( boost::make_tuple( a.symbol, a.contributor, a.contribution_id ) );
   FC_ASSERT( itr != idx.end(), "Unable to find contribution object for the provided action: ${a}", ("a", a) );

   auto symbol = itr->symbol;

   _do( db, *itr );

   db.remove( *itr );

   /*
    * We only cascade through actions when producing blocks
    * to avoid generating large amounts of automated actions
    * at once.
    */
   if ( !db.is_pending_tx() )
   {
      if ( !_then( db, symbol ) && _finally )
      {
         (*_finally)( db, symbol );
      }
   }
}

} // steem::chain::util::smt::ico

namespace generation_unit {

account_name_type get_account( const account_name_type& unit_target, const account_name_type& from );
bool is_vesting( const account_name_type& name );

} // steem::chain::util::smt::generation_unit

} } } } // steem::chain::util::smt

#endif
