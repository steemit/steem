#pragma once

#include <steemit/app/application.hpp>

#include <steemit/account_by_key/account_by_key_objects.hpp>

#include <fc/api.hpp>

namespace steemit { namespace account_by_key {

namespace detail
{
   class account_by_key_api_impl;
}
/**
 * @brief Lookup accounts by the keys associated with them.
*/
class account_by_key_api
{
   public:
      account_by_key_api( const app::api_context& ctx );

      void on_api_startup();

      /**
       * @brief Lookup accounts by the keys associated with them.
       * @param keys vector of @ref public_key_type.
       * @returns 2D vector array of @ref account_name_type: vector< vector< @ref account_name_type > >
      */

      vector< vector< account_name_type > > get_key_references( vector< public_key_type > keys )const;

   private:
      std::shared_ptr< detail::account_by_key_api_impl > my;
};

} } // steemit::account_by_key

FC_API( steemit::account_by_key::account_by_key_api, (get_key_references) )
