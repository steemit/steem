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
      o.phase = smt_phase::ico;
   } );

   smt_ico_evaluation_action eval_action;
   eval_action.control_account = token.control_account;
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
         o.phase = smt_phase::ico_completed;
      } );

      smt_token_launch_action launch_action;
      launch_action.control_account = token.control_account;
      launch_action.symbol = token.liquid_symbol;
      _db.push_required_action( launch_action, ico.launch_time );
   }
   else
   {
      _db.modify( token, []( smt_token_object& o )
      {
         o.phase = smt_phase::launch_failed;
      } );

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

   if ( !util::smt::ico::schedule_next_contributor_payout( _db, token.liquid_symbol ) )
      _db.remove( _db.get< smt_ico_object, by_symbol >( token.liquid_symbol ) );
}

void smt_refund_evaluator::do_apply( const smt_refund_action& a )
{
   using namespace steem::chain::util;

   _db.adjust_balance( a.contributor, a.refund );

   _db.modify( _db.get< smt_ico_object, by_symbol >( a.symbol ), [&]( smt_ico_object& o )
   {
      o.processed_contributions += a.refund.amount;
   } );

   auto key = boost::make_tuple( a.symbol, a.contributor, a.contribution_id );
   _db.remove( _db.get< smt_contribution_object, by_symbol_contributor >( key ) );

   if ( !smt::ico::schedule_next_refund( _db, a.symbol ) )
      _db.remove( _db.get< smt_ico_object, by_symbol >( a.symbol ) );
}

void smt_contributor_payout_evaluator::do_apply( const smt_contributor_payout_action& a )
{
   using namespace steem::chain::util;

   smt::ico::payout( _db, a.symbol, _db.get_account( a.contributor ), a.payouts );

   auto key = boost::make_tuple( a.symbol, a.contributor, a.contribution_id );
   _db.remove( _db.get< smt_contribution_object, by_symbol_contributor >( key ) );

   if ( !smt::ico::schedule_next_contributor_payout( _db, a.symbol ) )
      if ( !smt::ico::schedule_founder_payout( _db, a.symbol ) )
         _db.remove( _db.get< smt_ico_object, by_symbol >( a.symbol ) );
}

void smt_founder_payout_evaluator::do_apply( const smt_founder_payout_action& a )
{
   using namespace steem::chain::util;

   for ( auto& payout : a.payouts )
      smt::ico::payout( _db, a.symbol, _db.get_account( payout.first ), payout.second );

   _db.remove( _db.get< smt_ico_object, by_symbol >( a.symbol ) );
}

#endif

} } //steem::chain
