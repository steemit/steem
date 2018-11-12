#pragma once
#include <steem/protocol/types.hpp>
#include <steem/protocol/base.hpp>

namespace steem { namespace protocol {

#ifdef IS_TEST_NET
   struct example_optional_action : public base_operation
   {
      account_name_type account;

      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(account); }
   };
#endif

} } // steem::protocol

#ifdef IS_TEST_NET
FC_REFLECT( steem::protocol::example_optional_action, (account) )
#endif
