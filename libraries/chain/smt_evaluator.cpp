
#include <steemit/chain/steem_evaluator.hpp>
#include <steemit/chain/database.hpp>
#include <steemit/chain/steem_objects.hpp>
#include <steemit/chain/smt_objects.hpp>

#include <steemit/chain/util/reward.hpp>

#include <steemit/protocol/smt_operations.hpp>

namespace steemit { namespace chain {

void smt_elevate_account_evaluator::do_apply( const smt_elevate_account_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEMIT_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEMIT_SMT_HARDFORK) );
   const dynamic_global_property_object& dgpo = _db.get_dynamic_global_properties();

   if( o.fee.symbol == STEEM_SYMBOL )
   {
      FC_ASSERT( false, "Payment of fee using STEEM not yet implemented" );
   }
   else if( o.fee.symbol == SBD_SYMBOL )
   {
      FC_ASSERT( o.fee >= dgpo.smt_creation_fee, "Insufficient fee" );
   }
   else
   {
      FC_ASSERT( false, "Unknown symbol" );
   }

   const account_object& acct = _db.get_account( o.account );
   _db.modify( acct, [&]( account_object& a )
   {
      a.is_smt = true;
   });
}

void smt_setup_evaluator::do_apply( const smt_setup_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEMIT_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEMIT_SMT_HARDFORK) );
}

void smt_cap_reveal_evaluator::do_apply( const smt_cap_reveal_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEMIT_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEMIT_SMT_HARDFORK) );
}

void smt_refund_evaluator::do_apply( const smt_refund_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEMIT_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEMIT_SMT_HARDFORK) );
}

void smt_setup_inflation_evaluator::do_apply( const smt_setup_inflation_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEMIT_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEMIT_SMT_HARDFORK) );
}

void smt_set_setup_parameters_evaluator::do_apply( const smt_set_setup_parameters_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEMIT_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEMIT_SMT_HARDFORK) );
}

void smt_set_runtime_parameters_evaluator::do_apply( const smt_set_runtime_parameters_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEMIT_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEMIT_SMT_HARDFORK) );
}

} }
