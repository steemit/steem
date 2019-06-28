#pragma once

#include <dpn/protocol/types.hpp>

#include <fc/reflect/reflect.hpp>

namespace dpn { namespace protocol {
struct signed_transaction;
} } // dpn::protocol

namespace dpn { namespace plugins { namespace rc {

using dpn::protocol::account_name_type;
using dpn::protocol::signed_transaction;

account_name_type get_resource_user( const signed_transaction& tx );

} } } // dpn::plugins::rc
