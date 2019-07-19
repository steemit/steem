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

void launch( database& db, const smt_ico_launch_queue_object& obj );
void evaluate( database& db, const smt_ico_evaluation_queue_object& obj );
void launch_token( database& db, const smt_token_launch_queue_object& obj );
bool schedule_next_refund( database& db, const asset_symbol_type& a );
bool schedule_next_payout( database& db, const asset_symbol_type& a );

template< class QueueIndex, class SortOrder, class QueueObject, uint64_t MaxPerBlock >
void process_queue( database& db, std::function< time_point_sec( const QueueObject& ) > get_time, std::function< void( database&, const QueueObject& ) > process_item )
{
   const auto& queue = db.get_index< QueueIndex, SortOrder >();
   std::size_t num_processed = 0;

   auto itr = queue.begin();
   while ( itr != queue.end() && num_processed < MaxPerBlock )
   {
      if ( db.head_block_time() < get_time( *itr ) )
         break;

      process_item( db, *itr );

      num_processed++;
      itr = queue.begin();
   }
}

template< class ActionType >
void cascading_action_applier(
   database &db,
   ActionType a,
   std::function< void( database&, const smt_contribution_object& ) > _do,
   std::function< bool( database&, const asset_symbol_type& ) > _then,
   fc::optional< std::function< void( database&, const asset_symbol_type& ) > > _finally = fc::optional< std::function< void( database&, const asset_symbol_type& ) > >() )
{
   auto& idx = db.get_index< smt_contribution_index, by_symbol_contributor >();
   auto itr = idx.find( boost::make_tuple( a.symbol, a.contributor, a.contribution_id ) );
   FC_ASSERT( itr != idx.end(), "Unable to find contribution object for the provided action: ${a}", ("a", a) );

   auto symbol = itr->symbol;

   _do( db, *itr );

   db.remove( *itr );

   if ( !_then( db, symbol ) && _finally )
   {
      (*_finally)( db, symbol );
   }
}

} // steem::chain::util::smt::ico

namespace generation_unit {

account_name_type get_account( const account_name_type& unit_target, const account_name_type& from );
bool is_vesting( const account_name_type& name );

} // steem::chain::util::smt::generation_unit

} } } } // steem::chain::util::smt

#endif
