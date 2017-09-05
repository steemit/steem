
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

   asset effective_creation_fee;

   if( o.fee.symbol == STEEM_SYMBOL )
   {
      FC_ASSERT( false, "Payment of fee using STEEM not yet implemented" );
      // TODO:  Calculate effective_creation_fee in STEEM from price feed
      effective_creation_fee = asset( STEEMIT_MAX_SHARE_SUPPLY, STEEM_SYMBOL );
   }
   else if( o.fee.symbol == SBD_SYMBOL )
   {
      effective_creation_fee = dgpo.smt_creation_fee;
   }
   else
   {
      FC_ASSERT( false, "Asset fee must be STEEM or SBD, was ${s}", ("s", o.fee.symbol) );
   }

   const account_object& acct = _db.get_account( o.account );
   FC_ASSERT( o.fee >= effective_creation_fee, "Fee of ${of} is too small, must be at least ${ef}", ("of", o.fee)("ef", effective_creation_fee) );
   FC_ASSERT( _db.get_balance( acct, o.fee.symbol ) >= o.fee, "Account does not have sufficient funds for specified fee of ${of}", ("of", o.fee) );

   const account_object& null_account = _db.get_account( STEEMIT_NULL_ACCOUNT );

   _db.adjust_balance( acct        , -o.fee );
   _db.adjust_balance( null_account,  o.fee );

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
