#pragma once
#include <steem/plugins/database_api/database_api_objects.hpp>

#include <steem/protocol/types.hpp>
#include <steem/protocol/transaction.hpp>
#include <steem/protocol/block_header.hpp>

#include <steem/plugins/json_rpc/utility.hpp>

namespace steem { namespace plugins { namespace database_api {

using protocol::account_name_type;
using protocol::signed_transaction;
using protocol::transaction_id_type;
using protocol::public_key_type;
using plugins::json_rpc::void_type;

enum sort_order_type
{
   by_name,
   by_proxy,
   by_next_vesting_withdrawal,
   by_account,
   by_expiration,
   by_effective_date,
   by_vote_name,
   by_schedule_time,
   by_account_witness,
   by_witness_account,
   by_from_id,
   by_ratification_deadline,
   by_withdraw_route,
   by_destination,
   by_complete_from_id,
   by_to_complete,
   by_delegation,
   by_account_expiration,
   by_conversion_date,
   by_cashout_time,
   by_permlink,
   by_root,
   by_parent,
   by_last_update,
   by_author_last_update,
   by_comment_voter,
   by_voter_comment,
   by_price
};

/* get_config */

typedef void_type          get_config_args;
typedef fc::variant_object get_config_return;


/* Singletons */

/* get_dynamic_global_properties */

typedef void_type                            get_dynamic_global_properties_args;
typedef api_dynamic_global_property_object   get_dynamic_global_properties_return;


/* get_witness_schedule */

typedef void_type                   get_witness_schedule_args;
typedef api_witness_schedule_object get_witness_schedule_return;


/* get_hardfork_properties */

typedef void_type                      get_hardfork_properties_args;
typedef api_hardfork_property_object   get_hardfork_properties_return;


/* get_reward_funds */

typedef void_type get_reward_funds_args;

struct get_reward_funds_return
{
   vector< api_reward_fund_object > funds;
};


/* get_current_price_feed */

typedef void_type get_current_price_feed_args;
typedef price     get_current_price_feed_return;


/* get_current_feed_history */

typedef void_type                get_feed_history_args;
typedef api_feed_history_object  get_feed_history_return;


/* Witnesses */

struct list_witnesses_args
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct list_witnesses_return
{
   vector< api_witness_object > witnesses;
};


struct find_witnesses_args
{
   vector< account_name_type > owners;
};

typedef list_witnesses_return find_witnesses_return;


struct list_witness_votes_args
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct list_witness_votes_return
{
   vector< api_witness_vote_object > votes;
};


typedef void_type get_active_witnesses_args;

struct get_active_witnesses_return
{
   vector< account_name_type > witnesses;
};


/* Account */

struct list_accounts_args
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct list_accounts_return
{
   vector< api_account_object > accounts;
};


struct find_accounts_args
{
   vector< account_name_type > accounts;
};

typedef list_accounts_return find_accounts_return;


/* Owner Auth History */

struct list_owner_histories_args
{
   fc::variant       start;
   uint32_t          limit;
};

struct list_owner_histories_return
{
   vector< api_owner_authority_history_object > owner_auths;
};


struct find_owner_histories_args
{
   account_name_type owner;
};

typedef list_owner_histories_return find_owner_histories_return;


/* Account Recovery Requests */

struct list_account_recovery_requests_args
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct list_account_recovery_requests_return
{
   vector< api_account_recovery_request_object > requests;
};


struct find_account_recovery_requests_args
{
   vector< account_name_type > accounts;
};

typedef list_account_recovery_requests_return find_account_recovery_requests_return;


/* Change Recovery Account Requests */

struct list_change_recovery_account_requests_args
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct list_change_recovery_account_requests_return
{
   vector< api_change_recovery_account_request_object > requests;
};


struct find_change_recovery_account_requests_args
{
   vector< account_name_type > accounts;
};

typedef list_change_recovery_account_requests_return find_change_recovery_account_requests_return;


/* Escrow */

struct list_escrows_args
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct list_escrows_return
{
   vector< api_escrow_object > escrows;
};


struct find_escrows_args
{
   account_name_type from;
};

typedef list_escrows_return find_escrows_return;


/* Vesting Withdraw Routes */

struct list_withdraw_vesting_routes_args
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct list_withdraw_vesting_routes_return
{
   vector< api_withdraw_vesting_route_object > routes;
};


struct find_withdraw_vesting_routes_args
{
   account_name_type account;
   sort_order_type   order;
};

typedef list_withdraw_vesting_routes_return find_withdraw_vesting_routes_return;


/* Savings Withdraw */

struct list_savings_withdrawals_args
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct list_savings_withdrawals_return
{
   vector< api_savings_withdraw_object > withdrawals;
};


struct find_savings_withdrawals_args
{
   account_name_type account;
};

typedef list_savings_withdrawals_return find_savings_withdrawals_return;


/* Vesting Delegations */

struct list_vesting_delegations_args
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct list_vesting_delegations_return
{
   vector< api_vesting_delegation_object > delegations;
};


struct find_vesting_delegations_args
{
   account_name_type account;
};

typedef list_vesting_delegations_return find_vesting_delegations_return;


/* Vesting Delegation Expirations */

struct list_vesting_delegation_expirations_args
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct list_vesting_delegation_expirations_return
{
   vector< api_vesting_delegation_expiration_object > delegations;
};


struct find_vesting_delegation_expirations_args
{
   account_name_type account;
};

typedef list_vesting_delegation_expirations_return find_vesting_delegation_expirations_return;


/* SBD Converstions */

struct list_sbd_conversion_requests_args
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct list_sbd_conversion_requests_return
{
   vector< api_convert_request_object > requests;
};


struct find_sbd_conversion_requests_args
{
   account_name_type account;
};

typedef list_sbd_conversion_requests_return find_sbd_conversion_requests_return;


/* Decline Voting Rights Requests */

struct list_decline_voting_rights_requests_args
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct list_decline_voting_rights_requests_return
{
   vector< api_decline_voting_rights_request_object > requests;
};


struct find_decline_voting_rights_requests_args
{
   vector< account_name_type > accounts;
};

typedef list_decline_voting_rights_requests_return find_decline_voting_rights_requests_return;


/* Comments */

struct list_comments_args
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct list_comments_return
{
   vector< api_comment_object > comments;
};


struct find_comments_args
{
   vector< std::pair< account_name_type, string > > comments;
};

typedef list_comments_return find_comments_return;


/* Votes */

struct list_votes_args
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct list_votes_return
{
   vector< api_comment_vote_object > votes;
};


struct find_votes_args
{
   account_name_type author;
   string            permlink;
};

typedef list_votes_return find_votes_return;


/* Limit Orders */

struct list_limit_orders_args
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct list_limit_orders_return
{
   vector< api_limit_order_object > orders;
};


struct find_limit_orders_args
{
   account_name_type account;
};

typedef list_limit_orders_return find_limit_orders_return;


/* Order Book */

struct get_order_book_args
{
   uint32_t          limit;
};

typedef order_book get_order_book_return;


struct get_transaction_hex_args
{
   signed_transaction trx;
};

struct get_transaction_hex_return
{
   std::string hex;
};


struct get_required_signatures_args
{
   signed_transaction          trx;
   flat_set< public_key_type > available_keys;
};

struct get_required_signatures_return
{
   set< public_key_type > keys;
};


struct get_potential_signatures_args
{
   signed_transaction trx;
};

typedef get_required_signatures_return get_potential_signatures_return;


struct verify_authority_args
{
   signed_transaction trx;
};

struct verify_authority_return
{
   bool valid;
};

struct verify_account_authority_args
{
   account_name_type           account;
   flat_set< public_key_type > signers;
};

typedef verify_authority_return verify_account_authority_return;

struct verify_signatures_args
{
   fc::sha256                    hash;
   vector< signature_type >      signatures;
   vector< account_name_type >   required_owner;
   vector< account_name_type >   required_active;
   vector< account_name_type >   required_posting;
   vector< authority >           required_other;

