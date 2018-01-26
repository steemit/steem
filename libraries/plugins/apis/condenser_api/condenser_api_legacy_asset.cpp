#include <steem/plugins/condenser_api/condenser_api_legacy_asset.hpp>

namespace steem { namespace plugins { namespace condenser_api {

uint32_t string_to_asset_num( const char* p, uint8_t decimals )
{
   while( true )
   {
      switch( *p )
      {
         case ' ':  case '\t':  case '\n':  case '\r':
            ++p;
            continue;
         default:
            break;
      }
      break;
   }

   // [A-Z]
   uint32_t asset_num = 0;
   switch( *p )
   {
      case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
      case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
      case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
      {
         // [A-Z]{1,6}
         int shift = 0;
         uint64_t name_u64 = 0;
         while( true )
         {
            if( ((*p) >= 'A') && ((*p) <= 'Z') )
            {
               FC_ASSERT( shift < 64, "Cannot parse asset symbol" );
               name_u64 |= uint64_t(*p) << shift;
               shift += 8;
               ++p;
               continue;
            }
            break;
         }
         switch( name_u64 )
         {
            case STEEM_SYMBOL_U64:
               FC_ASSERT( decimals == 3, "Incorrect decimal places" );
               asset_num = STEEM_ASSET_NUM_STEEM;
               break;
            case SBD_SYMBOL_U64:
               FC_ASSERT( decimals == 3, "Incorrect decimal places" );
               asset_num = STEEM_ASSET_NUM_SBD;
               break;
            case VESTS_SYMBOL_U64:
               FC_ASSERT( decimals == 6, "Incorrect decimal places" );
               asset_num = STEEM_ASSET_NUM_VESTS;
               break;
            default:
               FC_ASSERT( false, "Cannot parse asset symbol" );
         }
         break;
      }
      default:
         FC_ASSERT( false, "Cannot parse asset symbol" );
   }

   // \s*\0
   while( true )
   {
      switch( *p )
      {
         case ' ':  case '\t':  case '\n':  case '\r':
            ++p;
            continue;
         case '\0':
            break;
         default:
            FC_ASSERT( false, "Cannot parse asset symbol" );
      }
      break;
   }

   return asset_num;
}

std::string asset_num_to_string( uint32_t asset_num )
{
   switch( asset_num )
   {
#ifdef IS_TEST_NET
      case STEEM_ASSET_NUM_STEEM:
         return "TESTS";
      case STEEM_ASSET_NUM_SBD:
         return "TBD";
#else
      case STEEM_ASSET_NUM_STEEM:
         return "STEEM";
      case STEEM_ASSET_NUM_SBD:
         return "SBD";
#endif
      case STEEM_ASSET_NUM_VESTS:
         return "VESTS";
      default:
         return "UNKN"; // SMTs will return this symbol if returned as a legacy asset
   }
}

int64_t precision( const asset_symbol_type& symbol )
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
   int64_t prec = precision(symbol);
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
   return result + " " + asset_num_to_string( symbol.asset_num );
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

      string str_symbol = s.substr( space_pos + 1 );

      if( dot_pos != std::string::npos )
      {
         FC_ASSERT( space_pos > dot_pos );

         auto intpart = s.substr( 0, dot_pos );
         auto fractpart = "1" + s.substr( dot_pos + 1, space_pos - dot_pos - 1 );
         uint8_t decimals = uint8_t( fractpart.size() - 1 );

         result.symbol.asset_num = string_to_asset_num( str_symbol.c_str(), decimals );

         int64_t prec = precision( result.symbol );

         result.amount = fc::to_int64( intpart );
         result.amount.value *= prec;
         result.amount.value += fc::to_int64( fractpart );
         result.amount.value -= prec;
      }
      else
      {
         auto intpart = s.substr( 0, space_pos );
         result.amount = fc::to_int64( intpart );
         result.symbol.asset_num = string_to_asset_num( str_symbol.c_str(), 0 );
      }
      return result;
   }
   FC_CAPTURE_AND_RETHROW( (from) )
}

} } }
