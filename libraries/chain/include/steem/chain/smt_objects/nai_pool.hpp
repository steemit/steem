#pragma once

#ifdef STEEM_ENABLE_SMT

namespace steem { namespace chain {

   class database;
   //class asset_symbol_type;

   void replenish_nai_pool( database& db );
   void remove_from_nai_pool( database &db, const asset_symbol_type& a );

} }

#endif
