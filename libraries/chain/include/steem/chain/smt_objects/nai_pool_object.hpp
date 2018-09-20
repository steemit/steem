#pragma once

#include <steem/chain/steem_object_types.hpp>
#include <steem/protocol/asset_symbol.hpp>

#ifdef STEEM_ENABLE_SMT

namespace steem { namespace chain {

   class nai_pool_object : public object< nai_pool_object_type, nai_pool_object >
   {
      nai_pool_object() = delete;

   public:
      template< typename Constructor, typename Allocator >
      nai_pool_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type              id;
      asset_symbol_type    asset_symbol;
   };
   
   struct by_symbol;

   typedef multi_index_container <
      nai_pool_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< nai_pool_object, nai_pool_id_type, &nai_pool_object::id > >,
         ordered_unique< tag< by_symbol >, member< nai_pool_object, asset_symbol_type, &nai_pool_object::asset_symbol > >
      >,
      allocator< nai_pool_object >
   > nai_pool_index;

} } // namespace steem::chain

FC_REFLECT( steem::chain::nai_pool_object, (id)(asset_symbol) )

CHAINBASE_SET_INDEX_TYPE( steem::chain::nai_pool_object, steem::chain::nai_pool_index )

#endif
