#include <steem/protocol/asset.hpp>
#include <boost/rational.hpp>
#include <boost/multiprecision/cpp_int.hpp>

/*

The bounds on asset serialization are as follows:

index : field
0     : decimals
1..6  : symbol
   7  : \0
*/

namespace steem { namespace protocol {

void asset_symbol_type::to_string( char* buf )const
{
   uint64_t* p = (uint64_t*) buf;
   static_assert( STEEM_ASSET_SYMBOL_MAX_LENGTH >= 7, "This code will overflow a short buffer" );

   switch( asset_num )
   {
      case STEEM_ASSET_NUM_STEEM:
         (*p) = STEEM_SYMBOL_U64;
         break;
      case STEEM_ASSET_NUM_SBD:
         (*p) = SBD_SYMBOL_U64;
         break;
      case STEEM_ASSET_NUM_VESTS:
         (*p) = VESTS_SYMBOL_U64;
         break;
      default:
         FC_ASSERT( false, "Cannot convert unknown asset symbol ${n} to string", ("n", asset_num) );
   }
}

asset_symbol_type asset_symbol_type::from_string( const char* p, uint8_t decimal_places )
{
   // \s*
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

   // [A-Z]{1,6}
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
               FC_ASSERT( decimal_places == 3, "Incorrect decimal places" );
               asset_num = STEEM_ASSET_NUM_STEEM;
               break;
            case SBD_SYMBOL_U64:
               FC_ASSERT( decimal_places == 3, "Incorrect decimal places" );
               asset_num = STEEM_ASSET_NUM_SBD;
               break;
            case VESTS_SYMBOL_U64:
               FC_ASSERT( decimal_places == 6, "Incorrect decimal places" );
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

   asset_symbol_type sym;
   sym.asset_num = asset_num;
   return sym;
}

asset_symbol_type::asset_symbol_space asset_symbol_type::space()const
{
   asset_symbol_type::asset_symbol_space s = legacy_space;
   switch( asset_num )
   {
      case STEEM_ASSET_NUM_STEEM:
      case STEEM_ASSET_NUM_SBD:
      case STEEM_ASSET_NUM_VESTS:
         s = legacy_space;
         break;
      default:
         FC_ASSERT( false, "Cannot determine space for asset ${n}", ("n", asset_num) );
   }
   return s;
}

void asset_symbol_type::validate()const
{
   switch( asset_num )
   {
      case STEEM_ASSET_NUM_STEEM:
      case STEEM_ASSET_NUM_SBD:
      case STEEM_ASSET_NUM_VESTS:
         break;
      default:
         FC_ASSERT( false, "Cannot determine space for asset ${n}", ("n", asset_num) );
   }
   FC_ASSERT( decimals() <= STEEM_ASSET_MAX_DECIMALS );
}

void asset::validate()const
{
   symbol.validate();
   FC_ASSERT( amount.value >= 0 );
   FC_ASSERT( amount.value <= STEEM_MAX_SHARE_SUPPLY );
}

std::string asset::to_string()const
{
   validate();

   static_assert( STEEM_MAX_SHARE_SUPPLY <= int64_t(9999999999999999ll), "BEGIN_OFFSET needs to be adjusted if MAX_SHARE_SUPPLY is increased" );
   static const int BEGIN_OFFSET = 17;
   //       amount string  space   symbol string                   null terminator
   char buf[BEGIN_OFFSET + 1     + STEEM_ASSET_SYMBOL_MAX_LENGTH + 1];
   char* p = buf+BEGIN_OFFSET-1;
   uint64_t v = uint64_t( amount.value );
   int decimal_places = int(symbol.decimals());

   p[1] = ' ';
   symbol.to_string( p+2 );

   for( int i=0; i<decimal_places; i++ )
   {
      char d = char( v%10 );
      v /= 10;
      (*p) = '0' + d;
      --p;
   }

   (*p) = '.';

   do
   {
      --p;
      char d = char( v%10 );
      v /= 10;
      (*p) = '0' + d;
   } while( v != 0 );

   return std::string(p);
}

void asset::fill_from_string( const char* p )
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

   int decimal_places = 0;
   int decimal_places_inc = 0;
   uint64_t value = 0;

