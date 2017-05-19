#pragma once
#include <steemit/protocol/types.hpp>

namespace steemit { namespace db_api {

using protocol::account_name_type;
using protocol::signed_transaction;
using protocol::transaction_id_type;
using protocol::public_key_type;

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
   by_voter_last_update,
   by_comment_weight_voter,
   by_price
};

struct get_block_header_params
{
   uint32_t block_num;
};

struct get_block_params
{
   uint32_t block_num;
};

struct get_ops_in_block_params
{
   uint32_t block_num;
   bool     only_virtual;
};

struct list_witnesses_params
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct find_witnesses_params
{
   vector< account_name_type > owners;
};

struct list_witness_votes_params
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};


/* Account */

struct list_accounts_params
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct find_accounts_params
{
   vector< account_name_type > accounts;
};


/* Owner Auth History */

struct list_owner_histories_params
{
   fc::variant       start;
   uint32_t          limit;
};

struct find_owner_histories_params
{
   account_name_type owner;
};


/* Account Recovery Requests */

struct list_account_recovery_requests_params
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct find_account_recovery_requests_params
{
   vector< account_name_type > accounts;
};


/* Change Recovery Account Requests */

struct list_change_recovery_account_requests_params
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct find_change_recovery_account_requests_params
{
   vector< account_name_type > accounts;
};


/* Escrow */

struct list_escrows_params
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct find_escrows_params
{
   account_name_type from;
};

struct list_withdraw_vesting_routes_params
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct find_withdraw_vesting_routes_params
{
   account_name_type account;
   sort_order_type   order;
};

struct list_savings_withdrawals_params
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct find_savings_withdrawals_params
{
   account_name_type account;
};

struct list_vesting_delegations_params
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct find_vesting_delegations_params
{
   account_name_type account;
};

struct list_vesting_delegation_expirations_params
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct find_vesting_delegation_expirations_params
{
   account_name_type account;
};

struct list_sbd_conversion_requests_params
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct find_sbd_conversion_requests_params
{
   account_name_type account;
};

struct list_decline_voting_rights_requests_params
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct find_decline_voting_rights_requests_params
{
   vector< account_name_type > accounts;
};

struct list_comments_params
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct find_comments_params
{
   vector< std::pair< account_name_type, string > > comments;
};

struct list_votes_params
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct find_votes_params
{
   account_name_type author;
   string            permlink;
};

struct list_limit_orders_params
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct find_limit_orders_params
{
   account_name_type account;
};

struct get_order_book_params
{
   uint32_t          limit;
};

struct get_transaction_hex_params
{
   signed_transaction trx;
};

struct get_transaction_params
{
   transaction_id_type id;
};

struct get_required_signatures_params
{
   signed_transaction          trx;
   flat_set< public_key_type > available_keys;
};

struct get_potential_signatures_params
{
   signed_transaction trx;
};

struct verify_authority_params
{
   signed_transaction trx;
};

struct verify_account_authority_params
{
   account_name_type           account;
   flat_set< public_key_type > signers;
};

} } // steemit::db_api

FC_REFLECT_ENUM( steemit::db_api::sort_order_type,
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
   (by_voter_last_update)
   (by_comment_weight_voter)
   (by_price) )

FC_REFLECT( steemit::db_api::get_block_header_params,
   (block_num) )

FC_REFLECT( steemit::db_api::get_block_params,
   (block_num) )

FC_REFLECT( steemit::db_api::get_ops_in_block_params,
   (block_num)
   (only_virtual) )

FC_REFLECT( steemit::db_api::list_witnesses_params,
   (start)
   (limit)
   (order) )

FC_REFLECT( steemit::db_api::find_witnesses_params,
   (owners) )

FC_REFLECT( steemit::db_api::list_witness_votes_params,
   (start)
   (limit)
   (order) )

FC_REFLECT( steemit::db_api::list_accounts_params,
   (start)
   (limit)
   (order) )

FC_REFLECT( steemit::db_api::find_accounts_params,
   (accounts) )

FC_REFLECT( steemit::db_api::list_owner_histories_params,
   (start)
   (limit) )

FC_REFLECT( steemit::db_api::find_owner_histories_params,
   (owner) )

FC_REFLECT( steemit::db_api::list_account_recovery_requests_params,
   (start)
   (limit)
   (order) )

FC_REFLECT( steemit::db_api::find_account_recovery_requests_params,
   (accounts) )

FC_REFLECT( steemit::db_api::list_change_recovery_account_requests_params,
   (start)
   (limit)
   (order) )

FC_REFLECT( steemit::db_api::find_change_recovery_account_requests_params,
   (accounts) )

FC_REFLECT( steemit::db_api::list_escrows_params,
   (start)
   (limit)
   (order) )

FC_REFLECT( steemit::db_api::find_escrows_params,
   (from) )

FC_REFLECT( steemit::db_api::list_withdraw_vesting_routes_params,
   (start)
   (limit)
   (order) )

FC_REFLECT( steemit::db_api::find_withdraw_vesting_routes_params,
   (account)
   (order) )

FC_REFLECT( steemit::db_api::list_savings_withdrawals_params,
   (start)
   (limit)
   (order) )

FC_REFLECT( steemit::db_api::find_savings_withdrawals_params,
   (account) )

FC_REFLECT( steemit::db_api::list_vesting_delegations_params,
   (start)
   (limit)
   (order) )

FC_REFLECT( steemit::db_api::find_vesting_delegations_params,
   (account) )

FC_REFLECT( steemit::db_api::list_vesting_delegation_expirations_params,
   (start)
   (limit)
   (order) )

FC_REFLECT( steemit::db_api::find_vesting_delegation_expirations_params,
   (account) )

FC_REFLECT( steemit::db_api::list_sbd_conversion_requests_params,
   (start)
   (limit)
   (order) )

FC_REFLECT( steemit::db_api::find_sbd_conversion_requests_params,
   (account) )

FC_REFLECT( steemit::db_api::list_decline_voting_rights_requests_params,
   (start)
   (limit)
   (order) )

FC_REFLECT( steemit::db_api::find_decline_voting_rights_requests_params,
   (accounts) )

FC_REFLECT( steemit::db_api::list_comments_params,
   (start)
   (limit)
   (order) )

FC_REFLECT( steemit::db_api::find_comments_params,
   (comments) )

FC_REFLECT( steemit::db_api::list_votes_params,
   (start)
   (limit)
   (order) )

FC_REFLECT( steemit::db_api::find_votes_params,
   (author)
   (permlink) )

FC_REFLECT( steemit::db_api::list_limit_orders_params,
   (start)
   (limit)
   (order) )

FC_REFLECT( steemit::db_api::find_limit_orders_params,
   (account) )

FC_REFLECT( steemit::db_api::get_order_book_params,
   (limit) )

FC_REFLECT( steemit::db_api::get_transaction_hex_params,
   (trx) )

FC_REFLECT( steemit::db_api::get_transaction_params,
   (id) )

FC_REFLECT( steemit::db_api::get_required_signatures_params,
   (trx)
   (available_keys) )

FC_REFLECT( steemit::db_api::get_potential_signatures_params,
   (trx) )

FC_REFLECT( steemit::db_api::verify_authority_params,
   (trx) )

FC_REFLECT( steemit::db_api::verify_account_authority_params,
   (account)
   (signers) )
