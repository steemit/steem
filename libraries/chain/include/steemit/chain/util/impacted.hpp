#pragma once

#include <fc/container/flat.hpp>
#include <steemit/protocol/operations.hpp>
#include <steemit/protocol/transaction.hpp>

#include <fc/string.hpp>

namespace steem { namespace app {

using namespace fc;

void operation_get_impacted_accounts(
   const steem::protocol::operation& op,
   fc::flat_set<protocol::account_name_type>& result );

void transaction_get_impacted_accounts(
   const steem::protocol::transaction& tx,
   fc::flat_set<protocol::account_name_type>& result
   );

} } // steem::app
