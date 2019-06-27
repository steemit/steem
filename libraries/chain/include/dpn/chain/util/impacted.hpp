#pragma once

#include <fc/container/flat.hpp>
#include <dpn/protocol/operations.hpp>
#include <dpn/protocol/transaction.hpp>

#include <fc/string.hpp>

namespace dpn { namespace app {

using namespace fc;

void operation_get_impacted_accounts(
   const dpn::protocol::operation& op,
   fc::flat_set<protocol::account_name_type>& result );

void transaction_get_impacted_accounts(
   const dpn::protocol::transaction& tx,
   fc::flat_set<protocol::account_name_type>& result
   );

} } // dpn::app
