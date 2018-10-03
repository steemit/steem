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
      nai_pool_object( Constructor&& c, allocator< Allocator > a ) : nai_pool( a )
      {
         c( *this );
      }

      id_type id;

      using t_nai_pool = chainbase::t_vector< asset_symbol_type >;
      t_nai_pool nai_pool;

      bool contains( const asset_symbol_type& a ) const
      {
         return std::find( nai_pool.begin(), nai_pool.end(), asset_symbol_type::from_asset_num( a.get_stripped_precision_smt_num() ) ) != nai_pool.end();
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

FC_REFLECT( steem::chain::nai_pool_object, (id)(nai_pool) )

CHAINBASE_SET_INDEX_TYPE( steem::chain::nai_pool_object, steem::chain::nai_pool_index )

#endif
