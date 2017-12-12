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

#define STEEM_ASSET_SYMBOL_MAX_LENGTH  12

namespace
{

template <typename T>
class conversion_helper
   {
   public:
      typedef std::array<char, std::numeric_limits<T>::digits10 + 2> buffer_t;
      typedef typename buffer_t::const_iterator const_iterator;

      /// Returns max. number of potential decimal digits to be produced from value of given type 
      size_t maxDigits() const
      {
         return buffer.size();
      }

      std::pair<const_iterator, const_iterator> to_decimal_string(T& v, unsigned int forceDecimalDigits)
      {
         FC_ASSERT(forceDecimalDigits < maxDigits());

         auto insertionPoint = buffer.rbegin();
         size_t produced = 0;
         do
         {
            char d = char( v%10 );
            v /= 10;
            *insertionPoint = '0' + d;

            if(++produced < forceDecimalDigits)
               ++insertionPoint;
            else
               break;
         }
         while(insertionPoint != buffer.rend());

         return std::make_pair(insertionPoint.base() - 1, buffer.end());
      }

      /// Allows to convert given value to decimal string
      void to_decimal_string(std::string* output, T& v)
      {
         auto insertionPoint = buffer.rbegin();

         while(1)
         {
            assert(insertionPoint != buffer.rend());

            char d = char( v%10 );
            v /= 10;
            *insertionPoint = '0' + d;

            if(v == 0)
               break;

            ++insertionPoint;
         }

         output->append(insertionPoint.base() - 1, buffer.end());
      }

   private:
      buffer_t buffer;
   };

} /// namespace anonymous