   void get_required_owner_authorities( flat_set< account_name_type >& a )const
   {
      a.insert( required_owner.begin(), required_owner.end() );
   }

   void get_required_active_authorities( flat_set< account_name_type >& a )const
   {
      a.insert( required_active.begin(), required_active.end() );
   }

   void get_required_posting_authorities( flat_set< account_name_type >& a )const
   {
      a.insert( required_posting.begin(), required_posting.end() );
   }

   void get_required_authorities( vector< authority >& a )const
   {
      a.insert( a.end(), required_other.begin(), required_other.end() );
   }
};

struct verify_signatures_return
{
   bool valid;
};

#ifdef STEEM_ENABLE_SMT
typedef void_type get_smt_next_identifier_args;

struct get_smt_next_identifier_return
{
   vector< asset_symbol_type > nais;
};
#endif

} } } // steem::database_api

FC_REFLECT_ENUM( steem::plugins::database_api::sort_order_type,
   (by_name)
   (by_proxy)
   (by_next_vesting_withdrawal)
   (by_account)
   (by_expiration)
   (by_effective_date)
   (by_vote_name)
   (by_schedule_time)
   (by_account_witness)
   (by_witness_account)
   (by_from_id)
   (by_ratification_deadline)
   (by_withdraw_route)
   (by_destination)
   (by_complete_from_id)
   (by_to_complete)
   (by_delegation)
   (by_account_expiration)
   (by_conversion_date)
   (by_cashout_time)
   (by_permlink)
   (by_root)
   (by_parent)
   (by_last_update)
   (by_author_last_update)
   (by_comment_voter)
   (by_voter_comment)
   (by_price) )

