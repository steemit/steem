#include <steem/chain/optional_action_evaluator.hpp>

namespace steem { namespace chain {

#ifdef IS_TEST_NET

void example_optional_evaluator::do_apply( const example_optional_action& a ) {}

#endif

void smt_token_emission_evaluator::do_apply( const smt_token_emission_action& a )
{

}

} } //steem::chain
