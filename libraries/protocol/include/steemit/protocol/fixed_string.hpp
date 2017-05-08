#pragma once

#include <fc/uint128.hpp>
#include <fc/io/raw_fwd.hpp>

namespace steemit { namespace protocol {

/**
 * This class is an in-place memory allocation of a 16 character string.
 *
 * The string will serialize the same way as std::string for variant and raw formats,
 * but will be in memory as a 128-bit integer so that we can exploit efficient
 * integer comparisons for sorting.
 */
class fixed_string
{
   public:
      typedef fc::uint128_t Storage;

      fixed_string(){}
      fixed_string( const fixed_string& c ) : data( c.data ){}
      fixed_string( const char* str ) : fixed_string( std::string( str ) ) {}
      fixed_string( const std::string& str );

      operator std::string()const;

      uint32_t size()const;

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

} } // steemit::protocol

namespace fc { namespace raw {

   template< typename Stream >
   inline void pack( Stream& s, const steemit::protocol::fixed_string& u )
   {
      pack( s, std::string( u ) );
   }

   template< typename Stream >
   inline void unpack( Stream& s, steemit::protocol::fixed_string& u )
   {
      std::string str;
      unpack( s, str );
      u = str;
   }

} // raw

   void to_variant(   const steemit::protocol::fixed_string& s, variant& v );
   void from_variant( const variant& v, steemit::protocol::fixed_string& s );
} // fc
