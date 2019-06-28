#pragma once

#include <dpn/protocol/asset.hpp>

namespace dpn { namespace chain { namespace util {

using dpn::protocol::asset;
using dpn::protocol::price;

inline asset to_dbd( const price& p, const asset& dpn )
{
   FC_ASSERT( dpn.symbol == DPN_SYMBOL );
   if( p.is_null() )
      return asset( 0, DBD_SYMBOL );
   return dpn * p;
}

inline asset to_dpn( const price& p, const asset& dbd )
{
   FC_ASSERT( dbd.symbol == DBD_SYMBOL );
   if( p.is_null() )
      return asset( 0, DPN_SYMBOL );
   return dbd * p;
}

} } }
