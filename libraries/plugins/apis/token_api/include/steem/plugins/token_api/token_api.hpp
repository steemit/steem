#pragma once

#include <steem/plugins/json_rpc/utility.hpp>

#include <steem/plugins/token_api/token_api_args.hpp>

#define TOKEN_API_SINGLE_QUERY_LIMIT 1000

namespace steem { namespace plugins { namespace token_api {

class token_api_impl;

class token_api
{
   public:
      token_api();
      ~token_api();

      DECLARE_API(

         (files_check)
         (db_check)
      )

   private:
      std::unique_ptr< token_api_impl > my;
};

} } } //steem::plugins::token_api

