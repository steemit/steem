
#pragma once

#include <fc/api.hpp>

namespace steemit { namespace app {
   struct api_context;
} }

namespace steemit { namespace plugin { namespace chain_stream {

namespace detail {
class chain_stream_api_impl;
}

class chain_stream_api
{
   public:
      chain_stream_api( const steemit::app::api_context& ctx );

      void on_api_startup();

      // TODO:  Add API methods here

   private:
      std::shared_ptr< detail::chain_stream_api_impl > my;
};

} } }

FC_API( steemit::plugin::chain_stream::chain_stream_api,
   // TODO:  Add method bubble list here
   )
