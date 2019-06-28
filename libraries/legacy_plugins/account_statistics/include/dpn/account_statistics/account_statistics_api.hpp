#pragma once

#include <dpn/account_statistics/account_statistics_plugin.hpp>

#include <fc/api.hpp>

namespace dpn { namespace app {
   struct api_context;
} }

namespace dpn { namespace account_statistics {

namespace detail
{
   class account_statistics_api_impl;
}

class account_statistics_api
{
   public:
      account_statistics_api( const dpn::app::api_context& ctx );

      void on_api_startup();

   private:
      std::shared_ptr< detail::account_statistics_api_impl > _my;
};

} } // dpn::account_statistics

FC_API( dpn::account_statistics::account_statistics_api, )