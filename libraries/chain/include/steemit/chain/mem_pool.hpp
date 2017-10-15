#pragma once

#include <steemit/chain/steem_object_types.hpp>

namespace steemit { namespace chain {

   class mem_pool_entry_object : public object< mem_pool_entry_object_type, mem_pool_entry_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         mem_pool_entry_object( Constructor && c, allocator< Allocator > a )
            :packed_trx( a )
         {
            c( *this );
         }

         id_type              id;

         /* Prevents duplicates from appearing in the mem pool and prevents
         * concurrent modification of the mem pool from _push_transaction
         * when applying the pending transaction state */
         transaction_id_type  trx_id;
         buffer_type          packed_trx;
   };

   struct by_id;
   struct by_trx_id;
   typedef multi_index_container<
      mem_pool_entry_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< mem_pool_entry_object, mem_pool_entry_id_type, &mem_pool_entry_object::id > >,
         ordered_unique< tag< by_trx_id >, member< mem_pool_entry_object, transaction_id_type, &mem_pool_entry_object::trx_id > >
      >,
      allocator< mem_pool_entry_object >
   > mem_pool_entry_index;

} } // steemit::chain

FC_REFLECT( steemit::chain::mem_pool_entry_object, (id)(trx_id)(packed_trx) )
CHAINBASE_SET_INDEX_TYPE( steemit::chain::mem_pool_entry_object, steemit::chain::mem_pool_entry_index )
