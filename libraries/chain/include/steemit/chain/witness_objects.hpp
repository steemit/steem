#pragma once

#include <steemit/chain/protocol/authority.hpp>
#include <steemit/chain/protocol/types.hpp>
#include <steemit/chain/protocol/steem_operations.hpp>

#include <graphene/db/generic_index.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace steemit { namespace chain {

   using namespace graphene::db;


   class witness_object : public abstract_object<witness_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_witness_object_type;

         string          owner;
         time_point_sec  created;
         string          url;
         uint32_t        total_missed = 0;
         uint64_t        last_aslot = 0;
         uint64_t        last_confirmed_block_num = 0;

         uint64_t        pow_worker = 0;

         public_key_type signing_key;

         chain_properties props;
         price            sbd_exchange_rate;
         time_point_sec   last_sbd_exchange_update;


         share_type      votes;
         fc::uint128     virtual_last_update;
         fc::uint128     virtual_position;
         fc::uint128     virtual_scheduled_time;

         witness_id_type get_id()const { return id; }
   };


   class witness_vote_object : public abstract_object<witness_vote_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_witness_vote_object_type;

         witness_id_type witness;
         account_id_type account;
   };

   class witness_schedule_object : public graphene::db::abstract_object<witness_schedule_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id = impl_witness_schedule_object_type;

         fc::uint128      current_virtual_time;
         uint32_t         next_shuffle_block_num = 1;
         vector< string > current_shuffled_witnesses;
         chain_properties median_props;
   };


   struct by_vote_name;
   struct by_name;
   struct by_pow;
   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      witness_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_name>, member<witness_object, string, &witness_object::owner> >,
         ordered_non_unique< tag<by_pow>, member<witness_object, uint64_t, &witness_object::pow_worker> >,
         ordered_unique< tag<by_vote_name>,
            composite_key< witness_object,
               member<witness_object, share_type, &witness_object::votes >,
               member<witness_object, string, &witness_object::owner >
            >,
            composite_key_compare< std::greater< share_type >, std::less< string > >
         >
      >
   > witness_multi_index_type;

   struct by_account_witness;
   struct by_witness_account;
   typedef multi_index_container<
      witness_vote_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_account_witness>,
            composite_key< witness_vote_object,
               member<witness_vote_object, account_id_type, &witness_vote_object::account >,
               member<witness_vote_object, witness_id_type, &witness_vote_object::witness >
            >,
            composite_key_compare< std::less< account_id_type >, std::less< witness_id_type > >
         >,
         ordered_unique< tag<by_witness_account>,
            composite_key< witness_vote_object,
               member<witness_vote_object, witness_id_type, &witness_vote_object::witness >,
               member<witness_vote_object, account_id_type, &witness_vote_object::account >
            >,
            composite_key_compare< std::less< witness_id_type >, std::less< account_id_type > >
         >
      > // indexed_by
   > witness_vote_multi_index_type;


   typedef generic_index< witness_object,         witness_multi_index_type>             witness_index;
   typedef generic_index< witness_vote_object,    witness_vote_multi_index_type >       witness_vote_index;
} }

FC_REFLECT_DERIVED( steemit::chain::witness_object, (graphene::db::object),
                    (owner)
                    (created)
                    (url)(votes)(virtual_last_update)(virtual_position)(virtual_scheduled_time)(total_missed)
                    (last_aslot)(last_confirmed_block_num)(pow_worker)(signing_key)
                    (props)
                    (sbd_exchange_rate)(last_sbd_exchange_update)
                  )
FC_REFLECT_DERIVED( steemit::chain::witness_vote_object, (graphene::db::object), (witness)(account) )

FC_REFLECT_DERIVED(
   steemit::chain::witness_schedule_object,
   (graphene::db::object),
   (current_virtual_time)(next_shuffle_block_num)(current_shuffled_witnesses)(median_props)
)
