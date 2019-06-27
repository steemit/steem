#pragma once

#include <dpn/protocol/dpn_required_actions.hpp>

#include <dpn/chain/evaluator.hpp>

namespace dpn { namespace chain {

using namespace dpn::protocol;

#ifdef IS_TEST_NET
DPN_DEFINE_ACTION_EVALUATOR( example_required, required_automated_action )
#endif

} } //dpn::chain
