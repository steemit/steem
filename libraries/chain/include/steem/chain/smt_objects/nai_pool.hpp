#pragma once
#include <steem/chain/database.hpp>
#include <steem/protocol/asset_symbol.hpp>

#ifdef STEEM_ENABLE_SMT

namespace steem { namespace chain {

   void replenish_nai_pool( database& db );
   void remove_from_nai_pool( database &db, const asset_symbol_type& a );

} } // steem::chain

#endif