FC_REFLECT( steem::plugins::database_api::get_reward_funds_return,
   (funds) )

FC_REFLECT( steem::plugins::database_api::list_witnesses_args,
   (start)(limit)(order) )

FC_REFLECT( steem::plugins::database_api::list_witnesses_return,
   (witnesses) )

FC_REFLECT( steem::plugins::database_api::find_witnesses_args,
   (owners) )

FC_REFLECT( steem::plugins::database_api::list_witness_votes_args,
   (start)(limit)(order) )

FC_REFLECT( steem::plugins::database_api::list_witness_votes_return,
   (votes) )

FC_REFLECT( steem::plugins::database_api::get_active_witnesses_return,
   (witnesses) )

FC_REFLECT( steem::plugins::database_api::list_accounts_args,
   (start)(limit)(order) )

FC_REFLECT( steem::plugins::database_api::list_accounts_return,
   (accounts) )

FC_REFLECT( steem::plugins::database_api::find_accounts_args,
   (accounts) )

FC_REFLECT( steem::plugins::database_api::list_owner_histories_args,
   (start)(limit) )

FC_REFLECT( steem::plugins::database_api::list_owner_histories_return,
   (owner_auths) )

FC_REFLECT( steem::plugins::database_api::find_owner_histories_args,
   (owner) )

FC_REFLECT( steem::plugins::database_api::list_account_recovery_requests_args,
   (start)(limit)(order) )

FC_REFLECT( steem::plugins::database_api::list_account_recovery_requests_return,
   (requests) )

FC_REFLECT( steem::plugins::database_api::find_account_recovery_requests_args,
   (accounts) )

FC_REFLECT( steem::plugins::database_api::list_change_recovery_account_requests_args,
   (start)(limit)(order) )

FC_REFLECT(
   steem::plugins::database_api::list_change_recovery_account_requests_return,
   (requests) )

FC_REFLECT( steem::plugins::database_api::find_change_recovery_account_requests_args,
   (accounts) )

FC_REFLECT( steem::plugins::database_api::list_escrows_args,
   (start)(limit)(order) )

FC_REFLECT( steem::plugins::database_api::list_escrows_return,
   (escrows) )

FC_REFLECT( steem::plugins::database_api::find_escrows_args,
   (from) )

FC_REFLECT( steem::plugins::database_api::list_withdraw_vesting_routes_args,
   (start)(limit)(order) )

