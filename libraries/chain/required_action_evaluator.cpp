#include <steem/chain/steem_fwd.hpp>
#include <steem/chain/required_action_evaluator.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/smt_objects.hpp>
#include <steem/chain/util/smt_token.hpp>

namespace steem { namespace chain {

#ifdef IS_TEST_NET

void example_required_evaluator::do_apply( const example_required_action& a ) {}

#endif

#ifdef STEEM_ENABLE_SMT
void smt_refund_evaluator::do_apply( const smt_refund_action& a )
{
   auto& idx = _db.get_index< smt_contribution_index, by_symbol_contributor >();
   auto itr = idx.find( boost::make_tuple( a.symbol, a.contributor, a.contribution_id ) );
   FC_ASSERT( itr != idx.end(), "Unable to find contribution object for the provided action: ${a}", ("a", a) );

   auto symbol = itr->symbol;

   _db.adjust_balance( itr->contributor, itr->contribution );
   _db.remove( *itr );

   util::smt::schedule_next_refund( _db, symbol );
}

void smt_contributor_payout_evaluator::do_apply( const smt_contributor_payout_action& a )
{
   auto& idx = _db.get_index< smt_contribution_index, by_symbol_contributor >();
   auto itr = idx.find( boost::make_tuple( a.symbol, a.contributor, a.contribution_id ) );
   FC_ASSERT( itr != idx.end(), "Unable to find contribution object for the provided action: ${a}", ("a", a) );

   auto symbol = itr->symbol;

   _db.remove( *itr );

   util::smt::schedule_next_contributor_payout( _db, symbol );
}
#endif

} } //steem::chain
