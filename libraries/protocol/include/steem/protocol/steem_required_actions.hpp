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

#ifdef STEEM_ENABLE_SMT
   struct smt_refund_action : public base_operation
   {
      asset_symbol_type symbol;
      account_name_type contributor;
      uint32_t          contribution_id;

      void validate()const;

      friend bool operator==( const smt_refund_action& lhs, const smt_refund_action& rhs );
   };
#endif
} } // steem::protocol

#ifdef IS_TEST_NET
FC_REFLECT( steem::protocol::example_required_action, (account) )
#endif

#ifdef STEEM_ENABLE_SMT
FC_REFLECT( steem::protocol::smt_refund_action, (symbol)(contributor)(contribution_id) )
#endif
