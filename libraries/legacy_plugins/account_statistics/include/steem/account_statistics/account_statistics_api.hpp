#pragma once

#include <steem/account_statistics/account_statistics_plugin.hpp>

#include <fc/api.hpp>

namespace steem { namespace app {
   struct api_context;
} }

namespace steem { namespace account_statistics {

namespace detail
{
   class account_statistics_api_impl;
}

class account_statistics_api
{
   public:
      account_statistics_api( const steem::app::api_context& ctx );

      void on_api_startup();

   private:
      std::shared_ptr< detail::account_statistics_api_impl > _my;
};

} } // steem::account_statistics

FC_API( steem::account_statistics::account_statistics_api, )