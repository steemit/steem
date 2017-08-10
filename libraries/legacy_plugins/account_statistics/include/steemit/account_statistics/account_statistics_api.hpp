#pragma once

#include <steemit/account_statistics/account_statistics_plugin.hpp>

#include <fc/api.hpp>

namespace steemit{ namespace app {
   struct api_context;
} }

namespace steemit { namespace account_statistics {

namespace detail
{
   class account_statistics_api_impl;
}

class account_statistics_api
{
   public:
      account_statistics_api( const steemit::app::api_context& ctx );

      void on_api_startup();

   private:
      std::shared_ptr< detail::account_statistics_api_impl > _my;
};

} } // steemit::account_statistics

FC_API( steemit::account_statistics::account_statistics_api, )