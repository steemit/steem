#include <steem/chain/required_action_evaluator.hpp>

namespace steem { namespace chain {

#ifdef IS_TEST_NET

void example_required_evaluator::do_apply( const example_required_action& a ) {}

#endif

} } //steem::chain