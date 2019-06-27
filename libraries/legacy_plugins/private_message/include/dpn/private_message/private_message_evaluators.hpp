#pragma once

#include <dpn/chain/evaluator.hpp>

#include <dpn/private_message/private_message_operations.hpp>
#include <dpn/private_message/private_message_plugin.hpp>

namespace dpn { namespace private_message {

DPN_DEFINE_PLUGIN_EVALUATOR( private_message_plugin, dpn::private_message::private_message_plugin_operation, private_message )

} }
