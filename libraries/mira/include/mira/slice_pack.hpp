#pragma once

#include <rocksdb/slice.h>

#include <boost/type_traits/integral_constant.hpp>
#include <boost/tuple/tuple.hpp>

#include <fc/time.hpp>
#include <fc/uint128.hpp>
#include <fc/crypto/sha256.hpp>

namespace mira {

using ::rocksdb::Slice;
using ::rocksdb::PinnableSlice;

template< typename T > struct is_static_length : public boost::false_type {};
template<> struct is_static_length< bool > : public boost::true_type {};
template<> struct is_static_length< char > : public boost::true_type {};
template<> struct is_static_length< int16_t > : public boost::true_type {};
template<> struct is_static_length< uint16_t > : public boost::true_type {};
template<> struct is_static_length< int32_t > : public boost::true_type {};
template<> struct is_static_length< uint32_t > : public boost::true_type {};
template<> struct is_static_length< int64_t > : public boost::true_type {};
template<> struct is_static_length< uint64_t > : public boost::true_type {};

template<> struct is_static_length< fc::time_point_sec > : public boost::true_type {};
template<> struct is_static_length< fc::uint128_t > : public boost::true_type {};
template<> struct is_static_length< fc::sha256 > : public boost::true_type {};
template< typename T > struct is_static_length< fc::safe< T > > : public is_static_length< T > {};

template<> struct is_static_length< boost::tuples::null_type > : public boost::true_type {};

template< typename HT, typename TT >
struct is_static_length< boost::tuples::cons< HT, TT > >
{
   static const bool value = is_static_length< HT >::value && is_static_length< TT >::value;
};

template< typename T > struct slice_packer
{
   static void pack( PinnableSlice& s, const T& t )
   {
      if( is_static_length< T >::value )
      {
         s.PinSelf( Slice( (char*)&t, sizeof(t) ) );
      }
      else
      {
         auto v = fc::raw::pack_to_vector( t );
         s.PinSelf( Slice( v.data(), v.size() ) );
      }
   }

   static void unpack( const Slice& s, T& t )
   {
      if( is_static_length< T >::value )
      {
         t = *(T*)s.data();
      }
      else
      {
         fc::raw::unpack_from_char_array< T >( s.data(), s.size(), t );
      }
   }
};

template< typename T >
void pack_to_slice( PinnableSlice& s, const T& t )
{
   slice_packer< T >::pack( s, t );
}

template< typename T >
void unpack_from_slice( const Slice& s, T& t )
{
   slice_packer< T >::unpack( s, t );
}

template< typename T >
T unpack_from_slice( const Slice& s )
{
   T t;
   unpack_from_slice( s, t );
   return t;
}

} // mira
