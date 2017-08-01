
#pragma once

#include <fc/api.hpp>

#include <steemit/plugins/block_info/block_info.hpp>

namespace steemit { namespace app {
   struct api_context;
} }

namespace steemit { namespace plugin { namespace block_info {

namespace detail {
class block_info_api_impl;
}

struct get_block_info_args
{
   uint32_t start_block_num = 0; //< Which block to begin iterating on.
   uint32_t count           = 1000; //< How many times to itterate over after this block.
};

/**
 * @brief Allows block information to be collected on a block by block basis.
*/

class block_info_api
{
   public:
      block_info_api( const steemit::app::api_context& ctx );

      void on_api_startup();
      /**
       * @brief get the information about a block alone.
       * @param args @ref get_block_info_args object.
       * @returns vector array of @ref block_info objects
       */
      std::vector< block_info > get_block_info( get_block_info_args args );

      /**
       * @brief get the information about a block and the actual signed block.
       * @param args @ref get_block_info_args object.
       * @returns vector array of @ref block_with_info objects
       */
      std::vector< block_with_info > get_blocks_with_info( get_block_info_args args );

   private:
      std::shared_ptr< detail::block_info_api_impl > my;
};

} } }

FC_REFLECT( steemit::plugin::block_info::get_block_info_args,
   (start_block_num)
   (count)
   )

FC_API( steemit::plugin::block_info::block_info_api,
   (get_block_info)
   (get_blocks_with_info)
   )
