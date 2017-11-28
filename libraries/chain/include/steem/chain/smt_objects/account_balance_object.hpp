#pragma once

#include <steem/chain/steem_object_types.hpp>
#include <steem/protocol/smt_operations.hpp>

#ifdef STEEM_ENABLE_SMT

namespace steem { namespace chain {

template <enum account_balance_target BalanceTarget, enum object_type ObjectType>
class account_balance_object : public object< ObjectType, account_balance_object<BalanceTarget, ObjectType> >
{
   account_balance_object() = delete;

public:
   typedef account_balance_object<BalanceTarget, ObjectType>   _this_type;

   template <class Constructor, class Allocator>
   account_balance_object(Constructor&&, allocator< Allocator >)
   {
      c( *this );
   }

   // id_type is actually oid<account_balance_object<BalanceTarget, ObjectType> >
   typename object< ObjectType, _this_type >::id_type id;
   account_name_type                                  owner = "@@@@@";
   asset                                              balance;

   account_balance_target balanceTarget() const
   {
      return BalanceTarget;
   }

   asset_symbol_type get_symbol() const
   {
      return balance.symbol;
   }
};

struct by_owned_symbol;

typedef multi_index_container <
   account_regular_balance_object,
   indexed_by <
      ordered_unique< tag< by_id >,
         member< account_regular_balance_object, account_regular_balance_id_type, &account_regular_balance_object::id > >,
      ordered_unique< tag< by_owned_symbol >,
         composite_key< account_regular_balance_object,
            member< account_regular_balance_object, account_name_type, &account_regular_balance_object::owner >,
            const_mem_fun< account_regular_balance_object, asset_symbol_type, &account_regular_balance_object::get_symbol >
         >,
         composite_key_compare< std::less< account_name_type >, std::less< asset_symbol_type > >
      >
   >,
   allocator< account_regular_balance_object >
> account_regular_balance_index;

typedef multi_index_container <
   account_savings_balance_object,
   indexed_by <
      ordered_unique< tag< by_id >,
         member< account_savings_balance_object, account_savings_balance_id_type, &account_savings_balance_object::id > >,
      ordered_unique< tag< by_owned_symbol >,
         composite_key< account_savings_balance_object,
            member< account_savings_balance_object, account_name_type, &account_savings_balance_object::owner >,
            const_mem_fun< account_savings_balance_object, asset_symbol_type, &account_savings_balance_object::get_symbol >
         >,
         composite_key_compare< std::less< account_name_type >, std::less< asset_symbol_type > >
      >
   >,
   allocator< account_savings_balance_object >
> account_savings_balance_index;

} } // namespace steem::chain

FC_REFLECT( steem::chain::account_regular_balance_object,
   (id)
   (owner)
   (balance)
)

FC_REFLECT( steem::chain::account_savings_balance_object,
   (id)
   (owner)
   (balance)
)

CHAINBASE_SET_INDEX_TYPE( steem::chain::account_regular_balance_object, steem::chain::account_regular_balance_index )
CHAINBASE_SET_INDEX_TYPE( steem::chain::account_savings_balance_object, steem::chain::account_savings_balance_index )

#endif
