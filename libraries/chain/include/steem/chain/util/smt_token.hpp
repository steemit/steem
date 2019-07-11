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

void launch_ico( database& db, const smt_ico_launch_queue_object& obj );
void evaluate_ico( database& db, const smt_ico_evaluation_queue_object& obj );
void launch_token( database& db, const smt_token_launch_queue_object& obj );
void schedule_next_refund( database& db, const asset_symbol_type& a );

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

} } } } // steem::chain::util::smt

#endif
