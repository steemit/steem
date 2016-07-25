#pragma once

#include <steemit/app/application.hpp>

#include <steemit/follow/follow_plugin.hpp>

#include <fc/api.hpp>

namespace steemit { namespace follow {

using std::vector;
using std::string;

namespace detail
{
   class follow_api_impl;
}

class follow_object;

class follow_api
{
   public:
      follow_api( const app::api_context& ctx );

      void on_api_startup();

      vector<follow_object> get_followers( string to, string start, uint16_t limit )const;
      vector<follow_object> get_following( string from, string start, uint16_t limit )const;

   private:
      std::shared_ptr< detail::follow_api_impl > my;
};

} } // steemit::follow

FC_API( steemit::follow::follow_api, (get_followers)(get_following) );