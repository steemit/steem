#pragma once
#include <steemit/protocol/types.hpp>

namespace steemit { namespace chain {

using graphene::db::object_id_type;
using graphene::db::object_id;
using graphene::db::abstract_object;
using graphene::db::undo_database;

using steemit::protocol::block_id_type;
using steemit::protocol::transaction_id_type;
using steemit::protocol::chain_id_type;
using steemit::protocol::account_name_type;
using steemit::protocol::share_type;

enum reserved_spaces
{
   relative_protocol_ids = 0,
   protocol_ids          = 1,
   implementation_ids    = 2
};

inline bool is_relative( object_id_type o ){ return o.space() == 0; }

enum object_type
{
   null_object_type,
   base_object_type,
   OBJECT_TYPE_COUNT ///< Sentry value which contains the number of different object types
};

enum impl_object_type
{
   impl_dynamic_global_property_object_type,
   impl_reserved0_object_type,      // formerly index_meta_object_type, TODO: delete me
   impl_account_object_type,
   impl_witness_object_type,
   impl_transaction_object_type,
   impl_block_summary_object_type,
   impl_chain_property_object_type,
   impl_witness_schedule_object_type,
   impl_comment_object_type,
   impl_comment_stats_object_type,
   impl_comment_vote_object_type,
   impl_vote_object_type,
   impl_witness_vote_object_type,
   impl_limit_order_object_type,
   impl_feed_history_object_type,
   impl_convert_request_object_type,
   impl_liquidity_reward_balance_object_type,
   impl_operation_object_type,
   impl_account_history_object_type,
   impl_category_object_type,
   impl_hardfork_property_object_type,
   impl_withdraw_vesting_route_object_type,
   impl_owner_authority_history_object_type,
   impl_account_recovery_request_object_type,
   impl_change_recovery_account_request_object_type,
   impl_escrow_object_type,
   impl_savings_withdraw_object_type,
   impl_decline_voting_rights_request_object_type
};

class operation_object;
class account_history_object;
class comment_object;
class category_object;
class comment_vote_object;
class comment_stats_object;
class vote_object;
class witness_vote_object;
class account_object;
class feed_history_object;
class limit_order_object;
class convert_request_object;
class dynamic_global_property_object;
class transaction_object;
class block_summary_object;
class chain_property_object;
class witness_schedule_object;
class account_object;
class witness_object;
class liquidity_reward_balance_object;
class hardfork_property_object;
class withdraw_vesting_route_object;
class owner_authority_history_object;
class account_recovery_request_object;
class change_recovery_account_request_object;
class escrow_object;
class savings_withdraw_object;
class decline_voting_rights_request_object;


typedef object_id< implementation_ids, impl_operation_object_type,                        operation_object >                        operation_id_type;
typedef object_id< implementation_ids, impl_account_history_object_type,                  account_history_object >                  account_history_id_type;
typedef object_id< implementation_ids, impl_comment_object_type,                          comment_object >                          comment_id_type;
typedef object_id< implementation_ids, impl_comment_stats_object_type,                    comment_stats_object >                    comment_stats_id_type;
typedef object_id< implementation_ids, impl_category_object_type,                         category_object >                         category_id_type;
typedef object_id< implementation_ids, impl_comment_vote_object_type,                     comment_vote_object >                     comment_vote_id_type;
typedef object_id< implementation_ids, impl_vote_object_type,                             vote_object >                             vote_id_type;
typedef object_id< implementation_ids, impl_witness_vote_object_type,                     witness_vote_object >                     witness_vote_id_type;
typedef object_id< implementation_ids, impl_limit_order_object_type,                      limit_order_object >                      limit_order_id_type;
typedef object_id< implementation_ids, impl_feed_history_object_type,                     feed_history_object >                     feed_history_id_type;
typedef object_id< implementation_ids, impl_convert_request_object_type,                  convert_request_object >                  convert_request_id_type;
typedef object_id< implementation_ids, impl_account_object_type,                          account_object >                          account_id_type;
typedef object_id< implementation_ids, impl_witness_object_type,                          witness_object >                          witness_id_type;
typedef object_id< implementation_ids, impl_dynamic_global_property_object_type,          dynamic_global_property_object >          dynamic_global_property_id_type;
typedef object_id< implementation_ids, impl_transaction_object_type,                      transaction_object >                      transaction_obj_id_type;
typedef object_id< implementation_ids, impl_block_summary_object_type,                    block_summary_object >                    block_summary_id_type;
typedef object_id< implementation_ids, impl_chain_property_object_type,                   chain_property_object >                   chain_property_id_type;
typedef object_id< implementation_ids, impl_witness_schedule_object_type,                 witness_schedule_object >                 witness_schedule_id_type;
typedef object_id< implementation_ids, impl_liquidity_reward_balance_object_type,         liquidity_reward_balance_object >         liquidity_reward_balance_id_type;
typedef object_id< implementation_ids, impl_hardfork_property_object_type,                hardfork_property_object >                hardfork_property_id_type;
typedef object_id< implementation_ids, impl_withdraw_vesting_route_object_type,           withdraw_vesting_route_object >           withdraw_vesting_route_id_type;
typedef object_id< implementation_ids, impl_owner_authority_history_object_type,          owner_authority_history_object >          owner_authority_history_id_type;
typedef object_id< implementation_ids, impl_account_recovery_request_object_type,         account_recovery_request_object >         account_recovery_request_id_type;
typedef object_id< implementation_ids, impl_change_recovery_account_request_object_type,  change_recovery_account_request_object >  change_recovery_account_request_id_type;
typedef object_id< implementation_ids, impl_escrow_object_type,                           escrow_object >                           escrow_id_type;
typedef object_id< implementation_ids, impl_savings_withdraw_object_type,                 savings_withdraw_object>                  savings_withdraw_id_type;
typedef object_id< implementation_ids, impl_decline_voting_rights_request_object_type,    decline_voting_rights_request_object >    decline_voting_rights_request_id_type;

} } //steemit::chain

