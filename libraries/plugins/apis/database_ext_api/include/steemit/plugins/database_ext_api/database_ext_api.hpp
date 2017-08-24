#pragma once

#include <steemit/plugins/json_rpc/utility.hpp>

#include <steemit/plugins/database_ext_api/database_ext_api_args.hpp>

#define DATABASE_EXT_API_SINGLE_QUERY_LIMIT 1000

namespace steemit { namespace plugins { namespace database_ext_api {

class database_ext_api_impl;

class database_ext_api
{
   public:
      database_ext_api();
      ~database_ext_api();

      DECLARE_API(
         /////////////////////////////
         // Blocks and transactions //
         /////////////////////////////

         /**
         * @brief Retrieve a full, signed block
         * @param block_num Height of the block to be returned
         * @return the referenced block, or null if no matching block was found
         */
         (get_block)
      )

   private:
      std::unique_ptr< database_ext_api_impl > my;
};

} } } //steemit::plugins::database_ext_api

