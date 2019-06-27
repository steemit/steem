#pragma once
#include <dpn/chain/dpn_fwd.hpp>

#include <dpn/chain/dpn_fwd.hpp>

#include <dpn/protocol/authority.hpp>
#include <dpn/protocol/operations.hpp>
#include <dpn/protocol/dpn_operations.hpp>

#include <dpn/chain/buffer_type.hpp>
#include <dpn/chain/dpn_object_types.hpp>
#include <dpn/chain/witness_objects.hpp>

namespace dpn { namespace chain {

   class operation_object : public object< operation_object_type, operation_object >
   {
      DPN_STD_ALLOCATOR_CONSTRUCTOR( operation_object )

      public:
         template< typename Constructor, typename Allocator >
         operation_object( Constructor&& c, allocator< Allocator > a )
            :serialized_op( a )
         {
            c( *this );
         }

         id_type              id;

         transaction_id_type  trx_id;
         uint32_t             block = 0;
         uint32_t             trx_in_block = 0;
         uint32_t             op_in_trx = 0;
         uint32_t             virtual_op = 0;
         time_point_sec       timestamp;
         buffer_type          serialized_op;

         uint64_t             get_virtual_op() const { return virtual_op; }
   };

   struct by_location;
   struct by_transaction_id;
   typedef multi_index_container<
      operation_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< operation_object, operation_id_type, &operation_object::id > >,
         ordered_unique< tag< by_location >,
            composite_key< operation_object,
               member< operation_object, uint32_t, &operation_object::block >,
               member< operation_object, operation_id_type, &operation_object::id >
            >
         >
#ifndef SKIP_BY_TX_ID
         ,
         ordered_unique< tag< by_transaction_id >,
            composite_key< operation_object,
               member< operation_object, transaction_id_type, &operation_object::trx_id>,
               member< operation_object, operation_id_type, &operation_object::id>
            >
         >
#endif
      >,
      allocator< operation_object >
   > operation_index;

   class account_history_object : public object< account_history_object_type, account_history_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         account_history_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         account_history_object() {}

         id_type           id;

         account_name_type account;
         uint32_t          sequence = 0;
         operation_id_type op;
   };

   struct by_account;
   struct by_account_rev;
   typedef multi_index_container<
      account_history_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< account_history_object, account_history_id_type, &account_history_object::id > >,
         ordered_unique< tag< by_account >,
            composite_key< account_history_object,
               member< account_history_object, account_name_type, &account_history_object::account>,
               member< account_history_object, uint32_t, &account_history_object::sequence>
            >,
            composite_key_compare< std::less< account_name_type >, std::greater< uint32_t > >
         >
      >,
      allocator< account_history_object >
   > account_history_index;
} }

#ifdef ENABLE_MIRA
namespace mira {

template<> struct is_static_length< dpn::chain::account_history_object > : public boost::true_type {};

} // mira
#endif

FC_REFLECT( dpn::chain::operation_object, (id)(trx_id)(block)(trx_in_block)(op_in_trx)(virtual_op)(timestamp)(serialized_op) )
CHAINBASE_SET_INDEX_TYPE( dpn::chain::operation_object, dpn::chain::operation_index )

FC_REFLECT( dpn::chain::account_history_object, (id)(account)(sequence)(op) )

CHAINBASE_SET_INDEX_TYPE( dpn::chain::account_history_object, dpn::chain::account_history_index )

namespace helpers
{
   template <>
   class index_statistic_provider<dpn::chain::operation_index>
   {
   public:
      typedef dpn::chain::operation_index IndexType;

      index_statistic_info gather_statistics(const IndexType& index, bool onlyStaticInfo) const
      {
         index_statistic_info info;
         gather_index_static_data(index, &info);

         if(onlyStaticInfo == false)
         {
            for(const auto& o : index)
               info._item_additional_allocation +=
                  o.serialized_op.capacity()*sizeof(dpn::chain::buffer_type::value_type);
         }

         return info;
      }
   };

   template <>
   class index_statistic_provider<dpn::chain::account_history_index>
   {
   public:
      typedef dpn::chain::account_history_index IndexType;

      index_statistic_info gather_statistics(const IndexType& index, bool onlyStaticInfo) const
      {
         index_statistic_info info;
         gather_index_static_data(index, &info);

         if(onlyStaticInfo == false)
         {
            //for(const auto& o : index)
            //   info._item_additional_allocation += o.get_ops().capacity()*
            //      sizeof(dpn::chain::account_history_object::operation_container::value_type);
         }

         return info;
      }
   };

} /// namespace helpers

