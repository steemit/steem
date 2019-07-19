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

      const auto& token_unit = ico.capped_generation_policy.pre_soft_cap_unit.token_unit;
      const auto& steem_unit = ico.capped_generation_policy.pre_soft_cap_unit.steem_unit;

      auto steem_units_sent = o.contribution.amount / ico.capped_generation_policy.pre_soft_cap_unit.steem_unit_sum();
      auto contributed_steem_units = ico.contributed.amount / ico.capped_generation_policy.pre_soft_cap_unit.steem_unit_sum();
      auto steem_units_hard_cap = ico.steem_units_hard_cap / ico.capped_generation_policy.pre_soft_cap_unit.steem_unit_sum();

      auto generated_token_units = std::min(
         contributed_steem_units * ico.capped_generation_policy.max_unit_ratio,
         steem_units_hard_cap * ico.capped_generation_policy.min_unit_ratio
      );

      auto effective_unit_ratio = generated_token_units / contributed_steem_units;

      for ( auto& e : token_unit )
      {
         auto effective_account = smt::generation_unit::get_account( e.first, o.contributor );
         auto token_share = e.second * steem_units_sent * effective_unit_ratio;

         if ( smt::generation_unit::is_vesting( e.first ) )
         {
            db.create_vesting( db.get_account( effective_account ), asset( token_share, o.symbol ) );
         }
         else
         {
            db.adjust_balance( effective_account, asset( token_share, o.symbol ) );
            db.adjust_supply( asset( token_share, o.symbol ) );
         }
      }

      for ( auto& e : steem_unit )
      {
         auto effective_account = smt::generation_unit::get_account( e.first, o.contributor );
         auto token_share = e.second * steem_units_sent;

         if ( smt::generation_unit::is_vesting( e.first ) )
            db.create_vesting( db.get_account( effective_account ), asset( token_share, STEEM_SYMBOL ) );
         else
            db.adjust_balance( effective_account, asset( token_share, STEEM_SYMBOL ) );
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
