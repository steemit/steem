
#pragma once

#include <steemit/chain/steem_object_types.hpp>

#include <fc/api.hpp>

namespace steemit { namespace app {
   struct api_context;
} }

namespace steemit { namespace plugin { namespace raw_block {

namespace detail {
class raw_block_api_impl;
}

struct get_raw_block_args
{
   uint32_t block_num = 0;
};

struct get_raw_block_result
{
   chain::block_id_type          block_id;
   chain::block_id_type          previous;
   fc::time_point_sec            timestamp;
   std::string                   raw_block;
};

/**
 * @brief Allows for unserialized block data to be retrevied.
 */
class raw_block_api
{
   public:
      raw_block_api( const steemit::app::api_context& ctx );

      void on_api_startup();
      /**
       * @brief Allows for unserialized block data to be retrevied.
       * @param args @ref get_raw_block_args object which simply contains which block you wish to recieve.
       * @returns @ref get_raw_block_result which contains the raw block as a string.
       */
      get_raw_block_result get_raw_block( get_raw_block_args args );

      /**
       * @brief Allows for unserialized block data to be broadcast/
       * @param block_b64 string that contains signed block data.
       * @returns void
       */
      void push_raw_block( std::string block_b64 );

   private:
      std::shared_ptr< detail::raw_block_api_impl > my;
};

} } }

FC_REFLECT( steemit::plugin::raw_block::get_raw_block_args,
   (block_num)
   )

FC_REFLECT( steemit::plugin::raw_block::get_raw_block_result,
   (block_id)
   (previous)
   (timestamp)
   (raw_block)
   )

FC_API( steemit::plugin::raw_block::raw_block_api,
   (get_raw_block)
   (push_raw_block)
   )
