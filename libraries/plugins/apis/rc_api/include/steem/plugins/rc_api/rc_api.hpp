#pragma once
#include <steem/plugins/json_rpc/utility.hpp>
#include <steem/plugins/rc/rc_objects.hpp>

#include <steem/protocol/types.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>

#define RC_API_SINGLE_QUERY_LIMIT 1000

namespace steem { namespace plugins { namespace rc {

namespace detail
{
   class rc_api_impl;
}

using plugins::json_rpc::void_type;

typedef void_type get_resource_params_args;

struct get_resource_params_return
{
   vector< string >                                         resource_names;
   variant_object                                           resource_params;
   variant_object                                           size_info;
};

typedef void_type get_resource_pool_args;

struct resource_pool_api_object
{
   int64_t pool = 0;
};

struct get_resource_pool_return
{
   variant_object                                           resource_pool;
};

struct rc_account_api_object
{
   account_name_type     account;
   steem::chain::util::manabar   rc_manabar;
   asset                 max_rc_creation_adjustment = asset( 0, VESTS_SYMBOL );
   int64_t               max_rc = 0;

   //
   // This is used for bug-catching, to match that the vesting shares in a
   // pre-op are equal to what they were at the last post-op.
   //
   // Don't return it in the API for now, because we don't want
   // API's to be built to assume its presence
   //
   // If it's needed it for debugging, we might think about un-commenting it
   //
   // asset                 last_vesting_shares = asset( 0, VESTS_SYMBOL );
};

struct find_rc_accounts_args
{
   std::vector< account_name_type >                         accounts;
};

struct find_rc_accounts_return
{
   std::vector< rc_account_api_object >                     rc_accounts;
};

class rc_api
{
   public:
      rc_api();
      ~rc_api();

      DECLARE_API(
         (get_resource_params)
         (get_resource_pool)
         (find_rc_accounts)
         )

   private:
      std::unique_ptr< detail::rc_api_impl > my;
};

} } } // steem::plugins::rc

FC_REFLECT( steem::plugins::rc::get_resource_params_return,
   (resource_names)
   (resource_params)
   (size_info)
   )

FC_REFLECT( steem::plugins::rc::resource_pool_api_object,
   (pool)
   )

FC_REFLECT( steem::plugins::rc::get_resource_pool_return,
   (resource_pool)
   )

FC_REFLECT( steem::plugins::rc::rc_account_api_object,
   (account)
   (rc_manabar)
   (max_rc_creation_adjustment)
   (max_rc)
   )

FC_REFLECT( steem::plugins::rc::find_rc_accounts_args,
   (accounts)
   )

FC_REFLECT( steem::plugins::rc::find_rc_accounts_return,
   (rc_accounts)
   )
