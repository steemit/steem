#pragma once
#include <steemit/protocol/authority.hpp>
#include <steemit/protocol/steem_operations.hpp>

#include <steemit/chain/steem_object_types.hpp>
#include <steemit/chain/witness_objects.hpp>
#include <steemit/chain/shared_authority.hpp>
#include <boost/multi_index/composite_key.hpp>


namespace steemit { namespace chain {

   class blind_balance_object : public chainbase::object< blind_balance_object_type, blind_balance_object >
   {
      public:
         /**
          *  Outputs in state 'checking' can be spent immediately
          *  Outputs in state 'savings' can be spent into a 3 day verification object
          *  Outputs in state 'pending' cannot be used in new transactions (except to cancel)
          *  Outputs in state 'pending' will be removed after time interval of (TODO: how long?)
          */
         enum states {
            checking = 0,
            savings  = 1,
            pending  = 2
         };
         template<typename Constructor, typename Allocator>
         blind_balance_object( Constructor&& c, allocator< Allocator > a )
         :confirmation(a)
         {
            c(*this);
         };

         id_type                    id;
         asset_symbol_type          symbol;
         account_name_type          owner;
         fc::ecc::commitment_type   commitment;
         buffer_type                confirmation; ///< in full nodes stores encrypted info for owner
         uint8_t                    state = 0;
   };

   struct by_commitment;
   struct by_id;
   struct by_owner_symbol;
   typedef shared_multi_index_container<
      blind_balance_object,
      indexed_by<
         ordered_unique< tag< by_id >, 
            member< blind_balance_object, blind_balance_object::id_type, &blind_balance_object::id > 
         >,
         ordered_unique< tag< by_commitment >, 
            member< blind_balance_object, fc::ecc::commitment_type, &blind_balance_object::commitment > 
         >,
         ordered_unique< tag< by_owner_symbol >, 
            composite_key< blind_balance_object,
               member< blind_balance_object, account_name_type, &blind_balance_object::owner >,
               member< blind_balance_object, asset_symbol_type, &blind_balance_object::symbol > 
            >
         >
      >
   > blind_balance_index;

   class pending_blind_transfer_object : public chainbase::object< pending_blind_transfer_object_type, pending_blind_transfer_object > {
      public:
         template<typename Constructor, typename Allocator>
         pending_blind_transfer_object( Constructor&& c, allocator< Allocator > a )
         :pending_commitments(a)
         {
            c(*this);
         };

         id_type                                   id;
         account_name_type                         owner;
         time_point_sec                            expiration;
         account_name_type                         to;
         asset                                     to_public_amount;
         shared_vector< fc::ecc::commitment_type > pending_commitments;

         const fc::ecc::commitment_type& transfer_id()const { return pending_commitments.front(); }
   };

   struct by_expiration;
   struct by_owner;
   typedef shared_multi_index_container<
      pending_blind_transfer_object,
      indexed_by<
         ordered_unique< tag< by_id >, 
            member< pending_blind_transfer_object, pending_blind_transfer_object::id_type, &pending_blind_transfer_object::id > 
         >,
         ordered_unique< tag< by_owner >, 
            member< pending_blind_transfer_object, account_name_type, &pending_blind_transfer_object::owner > 
         >,
         ordered_unique< tag< by_expiration >, 
            member< pending_blind_transfer_object, fc::time_point_sec, &pending_blind_transfer_object::expiration > 
         >,
         ordered_unique< tag< by_commitment >, 
            const_mem_fun< pending_blind_transfer_object, const fc::ecc::commitment_type&, &pending_blind_transfer_object::transfer_id > 
         >
      >
   > pending_blind_transfer_index;


} } // namespace steemit::chain

FC_REFLECT( steemit::chain::blind_balance_object, (id)(symbol)(owner)(commitment)(confirmation) )
CHAINBASE_SET_INDEX_TYPE( steemit::chain::blind_balance_object, steemit::chain::blind_balance_index )
FC_REFLECT( steemit::chain::pending_blind_transfer_object, (id)(owner)(expiration)(to)(to_public_amount)(pending_commitments) )
CHAINBASE_SET_INDEX_TYPE( steemit::chain::pending_blind_transfer_object, steemit::chain::pending_blind_transfer_index )
