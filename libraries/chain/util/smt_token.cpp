#include <steem/chain/util/smt_token.hpp>

#ifdef STEEM_ENABLE_SMT

namespace steem { namespace chain { namespace util { namespace smt {

const smt_token_object* find_token( database& db, uint32_t nai )
{
   const auto& idx = db.get_index< smt_token_index >().indices().get< by_symbol >();

   auto range = idx.range(
      [nai] ( const asset_symbol_type& a ) { return a >= asset_symbol_type::from_nai( nai, 0 ); },
      [nai] ( const asset_symbol_type& a ) { return a <= asset_symbol_type::from_nai( nai, STEEM_ASSET_MAX_DECIMALS ); }
   );

   /*
    * There *should* be only 1 record within the provided range, this is why we simply return
    * the address of the first element within it.
    */
   for ( const auto& record : boost::make_iterator_range( range ) )
      return &record;

   return nullptr;
}

const smt_token_object* find_token( database& db, asset_symbol_type symbol, bool precision_agnostic )
{
   // Only liquid symbols are stored in the smt token index
   auto s = symbol.is_vesting() ? symbol.get_paired_symbol() : symbol;

   if ( precision_agnostic )
      return find_token( db, s.to_nai() );
   else
      return db.find< smt_token_object, by_symbol >( s );
}

fc::optional< time_point_sec > last_emission_time( database& db, const asset_symbol_type& symbol )
{
   fc::optional< time_point_sec > end_time{};

   const auto& idx = db.get_index< smt_token_emissions_index >().indices().get< by_symbol >();
   auto range = idx.equal_range( symbol );

   for ( const auto& r : boost::make_iterator_range( range ) )
   {
      time_point_sec schedule_end;

      // A maximum interval count indicates we should emit indefinitely
      if ( r.interval_count == std::numeric_limits< uint32_t >::max() )
         return time_point_sec::maximum();
      else
         schedule_end = r.schedule_time + fc::seconds( uint64_t( r.interval_seconds ) * uint64_t( r.interval_count ) );

      if ( end_time.valid() )
         end_time = schedule_end > *end_time ? schedule_end : end_time;
      else
         end_time = schedule_end;
   }

   return end_time;
}

} } } } // steem::chain::util::smt

#endif
