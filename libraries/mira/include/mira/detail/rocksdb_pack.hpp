#pragma once

#include <rocksdb/slice.h>

#include <fc/io/raw.hpp>

namespace mira { namespace multi_index { namespace detail {

template< typename T >
void pack_to_slice( ::rocksdb::PinnableSlice& s, T& t )
{
   auto v = fc::raw::pack_to_vector( t );
   s.PinSelf( ::rocksdb::Slice( v.data(), v.size() ) );
}

template< typename T >
::rocksdb::Slice pack_to_slice( T& t )
{
   ::rocksdb::PinnableSlice s;
   pack_to_slice( s, t );
   return s;
}

template< typename T >
void unpack_from_slice( const ::rocksdb::Slice& s, T& t )
{
   fc::raw::unpack_from_char_array< T >( s.data(), s.size(), t );
}

template< typename T >
T unpack_from_slice( const ::rocksdb::Slice& s )
{
   T t;
   unpack_from_slice( s, t );
   return t;
}

} } }
