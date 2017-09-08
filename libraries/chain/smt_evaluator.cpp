
#include <steemit/chain/steem_evaluator.hpp>
#include <steemit/chain/database.hpp>
#include <steemit/chain/steem_objects.hpp>
#include <steemit/chain/smt_objects.hpp>

#include <steemit/chain/util/reward.hpp>

#include <steemit/protocol/smt_operations.hpp>
#ifdef STEEM_ENABLE_SMT
namespace steemit { namespace chain {

void smt_elevate_account_evaluator::do_apply( const smt_elevate_account_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );
   const dynamic_global_property_object& dgpo = _db.get_dynamic_global_properties();

   asset effective_elevation_fee;

   FC_ASSERT( dgpo.smt_creation_fee.symbol == STEEM_SYMBOL || dgpo.smt_creation_fee.symbol == SBD_SYMBOL,
      "Unexpected internal error - wrong symbol ${s} of account elevation fee.", ("s", dgpo.smt_creation_fee.symbol) );
   FC_ASSERT( o.fee.symbol == STEEM_SYMBOL || o.fee.symbol == SBD_SYMBOL,
      "Asset fee must be STEEM or SBD, was ${s}", ("s", o.fee.symbol) );
   if( o.fee.symbol == dgpo.smt_creation_fee.symbol )
   {
      effective_elevation_fee = dgpo.smt_creation_fee;
   }
   else
   {
      const auto& fhistory = _db.get_feed_history();
      FC_ASSERT( !fhistory.current_median_history.is_null(), "Cannot pay the fee using SBD because there is no price feed." );
      if( o.fee.symbol == STEEM_SYMBOL )
      {
         effective_elevation_fee = _db.to_sbd( o.fee );
      }
      else
      {
         effective_elevation_fee = _db.to_steem( o.fee );         
      }
   }

   const account_object& acct = _db.get_account( o.account );

   FC_ASSERT( acct.is_smt == false, "The account has already been elevated." );
   FC_ASSERT( o.fee >= effective_elevation_fee, "Fee of ${of} is too small, must be at least ${ef}", ("of", o.fee)("ef", effective_elevation_fee) );
   FC_ASSERT( _db.get_balance( acct, o.fee.symbol ) >= o.fee, "Account does not have sufficient funds for specified fee of ${of}", ("of", o.fee) );

   const account_object& null_account = _db.get_account( STEEM_NULL_ACCOUNT );

   _db.adjust_balance( acct        , -o.fee );
   _db.adjust_balance( null_account,  o.fee );

   _db.create< smt_token_object >( [&]( smt_token_object& token )
   {
      token.control_account = o.account;
   });
}

void smt_setup_evaluator::do_apply( const smt_setup_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );
}

void smt_cap_reveal_evaluator::do_apply( const smt_cap_reveal_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );
}

void smt_refund_evaluator::do_apply( const smt_refund_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );
}

void smt_setup_inflation_evaluator::do_apply( const smt_setup_inflation_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );
}

void smt_set_setup_parameters_evaluator::do_apply( const smt_set_setup_parameters_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );
}

void smt_set_runtime_parameters_evaluator::do_apply( const smt_set_runtime_parameters_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEM_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEM_SMT_HARDFORK) );
}

} }
#endif