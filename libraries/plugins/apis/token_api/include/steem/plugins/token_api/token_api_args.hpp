#pragma once
#include <steem/plugins/token_api/token_api_objects.hpp>

#include <steem/protocol/types.hpp>
#include <steem/protocol/transaction.hpp>

#include <steem/plugins/json_rpc/utility.hpp>

namespace steem { namespace plugins { namespace token_api {

using plugins::json_rpc::void_type;

/* files_check */
typedef void_type files_check_args;
typedef void_type files_check_return;

/* db_check */
typedef void_type db_check_args;
typedef void_type db_check_return;

} } } // steem::token_api
 