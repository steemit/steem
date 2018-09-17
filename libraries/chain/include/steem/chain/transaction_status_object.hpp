#pragma once
#include <steem/protocol/transaction.hpp>

#include <steem/chain/buffer_type.hpp>
#include <steem/chain/steem_object_types.hpp>

#include <boost/multi_index/hashed_index.hpp>

namespace steem { namespace chain {

   //using chainbase::t_vector;

   /**
    * The purpose of this object is to track the time of LIB and the first block in the TaPoS window
    */
   class transaction_status_object : public object< transaction_status_object_type, transaction_status_object >
   {
      transaction_status_object() = delete;

      public:
         template< typename Constructor, typename Allocator >
         transaction_status_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         id_type                     id;
         transaction_id_type         trx_id_in_block;
         block_num_id_type           block_num;
   };

   struct by_trx_id;
   struct by_block_num;
   typedef multi_index_container<
      transaction_status_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< transaction_status_object, transaction_status_object_id_type, &transaction_status_object::id > >,
         hashed_unique< tag< by_trx_id >, BOOST_MULTI_INDEX_MEMBER(transaction_status_object, transaction_id_type, trx_id_in_block), std::hash<transaction_id_type> >,
         ordered_non_unique< tag< by_block_num >, member< transaction_status_object, block_num_id_type, &transaction_status_object::block_num > >
      >, allocator< transaction_status_object >
   > transaction_status_index;

} } // steem::chain

FC_REFLECT( steem::chain::transaction_status_object, (id)(trx_id_in_block)(block_num) )
CHAINBASE_SET_INDEX_TYPE( steem::chain::transaction_status_object, steem::chain::transaction_status_index )
