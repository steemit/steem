#pragma once

#include <fc/io/raw.hpp>
#include <steem/protocol/types_fwd.hpp>

#define STEEM_ASSET_SYMBOL_PRECISION_BITS 4
#define SMT_MAX_NAI                       99999999
#define SMT_MIN_NAI                       1
#define STEEM_ASSET_SYMBOL_MAX_LENGTH     10

#define STEEM_ASSET_NUM_SBD \
  (((SMT_MAX_NAI+1) << STEEM_ASSET_SYMBOL_PRECISION_BITS) | 3)
#define STEEM_ASSET_NUM_STEEM \
  (((SMT_MAX_NAI+2) << STEEM_ASSET_SYMBOL_PRECISION_BITS) | 3)
#define STEEM_ASSET_NUM_VESTS \
  (((SMT_MAX_NAI+3) << STEEM_ASSET_SYMBOL_PRECISION_BITS) | 6)

#ifdef IS_TEST_NET

#define VESTS_SYMBOL_U64  (uint64_t('V') | (uint64_t('E') << 8) | (uint64_t('S') << 16) | (uint64_t('T') << 24) | (uint64_t('S') << 32))
#define STEEM_SYMBOL_U64  (uint64_t('T') | (uint64_t('E') << 8) | (uint64_t('S') << 16) | (uint64_t('T') << 24) | (uint64_t('S') << 32))
#define SBD_SYMBOL_U64    (uint64_t('T') | (uint64_t('B') << 8) | (uint64_t('D') << 16))

#else

#define VESTS_SYMBOL_U64  (uint64_t('V') | (uint64_t('E') << 8) | (uint64_t('S') << 16) | (uint64_t('T') << 24) | (uint64_t('S') << 32))
#define STEEM_SYMBOL_U64  (uint64_t('S') | (uint64_t('T') << 8) | (uint64_t('E') << 16) | (uint64_t('E') << 24) | (uint64_t('M') << 32))
#define SBD_SYMBOL_U64    (uint64_t('S') | (uint64_t('B') << 8) | (uint64_t('D') << 16))

#endif

#define VESTS_SYMBOL_SER  (uint64_t(6) | (VESTS_SYMBOL_U64 << 8)) ///< VESTS with 6 digits of precision
#define STEEM_SYMBOL_SER  (uint64_t(3) | (STEEM_SYMBOL_U64 << 8)) ///< STEEM with 3 digits of precision
#define SBD_SYMBOL_SER    (uint64_t(3) |   (SBD_SYMBOL_U64 << 8)) ///< Test Backed Dollars with 3 digits of precision

#define STEEM_ASSET_MAX_DECIMALS 12

namespace steem { namespace protocol {

class asset_symbol_type
{
   public:
      enum asset_symbol_space
      {
         legacy_space = 1
      };

      asset_symbol_type() {}

      // buf must have space for STEEM_ASSET_SYMBOL_MAX_LENGTH+1
      static asset_symbol_type from_string( const char* buf, uint8_t decimal_places );
      static asset_symbol_type from_asset_num( uint32_t asset_num )
      {   asset_symbol_type result;   result.asset_num = asset_num;   return result;   }
      void to_string( const char* buf )const;
      std::string to_string()const
      {
         char buf[ STEEM_ASSET_SYMBOL_MAX_LENGTH+1 ];
         to_string( buf );
         return std::string(buf);
      }

      asset_symbol_space space()const;
      uint8_t decimals()const
      {  return uint8_t( asset_num & 0x0F );    }
      void validate()const;

      friend bool operator == ( const asset_symbol_type& a, const asset_symbol_type& b )
      {  return (a.asset_num == b.asset_num);   }
      friend bool operator != ( const asset_symbol_type& a, const asset_symbol_type& b )
      {  return (a.asset_num != b.asset_num);   }
      friend bool operator <  ( const asset_symbol_type& a, const asset_symbol_type& b )
      {  return (a.asset_num <  b.asset_num);   }
      friend bool operator >  ( const asset_symbol_type& a, const asset_symbol_type& b )
      {  return (a.asset_num >  b.asset_num);   }
      friend bool operator <= ( const asset_symbol_type& a, const asset_symbol_type& b )
      {  return (a.asset_num <= b.asset_num);   }
      friend bool operator >= ( const asset_symbol_type& a, const asset_symbol_type& b )
      {  return (a.asset_num >= b.asset_num);   }

      uint32_t asset_num = 0;
};

} } // steem::protocol

namespace fc { namespace raw {

template< typename Stream >
inline void pack( Stream& s, const steem::protocol::asset_symbol_type& sym )
{
   uint64_t ser = 0;

   switch( sym.asset_num )
   {
      case STEEM_ASSET_NUM_STEEM:
         ser = STEEM_SYMBOL_SER;
         break;
      case STEEM_ASSET_NUM_SBD:
         ser = SBD_SYMBOL_SER;
         break;
      case STEEM_ASSET_NUM_VESTS:
         ser = VESTS_SYMBOL_SER;
         break;
      default:
         FC_ASSERT( false, "Cannot serialize unknown asset symbol" );
   }
   pack( s, ser );
}

template< typename Stream >
inline void unpack( Stream& s, steem::protocol::asset_symbol_type& sym )
{
   uint64_t ser = 0;
   fc::raw::unpack( s, ser );
   switch( ser )
   {
      case STEEM_SYMBOL_SER:
         sym.asset_num = STEEM_ASSET_NUM_STEEM;
         break;
      case SBD_SYMBOL_SER:
         sym.asset_num = STEEM_ASSET_NUM_SBD;
         break;
      case VESTS_SYMBOL_SER:
         sym.asset_num = STEEM_ASSET_NUM_VESTS;
         break;
   }
}

} // fc::raw

inline void to_variant( const steem::protocol::asset_symbol_type& sym, fc::variant& v )
{
   v = sym.to_string();
}

/*
inline void from_variant( const fc::variant& v,  steem::protocol::asset_symbol_type& sym )
{
   sym.from_string( sym.as_string().c_str(), 0 );
}
*/

} // fc
