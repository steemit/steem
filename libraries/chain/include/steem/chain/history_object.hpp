#pragma once

#include <steem/protocol/types.hpp>
#include <steem/chain/steem_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace steem { namespace chain {

   /** State-model representation of some blockchain operation.
    *  Holds all informations needed to restore given operation
    *  One operation_object can be shared against historical data for multiple accounts,
    *  which were involved into given operation.
    */
   class operation_object : public object< operation_object_type, operation_object >
   {
      operation_object() = delete;

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
         /// Seems that this member is never used
         //uint16_t             op_in_trx = 0;
         /// Seems that this member is never modified
         //uint64_t           virtual_op = 0;
         uint64_t             virtual_op() const { return 0;}
         time_point_sec       timestamp;
         buffer_type          serialized_op;
   };

   struct by_location;
   struct by_transaction_id;
   typedef multi_index_container<
      operation_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< operation_object, operation_id_type, &operation_object::id > >,
         ordered_non_unique< tag< by_location >, member< operation_object, uint32_t, &operation_object::block> >
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

   /** Stores historical operation data associated to given account.
    * 
    */
   class account_history_object : public object< account_history_object_type, account_history_object >
   {
   public:
      typedef chainbase::t_vector<operation_id_type> operation_container;

      template< typename Constructor, typename Allocator >
      account_history_object( Constructor&& c, allocator< Allocator > a ) :
         associated_ops(a)
      {
         c( *this );
      }

      id_type           id;
      account_id_type   account;

      const operation_container& get_ops() const
      {
         return associated_ops;
      }

      void store_operation(const operation_object& op)
      {
         associated_ops.push_back(op.id);
      }

      void truncate_operation_list(size_t maxLength)
      {
         auto opsCount = associated_ops.size();
         if(opsCount > maxLength)
         {
            auto toErase = associated_ops.size() - maxLength;
            associated_ops.erase(associated_ops.begin(), associated_ops.begin() + toErase);
         }
      }

      /** Allows to remove all operations being accepted by filter.
       *   Dedicated for use in outdated ops pruning (older than 30 days).
       *   \param filter - functor object having interface
       *       bool operator(const operation_id_type&) const;
       *       Shall return true if operation is outdated and shall be removed.
       */
      template <class Filter>
      void remove_outdated_operations(Filter&& filter)
      {
         auto i = associated_ops.begin();
         while(i != associated_ops.end())
         {
            const operation_id_type opId = *i;
            ++i;

            if(filter(opId) == false)
            {
               i = associated_ops.erase(associated_ops.begin(), i);
               /** if filter don't accept any item, the otherss placed after also shall
                *  be not accepted, since container shall be sorted.
                */
               assert(std::find_if(i, associated_ops.end(), filter) == associated_ops.end());
               break;
            }
         }
      }

   private:
      /** Holds IDs of operations somehow involving given accout.
       *  List head points to oldest operation, tail to newest one.
       *  Container order is determined by operation processing.
       */
      operation_container associated_ops;
      template <class T> friend struct fc::reflector;
   };

   struct by_account;
   typedef multi_index_container<
      account_history_object,
      indexed_by<
         ordered_unique< tag< by_id >,
            member< account_history_object, account_history_id_type, &account_history_object::id >
         >,
         ordered_unique< tag< by_account >,
            member< account_history_object, account_id_type, &account_history_object::account>
         >
      >,
      allocator< account_history_object >
   > account_history_index;
} }

FC_REFLECT( steem::chain::operation_object, (id)(trx_id)(block)(trx_in_block)(timestamp)(serialized_op) )
CHAINBASE_SET_INDEX_TYPE( steem::chain::operation_object, steem::chain::operation_index )

FC_REFLECT( steem::chain::account_history_object, (id)(account)(associated_ops) )
CHAINBASE_SET_INDEX_TYPE( steem::chain::account_history_object, steem::chain::account_history_index )

namespace helpers
{
   template <>
   class index_statistic_provider<steem::chain::operation_index>
   {
   public:
      typedef steem::chain::operation_index IndexType;

      index_statistic_info gather_statistics(const IndexType& index, bool onlyStaticInfo) const
      {
         index_statistic_info info;
         gather_index_static_data(index, &info);
         
         if(onlyStaticInfo == false)
         {
            for(const auto& o : index)
               info._item_additional_allocation +=
                  o.serialized_op.capacity()*sizeof(steem::chain::buffer_type::value_type);
         }

         return info;
      }
   };

   template <>
   class index_statistic_provider<steem::chain::account_history_index>
   {
   public:
      typedef steem::chain::account_history_index IndexType;

      index_statistic_info gather_statistics(const IndexType& index, bool onlyStaticInfo) const
      {
         index_statistic_info info;
         gather_index_static_data(index, &info);

         if(onlyStaticInfo == false)
         {
            for(const auto& o : index)
               info._item_additional_allocation += o.get_ops().capacity()*
                  sizeof(steem::chain::account_history_object::operation_container::value_type);
         }

         return info;
      }
   };

} /// namespace helpers

