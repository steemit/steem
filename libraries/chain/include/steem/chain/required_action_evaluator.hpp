#pragma once

#include <steem/protocol/steem_required_actions.hpp>

#include <steem/chain/evaluator.hpp>

namespace steem { namespace chain {

using namespace steem::protocol;

#ifdef IS_TEST_NET
STEEM_DEFINE_ACTION_EVALUATOR( example_required, required_automated_action )
STEEM_DEFINE_ACTION_EVALUATOR( example_large_required, required_automated_action )
#endif

STEEM_DEFINE_ACTION_EVALUATOR( smt_ico_launch, required_automated_action )
STEEM_DEFINE_ACTION_EVALUATOR( smt_ico_evaluation, required_automated_action )
STEEM_DEFINE_ACTION_EVALUATOR( smt_token_launch, required_automated_action )
STEEM_DEFINE_ACTION_EVALUATOR( smt_refund, required_automated_action )
STEEM_DEFINE_ACTION_EVALUATOR( smt_contributor_payout, required_automated_action )
STEEM_DEFINE_ACTION_EVALUATOR( smt_founder_payout, required_automated_action )

} } //steem::chain
