#include <steem/chain/util/smt_token.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/smt_objects/smt_token_object.hpp>
#include <steem/protocol/asset_symbol.hpp>

namespace steem { namespace chain { namespace util {

const smt_token_object* smt_token_lookup( database& db, uint32_t nai )
{
   const auto& idx = db.get_index< smt_token_index >().indices().get< by_symbol >();

   auto range = idx.range(
      [nai] ( const asset_symbol_type& a ) { return a >= asset_symbol_type::from_nai( nai, 0 ); },
      [nai] ( const asset_symbol_type& a ) { return a <= asset_symbol_type::from_nai( nai, STEEM_ASSET_MAX_DECIMALS ); }
   );

   for ( const auto& record : boost::make_iterator_range( range ) )
      return &record;

   return nullptr;
}

const smt_token_object* smt_token_lookup( database& db, asset_symbol_type symbol, bool precision_agnostic )
{
   // Only liquid symbols are stored in the smt token index
   auto s = symbol.is_vesting() ? symbol.get_paired_symbol() : symbol;

   if ( precision_agnostic )
      return smt_token_lookup( db, s.to_nai() );
   else
      return db.find< smt_token_object, by_symbol >( s );
}

} } } // steem::chain::util
