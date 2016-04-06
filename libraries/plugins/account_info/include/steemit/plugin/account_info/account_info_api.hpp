
#pragma once

#include <steemit/chain/account_object.hpp>

#include <steemit/chain/protocol/types.hpp>

#include <fc/api.hpp>
#include <fc/optional.hpp>

#include <set>
#include <string>
#include <vector>

namespace steemit { namespace app {
class application;
} }

namespace steemit { namespace chain {
class database;
} }

namespace steemit { namespace plugin { namespace account_info {

namespace detail { class account_info_api_impl; }

using namespace std;
using namespace steemit::chain;


class account_info_api
{
   public:
      account_info_api( const chain::database& db );
      account_info_api( const app::application& app );

      struct get_accounts_args { vector< string > accounts; };
      struct get_accounts_ret  { vector< account_object > account_objs; };

      /**
       * @brief Get a list of accounts by name
       * @param accounts Names of the accounts to retrieve
       * @return The accounts holding the provided names
       */
      get_accounts_ret get_accounts( get_accounts_args args )const;

      struct list_accounts_for_auth_members_args { vector<string> auth_members; };
      struct list_accounts_for_auth_members_ret  { map< string, vector<string> > accounts_by_auth_member; };

      /**
       *  @return all accounts that refer to the given key or account id in their owner or active authorities.
       */
      list_accounts_for_auth_members_ret list_accounts_for_auth_members( list_accounts_for_auth_members_args args )const;

      struct list_accounts_args { string after_account; uint32_t count = 0; };
      struct list_accounts_ret  { set<string> names; };

      /**
       * @brief Get names and IDs for registered accounts
       * @param after_account Lower bound of the first name to return
       * @param count Maximum number of results to return -- must not exceed 1000
       * @return Map of account names to corresponding IDs
       */
      list_accounts_ret list_accounts( list_accounts_args args )const;

      struct get_account_count_args {  };
      struct get_account_count_ret  { uint64_t num_accounts = 0; };

      /**
       * @brief Get the total number of accounts registered with the blockchain
       */
      get_account_count_ret get_account_count( get_account_count_args args )const;

   private:
      std::shared_ptr< detail::account_info_api_impl > _my;
};

} } }

FC_REFLECT( steemit::plugin::account_info::account_info_api::get_accounts_args, (accounts) )
FC_REFLECT( steemit::plugin::account_info::account_info_api::get_accounts_ret, (account_objs) )

FC_REFLECT( steemit::plugin::account_info::account_info_api::list_accounts_for_auth_members_args, (auth_members) )
FC_REFLECT( steemit::plugin::account_info::account_info_api::list_accounts_for_auth_members_ret, (accounts_by_auth_member) )

FC_REFLECT( steemit::plugin::account_info::account_info_api::list_accounts_args, (after_account)(count) )
FC_REFLECT( steemit::plugin::account_info::account_info_api::list_accounts_ret, (names) )

FC_REFLECT( steemit::plugin::account_info::account_info_api::get_account_count_args,  )
FC_REFLECT( steemit::plugin::account_info::account_info_api::get_account_count_ret, (num_accounts) )

FC_API( steemit::plugin::account_info::account_info_api,
   (get_accounts)
   (list_accounts_for_auth_members)
   (list_accounts)
   (get_account_count)
   )
