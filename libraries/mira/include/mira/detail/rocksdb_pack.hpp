#pragma once

#include <mira/detail/rocksdb_pack_fwd.hpp>

#include <fc/io/raw.hpp>

#include <fc/log/logger.hpp>

namespace mira {

struct is_static_cons;

template<>
inline void pack_to_slice( PinnableSlice& s, const bool& t )
{
   s.PinSelf( Slice( (char*)&t, sizeof(t) ) );
}

template<>
inline void unpack_from_slice( const Slice& s, bool& t )
{
   t = *(bool*)s.data();
}


template<>
inline void pack_to_slice( PinnableSlice& s, const char& t )
{
   s.PinSelf( Slice( (char*)&t, sizeof(t) ) );
}

template<>
inline void unpack_from_slice( const Slice& s, char& t )
{
   t = *s.data();
}


template<>
inline void pack_to_slice( PinnableSlice& s, const int16_t& t )
{
   s.PinSelf( Slice( (char*)&t, sizeof(t) ) );
}

template<>
inline void unpack_from_slice( const Slice& s, int16_t& t )
{
   t = *(int16_t*)s.data();
}


template<>
inline void pack_to_slice( PinnableSlice& s, const uint16_t& t )
{
   s.PinSelf( Slice( (char*)&t, sizeof(t) ) );
}

template<>
inline void unpack_from_slice( const Slice& s, uint16_t& t )
{
   t = *(uint16_t*)s.data();
}


template<>
inline void pack_to_slice( PinnableSlice& s, const int32_t& t )
{
   s.PinSelf( Slice( (char*)&t, sizeof(t) ) );
}

template<>
inline void unpack_from_slice( const Slice& s, int32_t& t )
{
   t = *(int32_t*)s.data();
}


template<>
inline void pack_to_slice( PinnableSlice& s, const uint32_t& t )
{
   s.PinSelf( Slice( (char*)&t, sizeof(t) ) );
}

template<>
inline void unpack_from_slice( const Slice& s, uint32_t& t )
{
   t = *(uint32_t*)s.data();
}

template<>
inline void pack_to_slice( PinnableSlice& s, const int64_t& t )
{
   s.PinSelf( Slice( (char*)&t, sizeof(t) ) );
}

template<>
inline void unpack_from_slice( const Slice& s, int64_t& t )
{
   t = *(int64_t*)s.data();
}

template<>
inline void pack_to_slice( PinnableSlice& s, const uint64_t& t )
{
   s.PinSelf( Slice( (char*)&t, sizeof(t) ) );
}

template<>
inline void unpack_from_slice( const Slice& s, uint64_t& t )
{
   t = *(uint64_t*)s.data();
}

/*
fc::time_point_sec
fc::uin128_t
fc::sha256
reflected types
safe< T >
cons
*/


template<>
inline void pack_to_slice( PinnableSlice& s, const fc::time_point_sec& t )
{
   s.PinSelf( Slice( (char*)&t, sizeof(t) ) );
}

template<>
inline void unpack_from_slice( const Slice& s, fc::time_point_sec& t )
{
   t = *(fc::time_point_sec*)s.data();
}


template< typename T >
inline void pack_to_slice( PinnableSlice& s, const T& t )
{
   auto v = fc::raw::pack_to_vector( t );
   s.PinSelf( Slice( v.data(), v.size() ) );
}
//*
template< typename T >
inline Slice pack_to_slice( const T& t )
{
   PinnableSlice s;
   pack_to_slice( s, t );
   return s;
}
//*/

template< typename T >
inline void unpack_from_slice( const Slice& s, T& t )
{
   fc::raw::unpack_from_char_array< T >( s.data(), s.size(), t );
}
/*
template< typename T >
inline T unpack_from_slice( const Slice& s )
{
   T t;
   unpack_from_slice( s, t );
   return t;
}
*/

} // mira
