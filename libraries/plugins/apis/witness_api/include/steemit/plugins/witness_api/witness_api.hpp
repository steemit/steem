#pragma once
#include <steemit/plugins/json_rpc/utility.hpp>
#include <steemit/plugins/witness/witness_objects.hpp>

#include <steemit/protocol/types.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>

namespace steemit { namespace plugins { namespace witness {

namespace detail
{
   class witness_api_impl;
}

struct get_account_bandwidth_args
{
   protocol::account_name_type   account;
   bandwidth_type                type;
};

typedef account_bandwidth_object api_account_bandwidth_object;

struct get_account_bandwidth_return
{
   optional< api_account_bandwidth_object > bandwidth;
};

class witness_api
{
   public:
      witness_api();
      ~witness_api();

      DECLARE_API( (get_account_bandwidth) )

   private:
      std::unique_ptr< detail::witness_api_impl > my;
};

} } } // steemit::plugins::witness

FC_REFLECT( steemit::plugins::witness::get_account_bandwidth_args,
            (account)(type) )

FC_REFLECT( steemit::plugins::witness::get_account_bandwidth_return,
            (bandwidth) )
