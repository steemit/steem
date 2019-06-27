#pragma once

#include <dpn/protocol/types.hpp>
#include <dpn/protocol/transaction.hpp>
#include <dpn/protocol/block_header.hpp>

#include <dpn/plugins/json_rpc/utility.hpp>
#include <dpn/plugins/transaction_status/transaction_status_objects.hpp>

namespace dpn { namespace plugins { namespace transaction_status_api {

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

} } } // dpn::transaction_status_api

FC_REFLECT( dpn::plugins::transaction_status_api::find_transaction_args, (transaction_id)(expiration) )
FC_REFLECT( dpn::plugins::transaction_status_api::find_transaction_return, (status)(block_num) )
