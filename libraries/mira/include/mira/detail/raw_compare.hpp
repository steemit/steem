#pragma once

#include <mira/multi_index_container_fwd.hpp>
#include <fc/io/raw_fwd.hpp>
#include <fc/io/raw.hpp>

namespace mira { namespace multi_index { namespace detail {

template< typename T, typename Stream, typename Compare >
int32_t raw_compare( Stream& s1, Stream& s2, const Compare& c )
{
   if( s1.size() == s2.size() && memcmp( s1.data(), s2.data(), s1.size() ) == 0 ) return 0;

   T t1;
   T t2;

   fc::raw::unpack( s1, t1 );
   fc::raw::unpack( s2, t2 );

   uint32_t r = c( t1, t2 );

   if( r ) return -1;

   return 1;
}

} } } // mira::multi_index::detail