FC_REFLECT_ENUM( steemit::chain::object_type,
                 (null_object_type)
                 (base_object_type)
                 (OBJECT_TYPE_COUNT)
               )
FC_REFLECT_ENUM( steemit::chain::impl_object_type,
                 (impl_dynamic_global_property_object_type)
                 (impl_reserved0_object_type)
                 (impl_account_object_type)
                 (impl_witness_object_type)
                 (impl_transaction_object_type)
                 (impl_block_summary_object_type)
                 (impl_chain_property_object_type)
                 (impl_witness_schedule_object_type)
                 (impl_comment_object_type)
                 (impl_comment_stats_object_type)
                 (impl_category_object_type)
                 (impl_comment_vote_object_type)
                 (impl_vote_object_type)
                 (impl_witness_vote_object_type)
                 (impl_limit_order_object_type)
                 (impl_feed_history_object_type)
                 (impl_convert_request_object_type)
                 (impl_liquidity_reward_balance_object_type)
                 (impl_operation_object_type)
                 (impl_account_history_object_type)
                 (impl_hardfork_property_object_type)
                 (impl_withdraw_vesting_route_object_type)
                 (impl_owner_authority_history_object_type)
                 (impl_account_recovery_request_object_type)
                 (impl_change_recovery_account_request_object_type)
                 (impl_escrow_object_type)
                 (impl_savings_withdraw_object_type)
                 (impl_decline_voting_rights_request_object_type)
               )

FC_REFLECT_TYPENAME( steemit::chain::operation_id_type )
FC_REFLECT_TYPENAME( steemit::chain::account_history_id_type )
FC_REFLECT_TYPENAME( steemit::chain::comment_id_type )
FC_REFLECT_TYPENAME( steemit::chain::comment_stats_id_type )
FC_REFLECT_TYPENAME( steemit::chain::category_id_type )
FC_REFLECT_TYPENAME( steemit::chain::comment_vote_id_type )
FC_REFLECT_TYPENAME( steemit::chain::vote_id_type )
FC_REFLECT_TYPENAME( steemit::chain::witness_vote_id_type )
FC_REFLECT_TYPENAME( steemit::chain::limit_order_id_type )
FC_REFLECT_TYPENAME( steemit::chain::feed_history_id_type )
FC_REFLECT_TYPENAME( steemit::chain::convert_request_id_type)
FC_REFLECT_TYPENAME( steemit::chain::account_id_type )
FC_REFLECT_TYPENAME( steemit::chain::witness_id_type )
FC_REFLECT_TYPENAME( steemit::chain::dynamic_global_property_id_type )
FC_REFLECT_TYPENAME( steemit::chain::transaction_obj_id_type )
FC_REFLECT_TYPENAME( steemit::chain::block_summary_id_type )
FC_REFLECT_TYPENAME( steemit::chain::chain_property_id_type )
FC_REFLECT_TYPENAME( steemit::chain::witness_schedule_id_type )
FC_REFLECT_TYPENAME( steemit::chain::liquidity_reward_balance_id_type )
FC_REFLECT_TYPENAME( steemit::chain::hardfork_property_id_type )
FC_REFLECT_TYPENAME( steemit::chain::withdraw_vesting_route_id_type )
FC_REFLECT_TYPENAME( steemit::chain::owner_authority_history_id_type )
FC_REFLECT_TYPENAME( steemit::chain::account_recovery_request_id_type )
FC_REFLECT_TYPENAME( steemit::chain::change_recovery_account_request_id_type )
FC_REFLECT_TYPENAME( steemit::chain::escrow_id_type )
FC_REFLECT_TYPENAME( steemit::chain::savings_withdraw_id_type )
FC_REFLECT_TYPENAME( steemit::chain::decline_voting_rights_request_id_type )
