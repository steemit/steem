#pragma once
#include <dpn/chain/database.hpp>
#include <dpn/protocol/asset_symbol.hpp>

#ifdef DPN_ENABLE_SMT

namespace dpn { namespace chain {

   void replenish_nai_pool( database& db );
   void remove_from_nai_pool( database &db, const asset_symbol_type& a );

} } // dpn::chain

#endif
