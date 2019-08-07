#pragma once
#include <steem/protocol/types.hpp>
#include <steem/protocol/base.hpp>
#include <steem/protocol/asset.hpp>
#include <steem/protocol/misc_utilities.hpp>

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
      account_name_type contributor;
      asset_symbol_type symbol;
      uint32_t          contribution_id;
      asset             refund;

      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const { a.insert( contributor ); }

      friend bool operator==( const smt_refund_action& lhs, const smt_refund_action& rhs );
   };

   struct smt_contributor_payout_action : public base_operation
   {
      account_name_type    contributor;
      asset_symbol_type    symbol;
      uint32_t             contribution_id;
      std::vector< asset > payouts;

      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const { a.insert( contributor ); }

      friend bool operator==( const smt_contributor_payout_action& lhs, const smt_contributor_payout_action& rhs );
   };

   struct smt_founder_payout_action : public base_operation
   {
      asset_symbol_type                                   symbol;
      std::map< account_name_type, std::vector< asset > > account_payouts;
      share_type                                          market_maker_steem  = 0;
      share_type                                          market_maker_tokens = 0;
      share_type                                          rewards_fund        = 0;

      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const
      {
         for ( auto& entry : account_payouts )
            a.insert( entry.first );
      }

      friend bool operator==( const smt_founder_payout_action& lhs, const smt_founder_payout_action& rhs );
   };

   struct smt_ico_launch_action : public base_operation
   {
      account_name_type control_account;
      asset_symbol_type symbol;

      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const { a.insert( control_account ); }

      friend bool operator==( const smt_ico_launch_action& lhs, const smt_ico_launch_action& rhs );
   };

   struct smt_ico_evaluation_action : public base_operation
   {
      account_name_type control_account;
      asset_symbol_type symbol;

      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const { a.insert( control_account ); }

      friend bool operator==( const smt_ico_evaluation_action& lhs, const smt_ico_evaluation_action& rhs );
   };

   struct smt_token_launch_action : public base_operation
   {
      account_name_type control_account;
      asset_symbol_type symbol;

      void validate()const;
      void get_required_active_authorities( flat_set<account_name_type>& a )const { a.insert( control_account ); }

      friend bool operator==( const smt_token_launch_action& lhs, const smt_token_launch_action& rhs );
   };
#endif
} } // steem::protocol

#ifdef IS_TEST_NET
FC_REFLECT( steem::protocol::example_required_action, (account) )
#endif

#ifdef STEEM_ENABLE_SMT
FC_REFLECT( steem::protocol::smt_refund_action, (contributor)(symbol)(contribution_id)(refund) )
FC_REFLECT( steem::protocol::smt_contributor_payout_action, (contributor)(symbol)(contribution_id)(payouts) )
FC_REFLECT( steem::protocol::smt_founder_payout_action, (symbol)(account_payouts)(market_maker_steem)(market_maker_tokens)(rewards_fund) )
FC_REFLECT( steem::protocol::smt_ico_launch_action, (control_account)(symbol) )
FC_REFLECT( steem::protocol::smt_ico_evaluation_action, (control_account)(symbol) )
FC_REFLECT( steem::protocol::smt_token_launch_action, (control_account)(symbol) )
#endif
