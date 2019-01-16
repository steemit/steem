#pragma once

#include <mira/composite_key_fwd.hpp>

#include <rocksdb/slice.h>

#include <boost/type_traits/integral_constant.hpp>

#include <fc/time.hpp>

namespace mira {

using ::rocksdb::Slice;
using ::rocksdb::PinnableSlice;

template< typename T > inline void pack_to_slice( PinnableSlice&, const T& );
//template< typename T > inline Slice pack_to_slice( const T& t );

template< typename T > inline void unpack_from_slice( const Slice& s, T& t );
//template< typename T > inline T unpack_from_slice( const Slice& s );

template< typename T > struct is_static_length : public boost::false_type {};

template<> inline void pack_to_slice( PinnableSlice&, const bool& );
template<> inline void unpack_from_slice( const Slice&, bool& );
template<> struct is_static_length< bool > : public boost::true_type {};

template<> inline void pack_to_slice( PinnableSlice&, const char& );
template<> inline void unpack_from_slice( const Slice&, char& );

template<> inline void pack_to_slice( PinnableSlice&, const int16_t& );
template<> inline void unpack_from_slice( const Slice&, int16_t& );

template<> inline void pack_to_slice( PinnableSlice&, const uint16_t& );
template<> inline void unpack_from_slice( const Slice&, uint16_t& );

template<> inline void pack_to_slice( PinnableSlice&, const int32_t& );
template<> inline void unpack_from_slice( const Slice&, int32_t& );

template<> inline void pack_to_slice( PinnableSlice&, const uint32_t& );
template<> inline void unpack_from_slice( const Slice&, uint32_t& );

template<> inline void pack_to_slice( PinnableSlice&, const int64_t& );
template<> inline void unpack_from_slice( const Slice&, int64_t& );

template<> inline void pack_to_slice( PinnableSlice&, const uint64_t& );
template<> inline void unpack_from_slice( const Slice&, uint64_t& );

/*
fc::time_point_sec
fc::uin128_t
fc::sha256
reflected types
safe< T >
cons
*/

template<> inline void pack_to_slice( PinnableSlice&, const fc::time_point_sec& );
template<> inline void unpack_from_slice( const Slice&, fc::time_point_sec& );

} // mira