FC_REFLECT( steem::plugins::database_api::list_withdraw_vesting_routes_return,
   (routes) )

FC_REFLECT( steem::plugins::database_api::find_withdraw_vesting_routes_args,
   (account)(order) )

FC_REFLECT( steem::plugins::database_api::list_savings_withdrawals_args,
   (start)(limit)(order) )

FC_REFLECT( steem::plugins::database_api::list_savings_withdrawals_return,
   (withdrawals) )

FC_REFLECT( steem::plugins::database_api::find_savings_withdrawals_args,
   (account) )

FC_REFLECT( steem::plugins::database_api::list_vesting_delegations_args,
   (start)(limit)(order) )

FC_REFLECT( steem::plugins::database_api::list_vesting_delegations_return,
   (delegations) )

FC_REFLECT( steem::plugins::database_api::find_vesting_delegations_args,
   (account) )

FC_REFLECT( steem::plugins::database_api::list_vesting_delegation_expirations_args,
   (start)(limit)(order) )

FC_REFLECT( steem::plugins::database_api::list_vesting_delegation_expirations_return,
   (delegations) )

FC_REFLECT( steem::plugins::database_api::find_vesting_delegation_expirations_args,
   (account) )

FC_REFLECT( steem::plugins::database_api::list_sbd_conversion_requests_args,
   (start)(limit)(order) )

FC_REFLECT( steem::plugins::database_api::list_sbd_conversion_requests_return,
   (requests) )

FC_REFLECT( steem::plugins::database_api::find_sbd_conversion_requests_args,
   (account) )

FC_REFLECT( steem::plugins::database_api::list_decline_voting_rights_requests_args,
   (start)(limit)(order) )

FC_REFLECT( steem::plugins::database_api::list_decline_voting_rights_requests_return,
   (requests) )

FC_REFLECT( steem::plugins::database_api::find_decline_voting_rights_requests_args,
   (accounts) )

FC_REFLECT( steem::plugins::database_api::list_comments_args,
   (start)(limit)(order) )

FC_REFLECT( steem::plugins::database_api::list_comments_return,
   (comments) )

FC_REFLECT( steem::plugins::database_api::find_comments_args,
   (comments) )

FC_REFLECT( steem::plugins::database_api::list_votes_args,
   (start)(limit)(order) )

FC_REFLECT( steem::plugins::database_api::list_votes_return,
   (votes) )

FC_REFLECT( steem::plugins::database_api::find_votes_args,
   (author)(permlink) )

FC_REFLECT( steem::plugins::database_api::list_limit_orders_args,
   (start)(limit)(order) )

FC_REFLECT( steem::plugins::database_api::list_limit_orders_return,
   (orders) )

FC_REFLECT( steem::plugins::database_api::find_limit_orders_args,
   (account) )

FC_REFLECT( steem::plugins::database_api::get_order_book_args,
   (limit) )

FC_REFLECT( steem::plugins::database_api::get_transaction_hex_args,
   (trx) )

FC_REFLECT( steem::plugins::database_api::get_transaction_hex_return,
   (hex) )

FC_REFLECT( steem::plugins::database_api::get_required_signatures_args,
   (trx)
   (available_keys) )

FC_REFLECT( steem::plugins::database_api::get_required_signatures_return,
   (keys) )

FC_REFLECT( steem::plugins::database_api::get_potential_signatures_args,
   (trx) )

FC_REFLECT( steem::plugins::database_api::verify_authority_args,
   (trx) )

FC_REFLECT( steem::plugins::database_api::verify_authority_return,
   (valid) )

FC_REFLECT( steem::plugins::database_api::verify_account_authority_args,
   (account)
   (signers) )

FC_REFLECT( steem::plugins::database_api::verify_signatures_args,
   (hash)
   (signatures)
   (required_owner)
   (required_active)
   (required_posting)
   (required_other) )

FC_REFLECT( steem::plugins::database_api::verify_signatures_return,
   (valid) )

#ifdef STEEM_ENABLE_SMT
FC_REFLECT( steem::plugins::database_api::get_smt_next_identifier_return,
   (nais) )
#endif
