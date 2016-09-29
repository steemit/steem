#pragma once
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include <graphene/db2/database.hpp>

#include <steemit/protocol/types.hpp>


namespace steemit { namespace chain {

namespace bip = graphene::db2::bip;
using namespace boost::multi_index;

using boost::multi_index_container;

using graphene::db2::object;
using graphene::db2::allocator;

using steemit::protocol::block_id_type;
using steemit::protocol::transaction_id_type;
using steemit::protocol::chain_id_type;
using steemit::protocol::account_name_type;
using steemit::protocol::share_type;
using steemit::protocol::shared_authority;

typedef bip::basic_string<
   char,std::char_traits<char>,
   bip::allocator<char,bip::managed_mapped_file::segment_manager>
> shared_string;

typedef bip::allocator<shared_string,bip::managed_mapped_file::segment_manager> shared_string_allocator_type;

struct by_id;

enum object_type
{
   dynamic_global_property_object_type,
   account_object_type,
   witness_object_type,
   transaction_object_type,
   block_summary_object_type,
   chain_property_object_type,
   witness_schedule_object_type,
   comment_object_type,
   comment_stats_object_type,
   comment_vote_object_type,
   vote_object_type,
   witness_vote_object_type,
   limit_order_object_type,
   feed_history_object_type,
   convert_request_object_type,
   liquidity_reward_balance_object_type,
   operation_object_type,
   account_history_object_type,
   category_object_type,
   hardfork_property_object_type,
   withdraw_vesting_route_object_type,
   owner_authority_history_object_type,
   account_recovery_request_object_type,
   change_recovery_account_request_object_type,
   escrow_object_type,
   savings_withdraw_object_type,
   decline_voting_rights_request_object_type
};

} } //steemit::chain

FC_REFLECT_ENUM( steemit::chain::object_type,
                 (dynamic_global_property_object_type)
                 (account_object_type)
                 (witness_object_type)
                 (transaction_object_type)
                 (block_summary_object_type)
                 (chain_property_object_type)
                 (witness_schedule_object_type)
                 (comment_object_type)
                 (comment_stats_object_type)
                 (category_object_type)
                 (comment_vote_object_type)
                 (vote_object_type)
                 (witness_vote_object_type)
                 (limit_order_object_type)
                 (feed_history_object_type)
                 (convert_request_object_type)
                 (liquidity_reward_balance_object_type)
                 (operation_object_type)
                 (account_history_object_type)
                 (hardfork_property_object_type)
                 (withdraw_vesting_route_object_type)
                 (owner_authority_history_object_type)
                 (account_recovery_request_object_type)
                 (change_recovery_account_request_object_type)
                 (escrow_object_type)
                 (savings_withdraw_object_type)
                 (decline_voting_rights_request_object_type)
               )
