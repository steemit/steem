#pragma once

#include <fc/uint128.hpp>
#include <fc/io/raw_fwd.hpp>

// These overloads need to be defined before the implementation in fixed_string
namespace fc
{
   uint128 endian_reverse( const uint128& u );
}

namespace std
{
   // The template below does not work crossing namespaces from std to fc
   pair< fc::uint128_t, uint64_t > endian_reverse( const pair< fc::uint128_t, uint64_t > p );

   pair< fc::uint128_t, fc::uint128_t > endian_reverse( const pair< fc::uint128_t, fc::uint128_t > p );

   template< typename T, typename V >
   pair< T, V > endian_reverse( const pair< T, V > p )
   {
      return std::make_pair( boost::endian::endian_reverse( p.first ), boost::endian::endian_reverse( p.second ) );
   }
}

namespace steemit { namespace protocol {

/**
 * This class is an in-place memory allocation of a fixed length character string.
 *
 * The string will serialize the same way as std::string for variant and raw formats.
 */
template< typename S = uint64_t, typename T = uint64_t >
class fixed_string
{
   public:
      typedef std::pair< S, T > Storage;

      fixed_string(){}
      fixed_string( const fixed_string& c ) : data( c.data ){}
      fixed_string( const char* str ) : fixed_string( std::string( str ) ) {}
      fixed_string( const std::string& str )
      {
         Storage d;
         if( str.size() <= sizeof(d) )
            memcpy( (char*)&d, str.c_str(), str.size() );
         else
            memcpy( (char*)&d, str.c_str(), sizeof(d) );

         data = boost::endian::big_to_native( d );
      }

      operator std::string()const
      {
         Storage d = boost::endian::native_to_big( data );
         size_t s;

         if( *(((const char*)&d) + sizeof(d) - 1) )
            s = sizeof(d);
         else
            s = strnlen( (const char*)&d, sizeof(d) );

         const char* self = (const char*)&d;

         return std::string( self, self + s );
      }

      uint32_t size()const
      {
         Storage d = boost::endian::native_to_big( data );
         if( *(((const char*)&d) + sizeof(d) - 1) )
            return sizeof(d);
         return strnlen( (const char*)&d, sizeof(d) );
      }

      uint32_t length()const { return size(); }

      fixed_string& operator = ( const fixed_string& str )
      {
         data = str.data;
         return *this;
      }

      fixed_string& operator = ( const char* str )
      {
         *this = fixed_string( str );
         return *this;
      }

      fixed_string& operator = ( const std::string& str )
      {
         *this = fixed_string( str );
         return *this;
      }

      friend std::string operator + ( const fixed_string& a, const std::string& b ) { return std::string( a ) + b; }
      friend std::string operator + ( const std::string& a, const fixed_string& b ){ return a + std::string( b ); }
      friend bool operator < ( const fixed_string& a, const fixed_string& b ) { return a.data < b.data; }
      friend bool operator <= ( const fixed_string& a, const fixed_string& b ) { return a.data <= b.data; }
      friend bool operator > ( const fixed_string& a, const fixed_string& b ) { return a.data > b.data; }
      friend bool operator >= ( const fixed_string& a, const fixed_string& b ) { return a.data >= b.data; }
      friend bool operator == ( const fixed_string& a, const fixed_string& b ) { return a.data == b.data; }
      friend bool operator != ( const fixed_string& a, const fixed_string& b ) { return a.data != b.data; }

      Storage data;
};

// These storage types work with memory layout and should be used instead of a custom template.
typedef fixed_string<>                                fixed_string_16;
typedef fixed_string< fc::uint128_t, uint64_t >       fixed_string_24;
typedef fixed_string< fc::uint128_t, fc::uint128_t >  fixed_string_32;

} } // steemit::protocol

namespace fc { namespace raw {

   template< typename Stream, typename S, typename T >
   inline void pack( Stream& s, const steemit::protocol::fixed_string< S, T >& u )
   {
      pack( s, std::string( u ) );
   }

   template< typename Stream, typename S, typename T >
   inline void unpack( Stream& s, steemit::protocol::fixed_string< S, T >& u )
   {
      std::string str;
      unpack( s, str );
      u = str;
   }

} // raw
   template< typename S, typename T >
   void to_variant(   const steemit::protocol::fixed_string< S, T>& s, variant& v ) { v = std::string( s ); }

   template< typename S, typename T >
   void from_variant( const variant& v, steemit::protocol::fixed_string< S, T >& s ) { s = v.as_string(); }
} // fc
