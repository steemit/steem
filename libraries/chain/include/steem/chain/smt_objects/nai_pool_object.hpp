#pragma once

#include <steem/chain/steem_fwd.hpp>
#include <steem/chain/steem_object_types.hpp>
#include <steem/protocol/asset_symbol.hpp>

#ifdef STEEM_ENABLE_SMT

namespace steem { namespace chain {

   class nai_pool_object : public object< nai_pool_object_type, nai_pool_object >
   {
      STEEM_STD_ALLOCATOR_CONSTRUCTOR( nai_pool_object );

   public:
      template< typename Constructor, typename Allocator >
      nai_pool_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type id;

      uint8_t num_available_nais = 0;
      fc::array< asset_symbol_type, SMT_MAX_NAI_POOL_COUNT > nais;

      std::vector< asset_symbol_type > pool() const
      {
         return std::vector< asset_symbol_type >{ nais.begin(), nais.begin() + num_available_nais };
      }

      bool contains( const asset_symbol_type& a ) const
      {
         const auto end = nais.begin() + num_available_nais;
         return std::find( nais.begin(), end, asset_symbol_type::from_asset_num( a.get_stripped_precision_smt_num() ) ) != end;
      }
   };

   typedef multi_index_container <
      nai_pool_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< nai_pool_object, nai_pool_id_type, &nai_pool_object::id > >
      >,
      allocator< nai_pool_object >
   > nai_pool_index;

} } // namespace steem::chain

FC_REFLECT( steem::chain::nai_pool_object, (id)(num_available_nais)(nais) )

CHAINBASE_SET_INDEX_TYPE( steem::chain::nai_pool_object, steem::chain::nai_pool_index )

#endif
