#pragma once

#define SERIALIZED_HEADERS
#define OWNER_API
#define SERIALIZABLE_BOOST_CONTAINERS
#include <serialize3/h/client_code/serialize_macros.h>

#include <steem/chain/serialize_operators.hpp>

#include <steem/protocol/asset_symbol.hpp>
#include <steem/protocol/asset.hpp>
#include <steem/protocol/fixed_string.hpp>
#include <steem/protocol/version.hpp>
#include <steem/protocol/types.hpp>
#include <steem/protocol/steem_operations.hpp>

#include <chainbase/chainbase.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/steem_objects.hpp>
#include <steem/chain/account_object.hpp>
#include <steem/chain/block_summary_object.hpp>
#include <steem/chain/comment_object.hpp>
#include <steem/chain/global_property_object.hpp>
#include <steem/chain/hardfork_property_object.hpp>
//#include <steem/chain/node_property_object.hpp>
#include <steem/chain/history_object.hpp>
#include <steem/chain/transaction_object.hpp>
#include <steem/chain/witness_objects.hpp>
#include <steem/chain/shared_authority.hpp>
#include <steem/chain/smt_objects/smt_token_object.hpp>
#include <steem/chain/smt_objects/account_balance_object.hpp>

namespace chainbase
{

template <typename TMultiIndex>
using index_t = index< generic_index<TMultiIndex> >;

class dynamic_global_property_index_t : public index_t< steem::chain::dynamic_global_property_index > {};
class account_index_t : public index_t< steem::chain::account_index > {};
class account_authority_index_t : public index_t< steem::chain::account_authority_index > {};
class witness_index_t : public index_t< steem::chain::witness_index > {};
class transaction_index_t : public index_t< steem::chain::transaction_index > {};
class block_summary_index_t : public index_t< steem::chain::block_summary_index > {};
class witness_schedule_index_t : public index_t< steem::chain::witness_schedule_index > {};
class comment_index_t : public index_t< steem::chain::comment_index > {};
class comment_content_index_t : public index_t< steem::chain::comment_content_index > {};
class comment_vote_index_t : public index_t< steem::chain::comment_vote_index > {};
class witness_vote_index_t : public index_t< steem::chain::witness_vote_index > {};
class limit_order_index_t : public index_t< steem::chain::limit_order_index > {};
class feed_history_index_t : public index_t< steem::chain::feed_history_index > {};
class convert_request_index_t : public index_t< steem::chain::convert_request_index > {};
class liquidity_reward_balance_index_t : public index_t< steem::chain::liquidity_reward_balance_index > {};
class operation_index_t : public index_t< steem::chain::operation_index > {};
class account_history_index_t : public index_t< steem::chain::account_history_index > {};
class hardfork_property_index_t : public index_t< steem::chain::hardfork_property_index > {};
class withdraw_vesting_route_index_t : public index_t< steem::chain::withdraw_vesting_route_index > {};
class owner_authority_history_index_t : public index_t< steem::chain::owner_authority_history_index > {};
class account_recovery_request_index_t : public index_t< steem::chain::account_recovery_request_index > {};
class change_recovery_account_request_index_t : public index_t< steem::chain::change_recovery_account_request_index > {};
class escrow_index_t : public index_t< steem::chain::escrow_index > {};
class savings_withdraw_index_t : public index_t< steem::chain::savings_withdraw_index > {};
class decline_voting_rights_request_index_t : public index_t< steem::chain::decline_voting_rights_request_index > {};
class reward_fund_index_t : public index_t< steem::chain::reward_fund_index > {};
class vesting_delegation_index_t : public index_t< steem::chain::vesting_delegation_index > {};
class vesting_delegation_expiration_index_t : public index_t< steem::chain::vesting_delegation_expiration_index > {};
#ifdef STEEM_ENABLE_SMT
class smt_token_index_t : public index_t< steem::chain::smt_token_index > {};
class account_regular_balance_index_t : public index_t< steem::chain::account_regular_balance_index > {};
class account_rewards_balance_index_t : public index_t< steem::chain::account_rewards_balance_index > {};
#endif
   
} // namespace chainbase

using boost::interprocess::iset_index;
