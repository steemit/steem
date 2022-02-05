#pragma once

#include <steem/plugins/json_rpc/utility.hpp>

#include <steem/plugins/block_api/block_api_args.hpp>

namespace steem { namespace plugins { namespace block_api {

#define BLOCK_API_DEFAULT_QUERY_LIMIT 250

class block_api_impl;

class block_api
{
   public:
      block_api( uint32_t query_limit );
      ~block_api();

      DECLARE_API(

         /////////////////////////////
         // Blocks and transactions //
         /////////////////////////////

         /**
         * @brief Retrieve a block header
         * @param block_num Height of the block whose header should be returned
         * @return header of the referenced block, or null if no matching block was found
         */
         (get_block_header)

         /**
         * @brief Retrieve a full, signed block
         * @param block_num Height of the block to be returned
         * @return the referenced block, or null if no matching block was found
         */
         (get_block)

         /**
         * @brief Retrieve a range of full, signed blocks
         * @param from_num Height of the first block to be returned
         * @param to_num   Height of the last block to be returned
         * @return a sequence of referenced blocks (item is null if no matching block was found)
         */
         (get_blocks_in_range)
      )

   private:
      std::unique_ptr< block_api_impl > my;
};

} } } //steem::plugins::block_api