   // [0-9]+([.][0-9]*)?\s
   while( true )
   {
      switch( *p )
      {
         case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
            value = value*10 + uint64_t((*p) - '0');
            FC_ASSERT( value <= STEEM_MAX_SHARE_SUPPLY, "Cannot parse asset amount" );
            ++p;
            decimal_places += decimal_places_inc;
            continue;
         case '.':
            FC_ASSERT( decimal_places_inc == 0, "Cannot parse asset amount" );
            decimal_places_inc = 1;
            ++p;
            continue;
         case ' ':  case '\t':  case '\n':  case '\r':
            ++p;
            break;
         default:
            FC_ASSERT( false, "Cannot parse asset amount" );
      }
      break;
   }

   FC_ASSERT( decimal_places <= STEEM_ASSET_MAX_DECIMALS, "Cannot parse asset amount" );

   symbol = asset_symbol_type::from_string( p, uint8_t(decimal_places) );
   amount.value = int64_t( value );
   return;
}

      typedef boost::multiprecision::int128_t  int128_t;

      bool operator == ( const price& a, const price& b )
      {
         if( std::tie( a.base.symbol, a.quote.symbol ) != std::tie( b.base.symbol, b.quote.symbol ) )
             return false;

         const auto amult = uint128_t( b.quote.amount.value ) * a.base.amount.value;
         const auto bmult = uint128_t( a.quote.amount.value ) * b.base.amount.value;

         return amult == bmult;
      }

      bool operator < ( const price& a, const price& b )
      {
         if( a.base.symbol < b.base.symbol ) return true;
         if( a.base.symbol > b.base.symbol ) return false;
         if( a.quote.symbol < b.quote.symbol ) return true;
         if( a.quote.symbol > b.quote.symbol ) return false;

         const auto amult = uint128_t( b.quote.amount.value ) * a.base.amount.value;
         const auto bmult = uint128_t( a.quote.amount.value ) * b.base.amount.value;

         return amult < bmult;
      }

      bool operator <= ( const price& a, const price& b )
      {
         return (a == b) || (a < b);
      }

      bool operator != ( const price& a, const price& b )
      {
         return !(a == b);
      }

      bool operator > ( const price& a, const price& b )
      {
         return !(a <= b);
      }

      bool operator >= ( const price& a, const price& b )
      {
         return !(a < b);
      }

      asset operator * ( const asset& a, const price& b )
      {
         if( a.symbol == b.base.symbol )
         {
            FC_ASSERT( b.base.amount.value > 0 );
            uint128_t result = (uint128_t(a.amount.value) * b.quote.amount.value)/b.base.amount.value;
            FC_ASSERT( result.hi == 0 );
            return asset( result.to_uint64(), b.quote.symbol );
         }
         else if( a.symbol == b.quote.symbol )
         {
            FC_ASSERT( b.quote.amount.value > 0 );
            uint128_t result = (uint128_t(a.amount.value) * b.base.amount.value)/b.quote.amount.value;
            FC_ASSERT( result.hi == 0 );
            return asset( result.to_uint64(), b.base.symbol );
         }
         FC_THROW_EXCEPTION( fc::assert_exception, "invalid asset * price", ("asset",a)("price",b) );
      }

      price operator / ( const asset& base, const asset& quote )
      { try {
         FC_ASSERT( base.symbol != quote.symbol );
         return price{ base, quote };
      } FC_CAPTURE_AND_RETHROW( (base)(quote) ) }

      price price::max( asset_symbol_type base, asset_symbol_type quote ) { return asset( share_type(STEEM_MAX_SHARE_SUPPLY), base ) / asset( share_type(1), quote); }
      price price::min( asset_symbol_type base, asset_symbol_type quote ) { return asset( 1, base ) / asset( STEEM_MAX_SHARE_SUPPLY, quote); }

      bool price::is_null() const { return *this == price(); }

      void price::validate() const
      { try {
         FC_ASSERT( base.amount > share_type(0) );
         FC_ASSERT( quote.amount > share_type(0) );
         FC_ASSERT( base.symbol != quote.symbol );
      } FC_CAPTURE_AND_RETHROW( (base)(quote) ) }


} } // steem::protocol
