
#include <steem/plugins/rc/rc_config.hpp>
#include <steem/plugins/rc/rc_operations.hpp>

namespace steem { namespace plugins { namespace rc {

void delegate_to_pool_operation::validate()const
{
   validate_account_name( from_account );
   validate_account_name( to_pool );

   FC_ASSERT( amount.symbol.is_vesting(), "Must use vesting symbol" );
   FC_ASSERT( amount.symbol == VESTS_SYMBOL, "Currently can only delegate VESTS (SMT's not supported #2698)" );
   FC_ASSERT( amount.amount.value >= 0, "Delegation to pool cannot be negative" );
}

void delegate_drc_from_pool_operation::validate()const
{
   validate_account_name( from_pool );
   validate_account_name( to_account );

   FC_ASSERT( to_slot >= 0 );
   FC_ASSERT( to_slot < STEEM_RC_MAX_OUTDEL_SLOTS );
   FC_ASSERT( amount.symbol.is_vesting(), "Must use vesting symbol" );
   FC_ASSERT( asset_symbol == VESTS_SYMBOL, "Currently can only delegate VESTS (SMT's not supported #2698)" );
   FC_ASSERT( drc_max_mana >= 0 );
}

void set_slot_delegator_operation::validate()const
{
   validate_account_name( from_pool );
   validate_account_name( to_account );
   validate_account_name( signer );

   FC_ASSERT( to_slot >= 0 );
   FC_ASSERT( to_slot < STEEM_RC_MAX_OUTDEL_SLOTS );
}

void delegate_to_pool_evaluator::do_apply( const delegate_to_pool_operation& op )
{
   database& d = db();
   const dynamic_global_property_object& gpo = d.get_dynamic_global_properties();
   uint32_t now = gpo.time.sec_since_epoch();
   const rc_account_object& from_rc_account = d.get< rc_account_object, by_name >( op.from_account );

   // Should be enforced by calling regenerate() in pre-op vtor
   FC_ASSERT( from_rc_account.rc_manabar.last_update_time == now );

   const rc_delegation_pool_object* to_pool = d.find< rc_delegation_pool_object, by_account_symbol >( op.to_pool, op.amount.symbol );
   steem::util::manabar rc_pool_manabar;
   steem::util::manabar_params rc_pool_manabar_params;
   rc_pool_manabar_params.regen_time = STEEM_RC_REGEN_TIME;
   if( !to_pool )
   {
      const account_object* to_pool_account = d.find< account_object, by_name >( op.to_pool );
      FC_ASSERT( to_pool_account, "Account ${a} does not exist", ("a", op.to_pool) );
      rc_pool_manabar.current_mana = 0;
      rc_pool_manabar.last_update_time = now;
      rc_pool_manabar_params.max_mana = 0;
   }
   else
   {
      rc_pool_manabar = to_pool->rc_pool_manabar;
      rc_pool_manabar_params.max_mana = to_pool->max_rc;
   }
   rc_pool_manabar.regenerate_mana( rc_pool_manabar_params, now );

   const rc_indel_edge_object* edge = d.find< rc_indel_edge_object, by_edge >( op.from_account, op.to_pool, op.amount.symbol );

   int64_t old_max_rc = edge ? edge->amount.value : 0;
   int64_t new_max_rc = op.amount.value;
   int64_t delta_max_rc = new_max_rc - old_max_rc;
   int64_t old_rc = rc_pool_manabar.current_mana;
   int64_t from_account_rc = from_rc_account.rc_manabar.current_mana;
   int64_t new_rc = std::min( new_max_rc, old_rc + std::max( std::min( delta_max_rc, from_account_rc ), 0 ) );
   int64_t delta_rc = new_rc - old_rc;

   steem::util::manabar rc_account_manabar = from_rc_account.rc_manabar;
   rc_account_manabar.current_mana -= delta_rc;
   rc_pool_manabar.current_mana += delta_rc;

   if( op.amount.symbol == VESTS_SYMBOL )
   {
      const account_object& from_account = d.get< account_object, by_name >( op.from_account );
      int64_t account_max_rc = get_maximum_rc( from_account, from_rc_account );
      FC_ASSERT( account_max_rc >= delta_max_rc, "Account ${a} has insufficient VESTS (have ${h}, need ${n})",
         ("a", op.from_account)("h", account_max_rc)("n", delta_max_rc) );
   }
   else
   {
      FC_ASSERT( false, "SMT delegation is not supported" );
      // TODO:  When fixing this FC_ASSERT() for SMT support, also be sure to implement
      // rc_delegation_from_account_object to enforce total SMT delegations from account <= vesting of SMT in account,
      // and also enforce this condition (by forcible de-delegation to the pool) on SMT powerdown.
   }

   if( !to_pool )
   {
      db.create< rc_delegation_pool_object >( [&]( rc_delegation_pool_object& pool )
      {
         pool.account = op.to_pool;
         pool.asset_symbol = op.amount.symbol;
         pool.rc_pool_manabar = rc_pool_manabar;
         pool.max_rc = delta_max_rc;
      } );
   }
   else
   {
      db.modify< rc_delegation_pool_object >( *to_pool, [&]( rc_delegation_pool_object& pool )
      {
         pool.rc_pool_manabar = rc_pool_manabar;
         pool.max_rc += delta_max_rc;
      } );
   }

   db.modify< rc_account_object >( from_rc_account, [&]( rc_account_object& rca )
   {
      rca.rc_manabar = rc_account_manabar;
      if( op.amount.symbol == VESTS_SYMBOL )
         rca.vests_delegated_to_pools += asset( delta_max_rc, VESTS_SYMBOL );
   } );
}

void delegate_drc_from_pool_evaluator::do_apply( const delegate_drc_from_pool_operation& op )
{
   FC_ASSERT( false, "Operation not yet supported" );
}

void set_slot_delegator_operation::do_apply( const set_slot_delegator_operation& op )
{
   FC_ASSERT( false, "Operation not yet supported" );
}

} } } // steem::plugins::rc
