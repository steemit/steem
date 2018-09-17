#pragma once

#include <steem/plugins/json_rpc/utility.hpp>
#include <steem/protocol/types.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>

namespace steem { namespace plugins { namespace transaction_polling_api {

using json_rpc::void_type;

enum transaction_status_codes
{
    not_aware_of_trx = 0,
    aware_of_trx,
    exp_time_future_trx_not_in_block_or_mempool,
    trx_in_mempool,
    trx_inclided_in_block_and_block_not_irreversible,
    trx_included_in_block_and_block_irreversible,
    trx_expired_trx_is_not_irreversible_could_be_in_fork,
    trx_expired_trx_is_irreversible_could_be_in_fork,
    trx_is_too_old
};

struct find_transaction_args
{
    fc::variant trx_id;
    fc::variant expiration;
};

struct find_transaction_return
{
    transaction_status_codes trx_status_code;
};

class transaction_polling_api_impl;

class transaction_polling_api
{
   public:
      transaction_polling_api();
      ~transaction_polling_api();

      DECLARE_API(
         /**
         * This is a transaction polling api. This api takes transaction id and expiration (optional) as an arguments
         * @return the state of the transaction (integer)
         *
         * Only transaction id is provided (no expiration time), then it returns
         * 0 - Not aware of the transaction (if transaction not found)
         * 1 - Aware of the transaction (if transaction is found)
         *
         * Both transaction id and expiration time are provided, then it returns
         * 2 - Expiration time in future, transaction not included in block or mempool
         * 3 - Transaction in mempool
         * 4 - Transaction has been included in block, block not irreversible
         * 5 - Transaction has been included in block, block is irreversible
         * 6 - Transaction has expired, transaction is not irreversible (transaction could be in a fork)
         * 7 - Transaction has expired, transaction is irreversible (transaction cannot be in a fork)
         * 8 - Transaction is too old, I don't know about it
         */
         (find_transaction)
      )

   private:
      std::unique_ptr< transaction_polling_api_impl > my;
};

} } } // steem::plugins::debug_node

FC_REFLECT_ENUM( steem::plugins::transaction_polling_api::transaction_status_codes,
                (not_aware_of_trx)
                (aware_of_trx)
                (exp_time_future_trx_not_in_block_or_mempool)
                (trx_in_mempool)
                (trx_inclided_in_block_and_block_not_irreversible)
                (trx_included_in_block_and_block_irreversible)
                (trx_expired_trx_is_not_irreversible_could_be_in_fork)
                (trx_expired_trx_is_irreversible_could_be_in_fork)
                (trx_is_too_old) )

FC_REFLECT( steem::plugins::transaction_polling_api::find_transaction_args,
           (trx_id)
           (expiration) )

FC_REFLECT( steem::plugins::transaction_polling_api::find_transaction_return,
           (trx_status_code) )
