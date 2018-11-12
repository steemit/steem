#pragma once
#include <steem/protocol/types.hpp>
#include <steem/protocol/base.hpp>

namespace steem { namespace protocol {

#ifdef IS_TEST_NET
   struct example_required_action : public base_operation
   {
      account_name_type account;

      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(account); }

      friend bool operator==( const example_required_action& lhs, const example_required_action& rhs );
   };
#endif

} } // steem::protocol

#ifdef IS_TEST_NET
FC_REFLECT( steem::protocol::example_required_action, (account) )
#endif
