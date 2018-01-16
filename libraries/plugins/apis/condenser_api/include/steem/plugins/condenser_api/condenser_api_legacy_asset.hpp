#pragma once

#include <steem/protocol/asset.hpp>

namespace steem { namespace plugins { namespace condenser_api {

using steem::protocol::asset;
using steem::protocol::asset_symbol_type;
using steem::protocol::share_type;

struct legacy_asset
{
   public:
      legacy_asset() {}

      asset to_asset()const
      {
         return asset( amount, symbol );
      }

      static legacy_asset from_asset( const asset& a )
      {
         legacy_asset leg;
         leg.amount = a.amount;
         leg.symbol = a.symbol;
         return leg;
      }

      uint8_t     decimals()const;
      void        set_decimals(uint8_t d);
      std::string symbol_name()const;
      int64_t     precision()const;

      string to_string()const;
      static legacy_asset from_string( const string& from );

      share_type                       amount;
      asset_symbol_type                symbol;
};

} } }

FC_REFLECT( steem::plugins::condenser_api::legacy_asset,
   (amount)
   (symbol)
   )
