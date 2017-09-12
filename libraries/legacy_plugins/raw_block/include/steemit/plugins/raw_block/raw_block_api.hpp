
#pragma once

#include <steemit/chain/steem_object_types.hpp>

#include <fc/api.hpp>

namespace steem { namespace app {
   struct api_context;
} }

namespace steem { namespace plugin { namespace raw_block {

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

class raw_block_api
{
   public:
      raw_block_api( const steem::app::api_context& ctx );

      void on_api_startup();

      get_raw_block_result get_raw_block( get_raw_block_args args );
      void push_raw_block( std::string block_b64 );

   private:
      std::shared_ptr< detail::raw_block_api_impl > my;
};

} } }

FC_REFLECT( steem::plugin::raw_block::get_raw_block_args,
   (block_num)
   )

FC_REFLECT( steem::plugin::raw_block::get_raw_block_result,
   (block_id)
   (previous)
   (timestamp)
   (raw_block)
   )

FC_API( steem::plugin::raw_block::raw_block_api,
   (get_raw_block)
   (push_raw_block)
   )
