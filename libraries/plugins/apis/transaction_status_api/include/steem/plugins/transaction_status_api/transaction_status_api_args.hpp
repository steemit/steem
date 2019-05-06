#pragma once

#include <steem/protocol/types.hpp>
#include <steem/protocol/transaction.hpp>
#include <steem/protocol/block_header.hpp>

#include <steem/plugins/json_rpc/utility.hpp>
#include <steem/plugins/transaction_status/transaction_status_objects.hpp>

namespace steem { namespace plugins { namespace transaction_status_api {

struct find_transaction_args
{
   chain::transaction_id_type transaction_id;
   fc::optional< fc::time_point_sec > expiration;
};

struct find_transaction_return
{
   transaction_status::transaction_status status;
   fc::optional< uint32_t > block_num;
};

} } } // steem::transaction_status_api

FC_REFLECT( steem::plugins::transaction_status_api::find_transaction_args, (transaction_id)(expiration) )
FC_REFLECT( steem::plugins::transaction_status_api::find_transaction_return, (status)(block_num) )
