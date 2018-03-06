#include <steem/protocol/asset.hpp>

#include <fc/io/json.hpp>

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

std::string asset_symbol_type::to_string()const
{
   return fc::json::to_string( fc::variant( *this ) );
}

asset_symbol_type asset_symbol_type::from_string( const std::string& str )
{
   return fc::json::from_string( str ).as< asset_symbol_type >();
}

void asset_symbol_type::to_nai_string( char* buf )const
{
   static_assert( STEEM_ASSET_SYMBOL_NAI_STRING_LENGTH >= 12, "This code will overflow a short buffer" );
   uint32_t x = to_nai();
   buf[11] = '\0';
   buf[10] = ((x%10)+'0');  x /= 10;
   buf[ 9] = ((x%10)+'0');  x /= 10;
   buf[ 8] = ((x%10)+'0');  x /= 10;
   buf[ 7] = ((x%10)+'0');  x /= 10;
   buf[ 6] = ((x%10)+'0');  x /= 10;
   buf[ 5] = ((x%10)+'0');  x /= 10;
   buf[ 4] = ((x%10)+'0');  x /= 10;
   buf[ 3] = ((x%10)+'0');  x /= 10;
   buf[ 2] = ((x   )+'0');
   buf[ 1] = '@';
   buf[ 0] = '@';
}

asset_symbol_type asset_symbol_type::from_nai_string( const char* p, uint8_t decimal_places )
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
                  FC_ASSERT( new_nai >= nai, "Cannot parse asset amount" ); // This is failing for system assets
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

   const uint8_t t[] = {
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
   // Can be replaced with some clever bitshifting
   uint32_t nai_check_digit = nai % 10;
   uint32_t nai_data_digits = nai / 10;

   FC_ASSERT( (nai_data_digits >= SMT_MIN_NAI) & (nai_data_digits <= SMT_MAX_NAI), "NAI out of range" );
   FC_ASSERT( nai_check_digit == damm_checksum_8digit(nai_data_digits), "Invalid check digit" );

   switch( nai_data_digits )
   {
      case STEEM_NAI_STEEM:
         FC_ASSERT( decimal_places == STEEM_PRECISION_STEEM );
         return STEEM_ASSET_NUM_STEEM;
      case STEEM_NAI_SBD:
         FC_ASSERT( decimal_places == STEEM_PRECISION_SBD );
         return STEEM_ASSET_NUM_SBD;
      case STEEM_NAI_VESTS:
         FC_ASSERT( decimal_places == STEEM_PRECISION_VESTS );
         return STEEM_ASSET_NUM_VESTS;
      default:
         FC_ASSERT( decimal_places <= STEEM_ASSET_MAX_DECIMALS, "Invalid decimal_places" );
         return (nai_data_digits << 5) | 0x10 | decimal_places;
   }
}

uint32_t asset_symbol_type::to_nai()const
{
   uint32_t nai_data_digits = 0;

   // Can be replaced with some clever bitshifting
   switch( asset_num )
   {
      case STEEM_ASSET_NUM_STEEM:
         nai_data_digits = STEEM_NAI_STEEM;
         break;
      case STEEM_ASSET_NUM_SBD:
         nai_data_digits = STEEM_NAI_SBD;
         break;
      case STEEM_ASSET_NUM_VESTS:
         nai_data_digits = STEEM_NAI_VESTS;
         break;
      default:
         FC_ASSERT( space() == smt_nai_space );
         nai_data_digits = (asset_num >> 5);
   }

   uint32_t nai_check_digit = damm_checksum_8digit(nai_data_digits);
   return nai_data_digits * 10 + nai_check_digit;
}

bool asset_symbol_type::is_vesting() const
{
   switch( space() )
   {
      case legacy_space:
      {
         switch( asset_num )
         {
            case STEEM_ASSET_NUM_STEEM:
               return false;
            case STEEM_ASSET_NUM_SBD:
               // SBD is certainly liquid.
               return false;
            case STEEM_ASSET_NUM_VESTS:
               return true;
            default:
               FC_ASSERT( false, "Unknown asset symbol" );
         }
      }
      case smt_nai_space:
         // 6th bit of asset_num is used as vesting/liquid variant indicator.
         return asset_num & 0x00000020;
      default:
         FC_ASSERT( false, "Unknown asset symbol" );
   }
}

asset_symbol_type asset_symbol_type::get_paired_symbol() const
{
   switch( space() )
   {
      case legacy_space:
      {
         switch( asset_num )
         {
            case STEEM_ASSET_NUM_STEEM:
               return from_asset_num( STEEM_ASSET_NUM_VESTS );
            case STEEM_ASSET_NUM_SBD:
               return *this;
            case STEEM_ASSET_NUM_VESTS:
               return from_asset_num( STEEM_ASSET_NUM_STEEM );
            default:
               FC_ASSERT( false, "Unknown asset symbol" );
         }
      }
      case smt_nai_space:
         {
         // Toggle 6th bit of this asset_num.
         auto paired_asset_num = asset_num ^ ( 0x1 << 5 );
         return from_asset_num( paired_asset_num );
         }
      default:
         FC_ASSERT( false, "Unknown asset symbol" );
   }
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

namespace fc {
   void to_variant( const steem::protocol::asset& var, fc::variant& vo )
   {
      try
      {
         variant v = mutable_variant_object( "amount", boost::lexical_cast< std::string >( var.amount.value ) )
                                          ( "precision", uint64_t( var.symbol.decimals() ) )
                                          ( "nai", var.symbol.to_nai_string() );
         vo = v;
      } FC_CAPTURE_AND_RETHROW()
   }

   void from_variant( const fc::variant& var, steem::protocol::asset& vo )
   {
      try
      {
         FC_ASSERT( var.is_object(), "Asset has to treated as object." );

         const auto& v_object = var.get_object();

         FC_ASSERT( v_object.contains( "amount" ), "Amount field doesn't exist." );
         vo.amount = boost::lexical_cast< int64_t >( v_object[ "amount" ].as< std::string >() );
         FC_ASSERT( vo.amount >= 0, "Asset amount cannot be negative" );

         FC_ASSERT( v_object.contains( "precision" ), "Precision field doesn't exist." );
         FC_ASSERT( v_object.contains( "nai" ), "NAI field doesn't exist." );
         vo.symbol = steem::protocol::asset_symbol_type::from_nai_string( v_object[ "nai" ].as< std::string >().c_str(), v_object[ "precision" ].as< uint8_t >() );
      } FC_CAPTURE_AND_RETHROW()
   }
}
