#pragma once
#include <fc/io/raw.hpp>

#include <steemit/protocol/transaction.hpp>

#include <graphene/db/index.hpp>
#include <graphene/db/generic_index.hpp>
#include <fc/uint128.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

namespace steemit { namespace chain {
   using namespace graphene::db;
   using boost::multi_index_container;
   using namespace boost::multi_index;
   /**
    * The purpose of this object is to enable the detection of duplicate transactions. When a transaction is included
    * in a block a transaction_object is added. At the end of block processing all transaction_objects that have
    * expired can be removed from the index.
    */
   class transaction_object : public object< impl_transaction_object_type, transaction_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         transaction_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         signed_transaction  trx;
         transaction_id_type trx_id;

         time_point_sec get_expiration()const { return trx.expiration; }
   };

   struct by_expiration;
   struct by_trx_id;
   typedef multi_index_container<
      transaction_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< transaction_object, id_type, &transaction_object::id > >,
         hashed_unique< tag<by_trx_id>, BOOST_MULTI_INDEX_MEMBER(transaction_object, transaction_id_type, trx_id), std::hash<transaction_id_type> >,
         ordered_non_unique< tag<by_expiration>, const_mem_fun<transaction_object, time_point_sec, &transaction_object::get_expiration > >
      >,
      bip::allocator< transaction_object, bip::managed_mapped_file::segment_manager >
   > transaction_index;

} }

FC_REFLECT_DERIVED( steemit::chain::transaction_object, (graphene::db::object), (trx)(trx_id) )
SET_INDEX_TYPE( steemit::chain::transaction_object, steemit::chain::transaction_index )
