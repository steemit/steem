#include <steemit/chain/protocol/version.hpp>

#include <fc/exception/exception.hpp>
#include <fc/variant.hpp>

#define TO_UTF8 0x30

namespace steemit { namespace chain {

version::version( uint8_t m, uint8_t h, uint8_t r )
{
   v_num = ( 0 | m ) << 8;
   v_num = ( v_num | h ) << 8;
   v_num =   v_num | r;
}

version::operator fc::string()const
{
   char data[6];
   data[0] = ( ( v_num >> 16 ) & 0x000000FF ) + TO_UTF8;
   data[1] = '.';
   data[2] = ( ( v_num >> 8 ) & 0x000000FF ) + TO_UTF8;
   data[3] = '.';
   data[4] = ( v_num & 0x000000FF ) + TO_UTF8;
   data[5] = 0;

   return fc::string( data );
}

} } // steemit::chain

namespace fc
{
   void to_variant( const steemit::chain::version& v, variant& var )
   {
      var = fc::string( v );
   }

   void from_variant( const variant& var, steemit::chain::version& v )
   {
      auto var_str = var.as_string();
      FC_ASSERT( var_str.length() == 5, "Variant is incorrect length for version" );
      const char* data = var_str.data();
      FC_ASSERT( data[1] == '.' && data[3] == '.', "Variant does not contain proper dotted decimal format" );

      v.v_num = ( 0 | ( data[0] - TO_UTF8 ) ) << 8;
      v.v_num = ( v.v_num | ( data[2] - TO_UTF8 ) ) << 8;
      v.v_num = v.v_num | ( data[4] - TO_UTF8 );
   }
}