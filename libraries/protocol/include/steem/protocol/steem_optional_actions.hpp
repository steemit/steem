#pragma once
#include <steem/protocol/types.hpp>
#include <steem/protocol/base.hpp>
#include <steem/protocol/asset.hpp>

namespace steem { namespace protocol {

#ifdef IS_TEST_NET
   struct example_optional_action : public base_operation
   {
      account_name_type account;

      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(account); }
   };
#endif

   struct smt_token_emission_action : public base_operation
   {
      account_name_type control_account;
      asset_symbol_type symbol;
      asset             emission;

      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(control_account); }
   };

} } // steem::protocol

#ifdef IS_TEST_NET
FC_REFLECT( steem::protocol::example_optional_action, (account) )
#endif

FC_REFLECT( steem::protocol::smt_token_emission_action, (control_account)(symbol)(emission) )
