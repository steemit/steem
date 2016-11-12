#pragma once
#include <steemit/protocol/authority.hpp>
#include <steemit/protocol/steem_operations.hpp>

#include <steemit/chain/steem_object_types.hpp>
#include <steemit/chain/witness_objects.hpp>
#include <steemit/chain/shared_authority.hpp>
#include <boost/multi_index/composite_key.hpp>


namespace steemit { namespace chain {

   class blind_balance_object : public chainbase::object< blind_balance_object_type, blind_balance_object > {
      public:
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

} } // namespace steemit::chain

FC_REFLECT( steemit::chain::blind_balance_object, (id)(symbol)(owner)(commitment)(confirmation) )
CHAINBASE_SET_INDEX_TYPE( steemit::chain::blind_balance_object, steemit::chain::blind_balance_index )
