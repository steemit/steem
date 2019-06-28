#pragma once

#include <steem/protocol/types.hpp>

#include <fc/reflect/reflect.hpp>

namespace steem { namespace protocol {
struct signed_transaction;
} } // steem::protocol

namespace steem { namespace plugins { namespace rc {

using steem::protocol::account_name_type;
using steem::protocol::signed_transaction;

account_name_type get_resource_user( const signed_transaction& tx );

} } } // steem::plugins::rc
