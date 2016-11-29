#include <steemit/protocol/version.hpp>

#include <fc/exception/exception.hpp>
#include <fc/variant.hpp>

#include <sstream>

namespace steemit { namespace protocol {

/* Quick conversion utilities from http://joelverhagen.com/blog/2010/11/convert-an-int-to-a-string-and-vice-versa-in-c/ */
inline int string_to_int( fc::string input )
{
   std::stringstream s( input );
   int i;
   s >> i;
   return i;
}

inline fc::string int_to_string( int input )
{
   std::stringstream s;
   s << input;
   return s.str();
}

version::version( uint8_t m, uint8_t h, uint16_t r )
{
   v_num = ( 0 | m ) << 8;
   v_num = ( v_num | h ) << 16;
   v_num =   v_num | r;
}

version::operator fc::string()const
{
   std::stringstream s;
   s << ( ( v_num >> 24 ) & 0x000000FF )
     << '.'
     << ( ( v_num >> 16 ) & 0x000000FF )
     << '.'
     << ( ( v_num & 0x0000FFFF ) );

   return s.str();
}

} } // steemit::protocol

namespace fc
{
   void to_variant( const steemit::protocol::version& v, variant& var )
   {
      var = fc::string( v );
   }

   void from_variant( const variant& var, steemit::protocol::version& v )
   {
      uint32_t major = 0, hardfork = 0, revision = 0;
      char dot_a = 0, dot_b = 0;

      std::stringstream s( var.as_string() );
      s >> major >> dot_a >> hardfork >> dot_b >> revision;

      // We'll accept either m.h.v or m_h_v as canonical version strings
      FC_ASSERT( ( dot_a == '.' || dot_a == '_' ) && dot_a == dot_b, "Variant does not contain proper dotted decimal format" );
      FC_ASSERT( major <= 0xFF, "Major version is out of range" );
      FC_ASSERT( hardfork <= 0xFF, "Hardfork version is out of range" );
      FC_ASSERT( revision <= 0xFFFF, "Revision version is out of range" );
      FC_ASSERT( s.eof(), "Extra information at end of version string" );

      v.v_num = 0 | ( major << 24 ) | ( hardfork << 16 ) | revision;
   }

   void to_variant( const steemit::protocol::hardfork_version& hv, variant& var )
   {
      to_variant( (const steemit::protocol::version&) hv, var );
   }

   void from_variant( const variant& var, steemit::protocol::hardfork_version& hv )
   {
      steemit::protocol::version ver;
      from_variant( var, ver );
      hv.v_num = ver.v_num & 0xffff0000;
   }
}
