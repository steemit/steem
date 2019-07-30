#include <steem/chain/steem_fwd.hpp>
#include <steem/chain/required_action_evaluator.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/smt_objects.hpp>
#include <steem/chain/util/smt_token.hpp>
#include <cmath>

namespace steem { namespace chain {

#ifdef IS_TEST_NET

void example_required_evaluator::do_apply( const example_required_action& a ) {}

#endif

#ifdef STEEM_ENABLE_SMT
void smt_ico_launch_evaluator::do_apply( const smt_ico_launch_action& a )
{
   const smt_token_object& token = _db.get< smt_token_object, by_symbol >( a.symbol );
   const smt_ico_object& ico = _db.get< smt_ico_object, by_symbol >( token.liquid_symbol );

   _db.modify( token, []( smt_token_object& o )
   {
      o.phase = smt_phase::contribution_begin_time_completed;
   } );

   smt_ico_evaluation_action eval_action;
   eval_action.symbol = token.liquid_symbol;
   _db.push_required_action( eval_action, ico.contribution_end_time );
}

void smt_ico_evaluation_evaluator::do_apply( const smt_ico_evaluation_action& a )
{
   const smt_token_object& token = _db.get< smt_token_object, by_symbol >( a.symbol );
   const smt_ico_object& ico = _db.get< smt_ico_object, by_symbol >( token.liquid_symbol );

   if ( ico.contributed.amount >= ico.steem_units_min )
   {
      _db.modify( token, []( smt_token_object& o )
      {
         o.phase = smt_phase::contribution_end_time_completed;
      } );

      smt_token_launch_action launch_action;
      launch_action.symbol = token.liquid_symbol;
      _db.push_required_action( launch_action, ico.launch_time );
   }
   else
   {
      _db.modify( token, []( smt_token_object& o )
      {
         o.phase = smt_phase::launch_failed;
      } );

      _db.remove( ico );

      util::smt::ico::schedule_next_refund( _db, token.liquid_symbol );
   }
}

void smt_token_launch_evaluator::do_apply( const smt_token_launch_action& a )
{
   const smt_token_object& token = _db.get< smt_token_object, by_symbol >( a.symbol );

   _db.modify( token, []( smt_token_object& o )
   {
      o.phase = smt_phase::launch_success;
   } );

   /*
    * If there are no contributions to schedule payouts for we no longer require
    * the ICO object.
    */
   if ( !util::smt::ico::schedule_next_contributor_payout( _db, token.liquid_symbol ) )
   {
      _db.remove( _db.get< smt_ico_object, by_symbol >( token.liquid_symbol ) );
   }
}

void smt_refund_evaluator::do_apply( const smt_refund_action& a )
{
   using namespace steem::chain::util;

   auto& idx = _db.get_index< smt_contribution_index, by_symbol_contributor >();
   auto itr = idx.find( boost::make_tuple( a.symbol, a.contributor, a.contribution_id ) );
   FC_ASSERT( itr != idx.end(), "Unable to find contribution object for the provided action: ${a}", ("a", a) );

   _db.adjust_balance( a.contributor, a.refund );

   _db.remove( *itr );

   if ( !_db.is_pending_tx() )
      smt::ico::schedule_next_refund( _db, a.symbol );
}

void smt_contributor_payout_evaluator::do_apply( const smt_contributor_payout_action& a )
{
   using namespace steem::chain::util;

   const auto& account = _db.get_account( a.contributor );

   for ( auto& payout : a.payouts )
   {
      if ( payout.symbol.is_vesting() )
      {
         _db.create_vesting( account, asset( payout.amount, payout.symbol.get_paired_symbol() ) );
      }
      else
      {
         _db.adjust_balance( account, payout );

         if ( payout.symbol.space() == asset_symbol_type::smt_nai_space )
            _db.adjust_supply( payout );
      }
   }

   if ( !_db.is_pending_tx() )
      if ( !smt::ico::schedule_next_contributor_payout( _db, a.symbol ) )
         smt::ico::schedule_founder_payout( _db, a.symbol );
}

void smt_founder_payout_evaluator::do_apply( const smt_founder_payout_action& a )
{
   using namespace steem::chain::util;

   const auto& account = _db.get_account( a.founder );

   for ( auto& payout : a.payouts )
   {
      if ( payout.symbol.is_vesting() )
      {
         _db.create_vesting( account, asset( payout.amount, payout.symbol.get_paired_symbol() ) );
      }
      else
      {
         _db.adjust_balance( account, payout );

         if ( payout.symbol.space() == asset_symbol_type::smt_nai_space )
            _db.adjust_supply( payout );
      }
   }
}

#endif

} } //steem::chain
