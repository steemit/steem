#pragma once

#include <steem/chain/steem_object_types.hpp>
#include <steem/protocol/smt_operations.hpp>

#include <serialize3/h/client_code/serialize_macros.h>

#ifdef STEEM_ENABLE_SMT

namespace steem { namespace chain {

/**
 * Class responsible for holding non STEEM balance (SMT one) for given account.
 * Since multiple kinds of balances are possible (atm regular and savings) this
 * class is parameterized to distinguish their types. 
 */
template <enum object_type ObjectType>
class account_balance_object : public object< ObjectType, account_balance_object<ObjectType> >
{
   SERIALIZABLE_OBJECT;
   //account_balance_object() = delete;

public:   
   typedef account_balance_object<ObjectType>   _this_type;
   using typename object< ObjectType, _this_type >::id_type;

   account_balance_object() = default;
   template <class Constructor, class Allocator>
   account_balance_object(Constructor&& c, allocator< Allocator > a)
   {
      c( *this );
   }

   // id_type is actually oid<account_balance_object<ObjectType> >
   id_type             id;
   /// Name of the account, the balance is held for.
   account_name_type   owner;
   asset               balance;

   /// Returns true if this object holds a savings balance, regular one otherwise.
   bool is_rewards_balance() const
   {
      return ObjectType == account_rewards_balance_object_type;
   }

   /// Returns symbol of the asset the balance is held for.
   asset_symbol_type get_symbol() const
   {
      return balance.symbol;
   }
};

struct by_owner_symbol;

template <enum object_type ObjectType>
using account_balance_index = chainbase::shared_multi_index_container <account_balance_object<ObjectType>,
   indexed_by <
      ordered_unique< tag< by_id >,
         member< account_balance_object<ObjectType>, typename account_balance_object<ObjectType>::id_type, &account_balance_object<ObjectType>::id>
      >,
      ordered_unique<tag<by_owner_symbol>,
         composite_key<account_balance_object<ObjectType>,
            member< account_balance_object<ObjectType>, account_name_type, &account_balance_object<ObjectType>::owner >,
            const_mem_fun< account_balance_object<ObjectType>, asset_symbol_type, &account_balance_object<ObjectType>::get_symbol >
         >
      >
   >
>;

typedef account_balance_index<account_regular_balance_object_type> account_regular_balance_index;
typedef account_balance_index<account_rewards_balance_object_type> account_rewards_balance_index;


} } // namespace steem::chain

FC_REFLECT( steem::chain::account_regular_balance_object,
   (id)
   (owner)
   (balance)
)

FC_REFLECT( steem::chain::account_rewards_balance_object,
   (id)
   (owner)
   (balance)
)

CHAINBASE_SET_INDEX_TYPE( steem::chain::account_regular_balance_object, steem::chain::account_regular_balance_index )
CHAINBASE_SET_INDEX_TYPE( steem::chain::account_rewards_balance_object, steem::chain::account_rewards_balance_index )

#endif
