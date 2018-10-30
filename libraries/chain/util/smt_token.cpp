#include <steem/chain/util/smt_token.hpp>

#ifdef STEEM_ENABLE_SMT

namespace steem { namespace chain { namespace util {

const smt_token_object* find_smt_token( database& db, uint32_t nai )
{
   const auto& idx = db.get_index< smt_token_index >().indices().get< by_symbol >();

   auto range = idx.range(
      [nai] ( const asset_symbol_type& a ) { return a >= asset_symbol_type::from_nai( nai, 0 ); },
      [nai] ( const asset_symbol_type& a ) { return a <= asset_symbol_type::from_nai( nai, STEEM_ASSET_MAX_DECIMALS ); }
   );

   /*
    * There *should* be only 1 record within the provided range, this is why we simple return
    * the address of the first element within it.
    */
   for ( const auto& record : boost::make_iterator_range( range ) )
      return &record;

   return nullptr;
}

const smt_token_object* find_smt_token( database& db, asset_symbol_type symbol, bool precision_agnostic )
{
   // Only liquid symbols are stored in the smt token index
   auto s = symbol.is_vesting() ? symbol.get_paired_symbol() : symbol;

   if ( precision_agnostic )
      return find_smt_token( db, s.to_nai() );
   else
      return db.find< smt_token_object, by_symbol >( s );
}

} } } // steem::chain::util

#endif
