#pragma once
#include <steemit/protocol/transaction.hpp>

#include <steemit/chain/steem_object_types.hpp>

#include <boost/multi_index/hashed_index.hpp>

namespace steemit { namespace chain {

   using steemit::protocol::signed_transaction;

   /**
    * The purpose of this object is to enable the detection of duplicate transactions. When a transaction is included
    * in a block a transaction_object is added. At the end of block processing all transaction_objects that have
    * expired can be removed from the index.
    */
   class transaction_object : public object< transaction_object_type, transaction_object >
   {
      transaction_object() = delete;

      public:
         template< typename Constructor, typename Allocator >
         transaction_object( Constructor&& c, allocator< Allocator > a )
            : packed_trx( a )
         {
            c( *this );
         }

         id_type              id;

         bip::vector< char, allocator< char > > packed_trx;
         transaction_id_type  trx_id;
         time_point_sec       expiration;
   };

   struct by_expiration;
   struct by_trx_id;
   typedef multi_index_container<
      transaction_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< transaction_object, transaction_object_id_type, &transaction_object::id > >,
         hashed_unique< tag< by_trx_id >, BOOST_MULTI_INDEX_MEMBER(transaction_object, transaction_id_type, trx_id), std::hash<transaction_id_type> >,
         ordered_non_unique< tag< by_expiration >, member<transaction_object, time_point_sec, &transaction_object::expiration > >
      >,
      allocator< transaction_object >
   > transaction_index;

} } // steemit::chain

FC_REFLECT( steemit::chain::transaction_object, (id)(packed_trx)(trx_id)(expiration) )
CHAINBASE_SET_INDEX_TYPE( steemit::chain::transaction_object, steemit::chain::transaction_index )
