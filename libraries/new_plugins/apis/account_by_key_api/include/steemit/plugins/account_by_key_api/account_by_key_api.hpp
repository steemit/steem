#pragma once
#include <steemit/plugins/json_rpc/utility.hpp>

#include <steemit/protocol/types.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>

namespace steemit { namespace plugins { namespace account_by_key_api {

namespace detail
{
   class account_by_key_api_impl;
}

struct get_key_references_args
{
   std::vector< steemit::protocol::public_key_type > keys;
};

struct get_key_references_return
{
   std::vector< std::vector< steemit::protocol::account_name_type > > accounts;
};

class account_by_key_api
{
   public:
      account_by_key_api();

      get_key_references_return get_key_references( const get_key_references_args& keys );

   private:
      std::shared_ptr< detail::account_by_key_api_impl > my;
};

} } } // steemit::plugins::account_by_key_api

FC_REFLECT( steemit::plugins::account_by_key_api::get_key_references_args,
   (keys) )

FC_REFLECT( steemit::plugins::account_by_key_api::get_key_references_return,
   (accounts) )
