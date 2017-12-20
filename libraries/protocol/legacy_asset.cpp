#include <steem/protocol/legacy_asset.hpp>

namespace steem { namespace protocol {

uint8_t legacy_asset::decimals()const
{
   auto a = (const char*)&symbol.ser;
   uint8_t result = uint8_t( a[0] );
   FC_ASSERT( result < 15 );
   return result;
}

void legacy_asset::set_decimals(uint8_t d)
{
   FC_ASSERT( d < 15 );
   auto a = (char*)&symbol;
   a[0] = d;
}

std::string legacy_asset::symbol_name()const
{
   auto a = (const char*)&symbol;
   FC_ASSERT( a[7] == 0 );
   return &a[1];
}

int64_t legacy_asset::precision()const
{
   static int64_t table[] = {
                     1, 10, 100, 1000, 10000,
                     100000, 1000000, 10000000, 100000000ll,
                     1000000000ll, 10000000000ll,
                     100000000000ll, 1000000000000ll,
                     10000000000000ll, 100000000000000ll
                     };
   uint8_t d = decimals();
   return table[ d ];
}

string legacy_asset::to_string()const
{
   int64_t prec = precision();
   string result = fc::to_string(amount.value / prec);
   if( prec > 1 )
   {
      auto fract = amount.value % prec;
      // prec is a power of ten, so for example when working with
      // 7.005 we have fract = 5, prec = 1000.  So prec+fract=1005
      // has the correct number of zeros and we can simply trim the
      // leading 1.
      result += "." + fc::to_string(prec + fract).erase(0,1);
   }
   return result + " " + symbol_name();
}

legacy_asset legacy_asset::from_string( const string& from )
{
   try
   {
      string s = fc::trim( from );
      auto space_pos = s.find( " " );
      auto dot_pos = s.find( "." );

      FC_ASSERT( space_pos != std::string::npos );

      legacy_asset result;
      result.symbol.ser = uint64_t(0);
      auto sy = (char*)&result.symbol.ser;

      if( dot_pos != std::string::npos )
      {
         FC_ASSERT( space_pos > dot_pos );

         auto intpart = s.substr( 0, dot_pos );
         auto fractpart = "1" + s.substr( dot_pos + 1, space_pos - dot_pos - 1 );
         result.set_decimals( fractpart.size() - 1 );

         result.amount = fc::to_int64( intpart );
         result.amount.value *= result.precision();
         result.amount.value += fc::to_int64( fractpart );
         result.amount.value -= result.precision();
      }
      else
      {
         auto intpart = s.substr( 0, space_pos );
         result.amount = fc::to_int64( intpart );
         result.set_decimals( 0 );
      }
      auto symbol = s.substr( space_pos + 1 );
      size_t symbol_size = symbol.size();

      if( symbol_size > 0 )
      {
         FC_ASSERT( symbol_size <= 6 );
         memcpy( sy+1, symbol.c_str(), symbol_size );
      }

      return result;
   }
   FC_CAPTURE_AND_RETHROW( (from) )
}

} } // steem::protocol
