#include <steem/plugins/condenser_api/condenser_api_legacy_asset.hpp>

namespace steem { namespace plugins { namespace condenser_api {

int64_t legacy_asset::precision()const
{
   static int64_t table[] = {
                     1, 10, 100, 1000, 10000,
                     100000, 1000000, 10000000, 100000000ll,
                     1000000000ll, 10000000000ll,
                     100000000000ll, 1000000000000ll,
                     10000000000000ll, 100000000000000ll
                     };
   uint8_t d = symbol.decimals();
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
   return result + " " + symbol.to_string();
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
      uint8_t decimals = 0;

      if( dot_pos != std::string::npos )
      {
         FC_ASSERT( space_pos > dot_pos );

         auto intpart = s.substr( 0, dot_pos );
         auto fractpart = "1" + s.substr( dot_pos + 1, space_pos - dot_pos - 1 );
         decimals = uint8_t( fractpart.size() - 1 );

         result.amount = fc::to_int64( intpart );
         result.amount.value *= result.precision();
         result.amount.value += fc::to_int64( fractpart );
         result.amount.value -= result.precision();
      }
      else
      {
         auto intpart = s.substr( 0, space_pos );
         result.amount = fc::to_int64( intpart );
      }
      string str_symbol = s.substr( space_pos + 1 );
      result.symbol = asset_symbol_type::from_string( str_symbol.c_str(), decimals );
      return result;
   }
   FC_CAPTURE_AND_RETHROW( (from) )
}

} } }
