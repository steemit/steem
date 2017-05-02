#include <steemit/protocol/fixed_string.hpp>

#include <boost/endian/conversion.hpp>

namespace fc {
   void to_variant(   const steemit::protocol::fixed_string& s, variant& v ) { v = std::string( s ); }
   void from_variant( const variant& v, steemit::protocol::fixed_string& s ) { s = v.as_string(); }

   uint128 endian_reverse( const uint128& u )
   {
      return uint128( boost::endian::endian_reverse( u.hi ), boost::endian::endian_reverse( u.lo ) );
   }
} // fc

namespace steemit { namespace protocol {

fixed_string::fixed_string( const std::string& str )
{
   Storage d;
   if( str.size() <= sizeof(d) )
      memcpy( (char*)&d, str.c_str(), str.size() );
   else
      memcpy( (char*)&d, str.c_str(), sizeof(d) );

   data = boost::endian::big_to_native( d );
}

fixed_string::operator std::string()const
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

uint32_t fixed_string::size()const
{
   Storage d = boost::endian::native_to_big( data );
   if( *(((const char*)&d) + sizeof(d) - 1) )
      return sizeof(d);
   return strnlen( (const char*)&d, sizeof(d) );
}

} } // steemit::protocol