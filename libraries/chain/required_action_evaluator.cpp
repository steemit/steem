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
void smt_event_evaluator::do_apply( const smt_event_action& a )
{
   using namespace steem::chain::util;

   switch ( a.event )
   {
      case smt_event::ico_launch:
         smt::ico::launch( _db, a.symbol );
         break;
      case smt_event::ico_evaluation:
         smt::ico::evaluate( _db, a.symbol );
         break;
      case smt_event::token_launch:
         smt::ico::launch_token( _db, a.symbol );
         break;
      default:
         FC_ASSERT( false, "Invalid SMT event processing submission" );
         break;
   }
}
void smt_refund_evaluator::do_apply( const smt_refund_action& a )
{
   using namespace steem::chain::util;

   smt::ico::cascading_action_applier( _db, a, []( database& db, const smt_contribution_object& o )
   {
      db.adjust_balance( o.contributor, o.contribution );
   }, smt::ico::schedule_next_refund );
}

void smt_contributor_payout_evaluator::do_apply( const smt_contributor_payout_action& a )
{
   using namespace steem::chain::util;

   smt::ico::cascading_action_applier( _db, a, []( database& db, const smt_contribution_object& o )
   {
      const auto& ico = db.get< smt_ico_object, by_symbol >( o.symbol );

      smt_generation_unit effective_generation_unit;

      if ( ico.processed < ico.steem_units_soft_cap )
         effective_generation_unit = ico.capped_generation_policy.pre_soft_cap_unit;
      else
         effective_generation_unit = ico.capped_generation_policy.post_soft_cap_unit;

      const auto& token_unit = effective_generation_unit.token_unit;
      const auto& steem_unit = effective_generation_unit.steem_unit;

      auto steem_units_sent = o.contribution.amount / effective_generation_unit.steem_unit_sum();
      auto contributed_steem_units = ico.contributed.amount / effective_generation_unit.steem_unit_sum();
      auto steem_units_hard_cap = ico.steem_units_hard_cap / effective_generation_unit.steem_unit_sum();

      auto generated_token_units = std::min(
         contributed_steem_units * ico.capped_generation_policy.max_unit_ratio,
         steem_units_hard_cap * ico.capped_generation_policy.min_unit_ratio
      );

      auto effective_unit_ratio = generated_token_units / contributed_steem_units;

      for ( auto& e : token_unit )
      {
         auto effective_account = smt::generation_unit::get_account( e.first, o.contributor );
         auto token_shares = e.second * steem_units_sent * effective_unit_ratio;

         if ( smt::generation_unit::is_vesting( e.first ) )
         {
            db.create_vesting( db.get_account( effective_account ), asset( token_shares, o.symbol ) );
         }
         else
         {
            db.adjust_balance( effective_account, asset( token_shares, o.symbol ) );
            db.adjust_supply( asset( token_shares, o.symbol ) );
         }
      }

      for ( auto& e : steem_unit )
      {
         auto effective_account = smt::generation_unit::get_account( e.first, o.contributor );
         auto steem_shares = e.second * steem_units_sent;

         if ( smt::generation_unit::is_vesting( e.first ) )
            db.create_vesting( db.get_account( effective_account ), asset( steem_shares, STEEM_SYMBOL ) );
         else
            db.adjust_balance( effective_account, asset( steem_shares, STEEM_SYMBOL ) );

         db.modify( ico, [&]( smt_ico_object& obj )
         {
            obj.processed += steem_shares;
         } );
      }
   },
   smt::ico::schedule_next_payout,
   []( database& db, const asset_symbol_type& a )
   {
      db.remove( db.get< smt_ico_object, by_symbol >( a ) );
   } );
}
#endif

} } //steem::chain
