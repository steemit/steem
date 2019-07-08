#include <steem/chain/steem_fwd.hpp>
#include <steem/chain/required_action_evaluator.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/smt_objects.hpp>

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

   auto next = itr;
   ++next;

   // If the next contribution object is of the same symbol and we are not at the end of the index
   if ( next != idx.end() && next->symbol == a.symbol )
   {
      // Set next refund
      smt_refund_action refund_action;
      refund_action.symbol = next->symbol;
      refund_action.contributor = next->contributor;
      refund_action.contribution_id = next->contribution_id;

      _db.push_required_action( refund_action, _db.head_block_time() + STEEM_BLOCK_INTERVAL );
   }

   // Refund the contributors STEEM and remove the contribution from state
   _db.adjust_balance( itr->contributor, itr->contribution );
   _db.remove( *itr );
}
#endif

} } //steem::chain
