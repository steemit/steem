#pragma once
#include <steem/plugins/transaction_status_api/transaction_status_api_objects.hpp>

#include <steem/protocol/types.hpp>
#include <steem/protocol/transaction.hpp>
#include <steem/protocol/block_header.hpp>

#include <steem/plugins/json_rpc/utility.hpp>

namespace steem { namespace plugins { namespace transaction_status_api {

struct find_transaction_args
{
   transaction_id_type id;
   fc::time_point_sec expiration;
};

struct find_transaction_return
{
   transaction_status status;
};

} } } // steem::transaction_status_api

FC_REFLECT( steem::plugins::transaction_status_api::find_transaction_args, (id)(expiration) )
FC_REFLECT( steem::plugins::transaction_status_api::find_transaction_return, (status) )
