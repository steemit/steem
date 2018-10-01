#pragma once

#include <steem/plugins/json_rpc/utility.hpp>

#include <steem/plugins/transaction_status_api/transaction_status_api_args.hpp>
#include <steem/plugins/transaction_status_api/transaction_status_api_objects.hpp>

namespace steem { namespace plugins { namespace transaction_status_api {

class transaction_status_api_impl;

class transaction_status_api
{
   public:
      transaction_status_api();
      ~transaction_status_api();

      DECLARE_API(
         (find_transaction)
      )

   private:
      std::unique_ptr< transaction_status_api_impl > my;
};

} } } //steem::plugins::transaction_status_api