void asset_symbol_type::to_string(std::string* buffer) const
{
   union
   {
      uint64_t  encodedValue;
      char      buffer[sizeof(encodedValue)+1];
   } helper;

   switch( asset_num )
   {
      case STEEM_ASSET_NUM_STEEM:
         helper.encodedValue = STEEM_SYMBOL_U64;
         buffer->append(helper.buffer);
         break;
      case STEEM_ASSET_NUM_SBD:
         helper.encodedValue = SBD_SYMBOL_U64;
         buffer->append(helper.buffer);
         break;
      case STEEM_ASSET_NUM_VESTS:
         helper.encodedValue = VESTS_SYMBOL_U64;
         buffer->append(helper.buffer);
         break;
      default:
      {
         FC_ASSERT(space() == smt_nai_space);

         uint32_t x = to_nai();
         buffer->append("@@");

         conversion_helper<uint32_t> helper;
         helper.to_decimal_string(buffer, x);
      }
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
      case '@':
      {
         ++p;
         FC_ASSERT( (*p) == '@', "Cannot parse asset symbol" );
         ++p;

         uint64_t nai = 0;
         int digit_count = 0;
         while( true )
         {
            switch( *p )
            {
               case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
               {
                  uint64_t new_nai = nai*10 + ((*p) - '0');
                  FC_ASSERT( new_nai > nai, "Cannot parse asset amount" );
                  FC_ASSERT( new_nai <= SMT_MAX_NAI, "Cannot parse asset amount" );
                  nai = new_nai;
                  ++p;
                  ++digit_count;
                  continue;
               }
               default:
                  break;
            }
            break;
         }
         FC_ASSERT( digit_count == 9 );
         asset_num = asset_num_from_nai( nai, uint8_t( decimal_places ) );
         break;
      }
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

// Highly optimized implementation of Damm algorithm
// https://en.wikipedia.org/wiki/Damm_algorithm
uint8_t damm_checksum_8digit(uint32_t value)
{
   FC_ASSERT( value < 100000000 );

   static const uint8_t t[] = {
       0, 30, 10, 70, 50, 90, 80, 60, 40, 20,
      70,  0, 90, 20, 10, 50, 40, 80, 60, 30,
      40, 20,  0, 60, 80, 70, 10, 30, 50, 90,
      10, 70, 50,  0, 90, 80, 30, 40, 20, 60,
      60, 10, 20, 30,  0, 40, 50, 90, 70, 80,
      30, 60, 70, 40, 20,  0, 90, 50, 80, 10,
      50, 80, 60, 90, 70, 20,  0, 10, 30, 40,
      80, 90, 40, 50, 30, 60, 20,  0, 10, 70,
      90, 40, 30, 80, 60, 10, 70, 20,  0, 50,
      20, 50, 80, 10, 40, 30, 60, 70, 90, 0
   };

   uint32_t q0 = value/10;
   uint32_t d0 = value%10;
   uint32_t q1 = q0/10;
   uint32_t d1 = q0%10;
   uint32_t q2 = q1/10;
   uint32_t d2 = q1%10;
   uint32_t q3 = q2/10;
   uint32_t d3 = q2%10;
   uint32_t q4 = q3/10;
   uint32_t d4 = q3%10;
   uint32_t q5 = q4/10;
   uint32_t d5 = q4%10;
   uint32_t d6 = q5%10;
   uint32_t d7 = q5/10;

   uint8_t x = t[d7];
   x = t[x+d6];
   x = t[x+d5];
   x = t[x+d4];
   x = t[x+d3];
   x = t[x+d2];
   x = t[x+d1];
   x = t[x+d0];
   return x/10;
}

uint32_t asset_symbol_type::asset_num_from_nai( uint32_t nai, uint8_t decimal_places )
{
   FC_ASSERT( decimal_places <= STEEM_ASSET_MAX_DECIMALS, "Invalid decimal_places" );
   uint32_t nai_check_digit = nai % 10;
   uint32_t nai_data_digits = nai / 10;
   FC_ASSERT( (nai_data_digits >= SMT_MIN_NAI) & (nai_data_digits <= SMT_MAX_NAI), "NAI out of range" );
   FC_ASSERT( nai_check_digit == damm_checksum_8digit(nai_data_digits), "Invalid check digit" );
   return (nai_data_digits << 5) | 0x10 | decimal_places;
}

uint32_t asset_symbol_type::to_nai()const
{
   FC_ASSERT( space() == smt_nai_space );
   uint32_t nai_data_digits = (asset_num >> 5);
   uint32_t nai_check_digit = damm_checksum_8digit(nai_data_digits);
   return nai_data_digits * 10 + nai_check_digit;
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
         s = smt_nai_space;
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
      {
         uint32_t nai_data_digits = (asset_num >> 5);
         uint32_t nai_1bit = (asset_num & 0x10);
         uint32_t nai_decimal_places = (asset_num & 0x0F);
         FC_ASSERT( (nai_data_digits >= SMT_MIN_NAI) &
                    (nai_data_digits <= SMT_MAX_NAI) &
                    (nai_1bit == 0x10) &
                    (nai_decimal_places <= STEEM_ASSET_MAX_DECIMALS),
                    "Cannot determine space for asset ${n}", ("n", asset_num) );
      }
   }
   // this assert is duplicated by above code in all cases
   // FC_ASSERT( decimals() <= STEEM_ASSET_MAX_DECIMALS );
}

void asset::validate()const
{
   symbol.validate();
   FC_ASSERT( amount.value >= 0 );
   FC_ASSERT( amount.value <= STEEM_MAX_SATOSHIS );
}

std::string asset::to_string()const
{
   validate();
   
   conversion_helper<uint64_t> helper, decimalsHelper;

   std::string retVal;
   //               amount-string     dot      decimals       space       symbol-string
   retVal.reserve(helper.maxDigits() + 1  + symbol.decimals() + 1 + STEEM_ASSET_SYMBOL_MAX_LENGTH);

   uint64_t v = uint64_t( amount.value );

   auto decimals = decimalsHelper.to_decimal_string(v, symbol.decimals());
   helper.to_decimal_string(&retVal, v);

   retVal += '.';
   retVal.append(decimals.first, decimals.second);

   retVal += ' ';

   symbol.to_string(&retVal);

   return retVal;
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
   int digits = 0;

   // [0-9]+([.][0-9]*)?\s
   while( true )
   {
      switch( *p )
      {
         case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
            value = value*10 + uint64_t((*p) - '0');
            FC_ASSERT( value <= STEEM_MAX_SATOSHIS, "Cannot parse asset amount" );
            ++p;
            ++digits;
            FC_ASSERT( digits <= 19, "Cannot parse asset amount" );
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

#define BQ(a) \
   std::tie( a.base.symbol, a.quote.symbol )

#define DEFINE_PRICE_COMPARISON_OPERATOR( op ) \
bool operator op ( const price& a, const price& b ) \
{ \
   if( BQ(a) != BQ(b) ) \
      return BQ(a) op BQ(b); \
   \
   const uint128_t amult = uint128_t( b.quote.amount.value ) * a.base.amount.value; \
   const uint128_t bmult = uint128_t( a.quote.amount.value ) * b.base.amount.value; \
   \
   return amult op bmult;  \
}

DEFINE_PRICE_COMPARISON_OPERATOR( == )
DEFINE_PRICE_COMPARISON_OPERATOR( != )
DEFINE_PRICE_COMPARISON_OPERATOR( <  )
DEFINE_PRICE_COMPARISON_OPERATOR( <= )
DEFINE_PRICE_COMPARISON_OPERATOR( >  )
DEFINE_PRICE_COMPARISON_OPERATOR( >= )

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

      price price::max( asset_symbol_type base, asset_symbol_type quote ) { return asset( share_type(STEEM_MAX_SATOSHIS), base ) / asset( share_type(1), quote); }
      price price::min( asset_symbol_type base, asset_symbol_type quote ) { return asset( 1, base ) / asset( STEEM_MAX_SATOSHIS, quote); }

      bool price::is_null() const { return *this == price(); }

      void price::validate() const
      { try {
         FC_ASSERT( base.amount > share_type(0) );
         FC_ASSERT( quote.amount > share_type(0) );
         FC_ASSERT( base.symbol != quote.symbol );
      } FC_CAPTURE_AND_RETHROW( (base)(quote) ) }


} } // steem::protocol
