#pragma once

#include <steem/chain/steem_object_types.hpp>
#include <steem/protocol/smt_operations.hpp>

#ifdef STEEM_ENABLE_SMT

namespace steem { namespace chain {

/**
 * Class responsible for holding regular (i.e. non-reward) balance of SMT for given account.
 * It has not been unified with reward balance object counterpart, due to different number
 * of fields needed to hold balances (2 for regular, 3 for reward).
 */
class account_regular_balance_object : public object< account_regular_balance_object_type, account_regular_balance_object >
{
   STEEM_STD_ALLOCATOR_CONSTRUCTOR( account_regular_balance_object );

public:   
   template <typename Constructor, typename Allocator>
   account_regular_balance_object(Constructor&& c, allocator< Allocator > a)
   {
      c( *this );
   }

   // id_type is actually oid<account_regular_balance_object>
   id_type             id;
   /// Name of the account, the balance is held for.
   account_name_type   owner;
   asset               liquid;   /// 'balance' for STEEM
   asset               vesting;  /// 'vesting_shares' for VESTS

   /** Set of simple methods that allow unification of
    *  regular and rewards balance manipulation code.
    */
   ///@{
   asset_symbol_type get_liquid_symbol() const
   {
      return liquid.symbol;
   }
   void clear_balance( asset_symbol_type liquid_symbol )
   {
      owner = "";
      liquid = asset( 0, liquid_symbol);
      vesting = asset( 0, liquid_symbol.get_paired_symbol() );
   }
   void add_vesting( const asset& vesting_shares, const asset& vesting_value )
   {
      // There's no need to store vesting value (in liquid SMT variant) in regular balance.
      vesting += vesting_shares;
   }
   ///@}

   bool validate() const
   { return liquid.symbol == vesting.symbol.get_paired_symbol(); }
};

/**
 * Class responsible for holding reward balance of SMT for given account.
 * It has not been unified with regular balance object counterpart, due to different number
 * of fields needed to hold balances (2 for regular, 3 for reward).
 */
class account_rewards_balance_object : public object< account_rewards_balance_object_type, account_rewards_balance_object >
{
   STEEM_STD_ALLOCATOR_CONSTRUCTOR( account_rewards_balance_object );

public:   
   template <typename Constructor, typename Allocator>
   account_rewards_balance_object(Constructor&& c, allocator< Allocator > a)
   {
      c( *this );
   }

   // id_type is actually oid<account_rewards_balance_object>
   id_type             id;
   /// Name of the account, the balance is held for.
   account_name_type   owner;
   asset               pending_liquid;          /// 'reward_steem_balance' for pending STEEM
   asset               pending_vesting_shares;  /// 'reward_vesting_balance' for pending VESTS
   asset               pending_vesting_value;   /// 'reward_vesting_steem' for pending VESTS

   /** Set of simple methods that allow unification of
    *  regular and rewards balance manipulation code.
    */
   ///@{
   asset_symbol_type get_liquid_symbol() const
   {
      return pending_liquid.symbol;
   }
   void clear_balance( asset_symbol_type liquid_symbol )
   {
      owner = "";
      pending_liquid = asset( 0, liquid_symbol);
      pending_vesting_shares = asset( 0, liquid_symbol.get_paired_symbol() );
      pending_vesting_value = asset( 0, liquid_symbol);
   }
   void add_vesting( const asset& vesting_shares, const asset& vesting_value )
   {
      pending_vesting_shares += vesting_shares;
      pending_vesting_value += vesting_value;
   }
   ///@}

   bool validate() const
   {
      return pending_liquid.symbol == pending_vesting_shares.symbol.get_paired_symbol() &&
             pending_liquid.symbol == pending_vesting_value.symbol;
   }
};

struct by_owner_liquid_symbol;

typedef multi_index_container <
   account_regular_balance_object,
   indexed_by <
      ordered_unique< tag< by_id >,
         member< account_regular_balance_object, account_regular_balance_id_type, &account_regular_balance_object::id>
      >,
      ordered_unique<tag<by_owner_liquid_symbol>,
         composite_key<account_regular_balance_object,
            member< account_regular_balance_object, account_name_type, &account_regular_balance_object::owner >,
            const_mem_fun< account_regular_balance_object, asset_symbol_type, &account_regular_balance_object::get_liquid_symbol >
         >
      >
   >,
   allocator< account_regular_balance_object >
> account_regular_balance_index;

typedef multi_index_container <
   account_rewards_balance_object,
   indexed_by <
      ordered_unique< tag< by_id >,
         member< account_rewards_balance_object, account_rewards_balance_id_type, &account_rewards_balance_object::id>
      >,
      ordered_unique<tag<by_owner_liquid_symbol>,
         composite_key<account_rewards_balance_object,
            member< account_rewards_balance_object, account_name_type, &account_rewards_balance_object::owner >,
            const_mem_fun< account_rewards_balance_object, asset_symbol_type, &account_rewards_balance_object::get_liquid_symbol >
         >
      >
   >,
   allocator< account_rewards_balance_object >
> account_rewards_balance_index;

} } // namespace steem::chain

FC_REFLECT( steem::chain::account_regular_balance_object,
   (id)
   (owner)
   (liquid)
   (vesting)
)

FC_REFLECT( steem::chain::account_rewards_balance_object,
   (id)
   (owner)
   (pending_liquid)
   (pending_vesting_shares)
   (pending_vesting_value)
)

CHAINBASE_SET_INDEX_TYPE( steem::chain::account_regular_balance_object, steem::chain::account_regular_balance_index )
CHAINBASE_SET_INDEX_TYPE( steem::chain::account_rewards_balance_object, steem::chain::account_rewards_balance_index )

#endif
