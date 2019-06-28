#pragma once

#include <dpn/protocol/dpn_optional_actions.hpp>

#include <dpn/chain/evaluator.hpp>

namespace dpn { namespace chain {

using namespace dpn::protocol;

#ifdef IS_TEST_NET
DPN_DEFINE_ACTION_EVALUATOR( example_optional, optional_automated_action )
#endif

} } //dpn::chain
