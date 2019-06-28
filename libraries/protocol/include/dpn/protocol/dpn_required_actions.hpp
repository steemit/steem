#pragma once
#include <dpn/protocol/types.hpp>
#include <dpn/protocol/base.hpp>

namespace dpn { namespace protocol {

#ifdef IS_TEST_NET
   struct example_required_action : public base_operation
   {
      account_name_type account;

      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(account); }

      friend bool operator==( const example_required_action& lhs, const example_required_action& rhs );
   };
#endif

} } // dpn::protocol

#ifdef IS_TEST_NET
FC_REFLECT( dpn::protocol::example_required_action, (account) )
#endif
