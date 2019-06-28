#pragma once

#include <dpn/protocol/asset.hpp>

#define DPN_SYMBOL_LEGACY_SER_1   (uint64_t(1) | (DPN_SYMBOL_U64 << 8))
#define DPN_SYMBOL_LEGACY_SER_2   (uint64_t(2) | (DPN_SYMBOL_U64 << 8))
#define DPN_SYMBOL_LEGACY_SER_3   (uint64_t(5) | (DPN_SYMBOL_U64 << 8))
#define DPN_SYMBOL_LEGACY_SER_4   (uint64_t(3) | (uint64_t('0') << 8) | (uint64_t('.') << 16) | (uint64_t('0') << 24) | (uint64_t('0') << 32) | (uint64_t('1') << 40))
#define DPN_SYMBOL_LEGACY_SER_5   (uint64_t(3) | (uint64_t('6') << 8) | (uint64_t('.') << 16) | (uint64_t('0') << 24) | (uint64_t('0') << 32) | (uint64_t('0') << 40))

namespace dpn { namespace protocol {

class legacy_dpn_asset_symbol_type
{
   public:
      legacy_dpn_asset_symbol_type() {}

      bool is_canon()const
      {   return ( ser == DPN_SYMBOL_SER );    }

      uint64_t ser = DPN_SYMBOL_SER;
};

struct legacy_dpn_asset
{
   public:
      legacy_dpn_asset() {}

      template< bool force_canon >
      asset to_asset()const
      {
         if( force_canon )
         {
            FC_ASSERT( symbol.is_canon(), "Must use canonical DPN symbol serialization" );
         }
         return asset( amount, DPN_SYMBOL );
      }

      static legacy_dpn_asset from_amount( share_type amount )
      {
         legacy_dpn_asset leg;
         leg.amount = amount;
         return leg;
      }

      static legacy_dpn_asset from_asset( const asset& a )
      {
         FC_ASSERT( a.symbol == DPN_SYMBOL );
         return from_amount( a.amount );
      }

      share_type                       amount;
      legacy_dpn_asset_symbol_type   symbol;
};

} }

namespace fc { namespace raw {

template< typename Stream >
inline void pack( Stream& s, const dpn::protocol::legacy_dpn_asset_symbol_type& sym )
{
   switch( sym.ser )
   {
      case DPN_SYMBOL_LEGACY_SER_1:
      case DPN_SYMBOL_LEGACY_SER_2:
      case DPN_SYMBOL_LEGACY_SER_3:
      case DPN_SYMBOL_LEGACY_SER_4:
      case DPN_SYMBOL_LEGACY_SER_5:
         wlog( "pack legacy serialization ${s}", ("s", sym.ser) );
      case DPN_SYMBOL_SER:
         pack( s, sym.ser );
         break;
      default:
         FC_ASSERT( false, "Cannot serialize legacy symbol ${s}", ("s", sym.ser) );
   }
}

template< typename Stream >
inline void unpack( Stream& s, dpn::protocol::legacy_dpn_asset_symbol_type& sym, uint32_t depth )
{
   //  994240:        "account_creation_fee": "0.1 DPN"
   // 1021529:        "account_creation_fee": "10.0 DPN"
   // 3143833:        "account_creation_fee": "3.00000 DPN"
   // 3208405:        "account_creation_fee": "2.00000 DPN"
   // 3695672:        "account_creation_fee": "3.00 DPN"
   // 4338089:        "account_creation_fee": "0.001 0.001"
   // 4626205:        "account_creation_fee": "6.000 6.000"
   // 4632595:        "account_creation_fee": "6.000 6.000"
   depth++;
   uint64_t ser = 0;

   fc::raw::unpack( s, ser, depth );
   switch( ser )
   {
      case DPN_SYMBOL_LEGACY_SER_1:
      case DPN_SYMBOL_LEGACY_SER_2:
      case DPN_SYMBOL_LEGACY_SER_3:
      case DPN_SYMBOL_LEGACY_SER_4:
      case DPN_SYMBOL_LEGACY_SER_5:
         wlog( "unpack legacy serialization ${s}", ("s", ser) );
      case DPN_SYMBOL_SER:
         sym.ser = ser;
         break;
      default:
         FC_ASSERT( false, "Cannot deserialize legacy symbol ${s}", ("s", ser) );
   }
}

} // fc::raw

inline void to_variant( const dpn::protocol::legacy_dpn_asset& leg, fc::variant& v )
{
   to_variant( leg.to_asset<false>(), v );
}

inline void from_variant( const fc::variant& v, dpn::protocol::legacy_dpn_asset& leg )
{
   dpn::protocol::asset a;
   from_variant( v, a );
   leg = dpn::protocol::legacy_dpn_asset::from_asset( a );
}

template<>
struct get_typename< dpn::protocol::legacy_dpn_asset_symbol_type >
{
   static const char* name()
   {
      return "dpn::protocol::legacy_dpn_asset_symbol_type";
   }
};

} // fc

FC_REFLECT( dpn::protocol::legacy_dpn_asset,
   (amount)
   (symbol)
   )
