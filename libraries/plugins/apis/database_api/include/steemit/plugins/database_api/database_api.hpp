#pragma once
#include <steemit/protocol/types.hpp>
#include <steemit/app/database_api.hpp>

#include <steemit/plugins/json_rpc/utility.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>

namespace steemit { namespace plugins { namespace database_api {

using namespace steemit::app;

using std::vector;
using fc::variant;
using fc::optional;
using steemit::plugins::json_rpc::void_type;

typedef vector< pair< string, uint32_t > > get_tags_used_by_author_return_type;
typedef map< uint32_t, applied_operation > get_account_history_return_type;

typedef void_type get_active_witnesses_args;

struct get_active_witnesses_return
{
   vector< account_name_type > witnesses;
};

typedef void_type get_miner_queue_args;

struct get_miner_queue_return
{
   vector< account_name_type > miners;
};

struct get_block_header_args
{
   uint32_t block_num;
};

struct get_block_header_return
{
   optional< block_header > header;
};

typedef get_block_header_args get_block_args;

struct get_block_return
{
   optional< signed_block_api_obj > block;
}

struct get_ops_in_block_args
{
   uint32_t block_num;
   bool     only_virtual;
};

struct get_ops_in_block_return
{
   vector< applied_operation > ops;
};

DEFINE_API_ARGS( get_config,                             void_type,           fc::variant_object )
DEFINE_API_ARGS( get_dynamic_global_properties,          void_type,           dynamic_global_property_api_obj )
DEFINE_API_ARGS( get_chain_properties,                   void_type,           chain_properties )
DEFINE_API_ARGS( get_current_median_history_price,       void_type,           price )
DEFINE_API_ARGS( get_feed_history,                       void_type,           feed_history_api_obj )
DEFINE_API_ARGS( get_witness_schedule,                   void_type,           witness_schedule_api_obj )

typedef void_type get_hardfork_version_args

struct get_hardfork_version_return
{
   hardfork_version hf_version;
};

DEFINE_API_ARGS( get_next_scheduled_hardfork,            void_type,           scheduled_hardfork )
DEFINE_API_ARGS( get_reward_fund,                        vector< variant >,   reward_fund_api_obj )

struct lookup_account_names_args
{
   vector< account_name_type > account_names;
};

struct lookup_account_names_return
{
   vector< optional< account_api_obj > > accounts;
};

struct lookup_accounts_args
{
   account_name_type lower_bound;
   uint32_t          limit = 1000;
};

struct lookup_accounts_return
{
   vector< account_name_type > account_names;
}

typedef void_type get_account_count_args;

struct get_account_count_return
{
   uint64_t count;
};

struct get_owner_history_args
{
   account_name_type account;
};

struct get_owner_history_return
{
   vector< owner_authority_history_api_obj > owner_history;
};

typedef get_owner_history_args get_recovery_request_args;

struct get_recovery_request_return
{
   optional< account_recovery_request_api_obj > request;
};

struct get_escrow_args
{
   account_name_type from;
   uint32_t          escrow_id;
};

struct get_escrow_return
{
   optional< escrow_api_obj > escrow;
};

struct get_withdraw_routes_args
{
   account_name_type    account;
   withdraw_route_type  type = outgoing;
};

struct get_withdraw_routes_return
{
   vector< withdraw_route > routes;
};

typedef get_owner_history_args get_savings_withdraw_from_args;

struct get_savings_withdraw_from_return
{
   vector< savings_withdraw_api_obj > withdrawals;
};

typedef get_owner_history_args get_savings_withdraw_to_args;

typedef get_savings_withdraw_from_return get_savings_withdraw_to_return;

struct get_vesting_delegations_args
{
   account_name_type account;
   account_name_type from;
   uint32_t          limit = 100;
};

struct get_vesting_delegations_return
{
   vector< vesting_delegation_api_obj > delegations;
};

struct get_expiring_vesting_delegations_args
{
   vector< vesting_delegation_expiration_api_obj > delegations;
};

struct get_witnesses_args
{
   vector< witness_id_type > witness_ids;
};

struct get_witnesses_return
{
   vector< optional< witness_api_obj > > witnesses;
};

struct get_conversion_requests_args
{
   account_name_type account;
};

struct get_conversion_requests_return
{
   vector< convert_request_api_obj > conversions;
};

struct get_wtness_by_account_args
{
   account_name_type account;
};

struct get_witness_by_account_return
{
   optional< witness_api_obj > witness;
};

struct get_witnesses_by_vote_args
{
   account_name_type from;
   uint32_t          limit = 100;
};

struct get_witnesses_by_vote_return
{
   vector< witness_api_obj > witnesses;
};

struct lookup_witness_accounts_args
{
   account_name_type from;
   uint32_t          limit = 100;
};

struct lookup_witness_accounts_return
{
   
};


DEFINE_API_ARGS( lookup_witness_accounts,                vector< variant >,   set< account_name_type > )
DEFINE_API_ARGS( get_order_book,                         vector< variant >,   order_book )
DEFINE_API_ARGS( get_open_orders,                        vector< variant >,   vector< extended_limit_order > )
DEFINE_API_ARGS( get_liquidity_queue,                    vector< variant >,   vector< liquidity_balance > )
DEFINE_API_ARGS( get_witness_count,                      void_type,           uint64_t )
DEFINE_API_ARGS( get_transaction_hex,                    vector< variant >,   string )
DEFINE_API_ARGS( get_transaction,                        vector< variant >,   annotated_signed_transaction )
DEFINE_API_ARGS( get_required_signatures,                vector< variant >,   set< public_key_type > )
DEFINE_API_ARGS( get_potential_signatures,               vector< variant >,   set< public_key_type > )
DEFINE_API_ARGS( verify_authority,                       vector< variant >,   bool )
DEFINE_API_ARGS( verify_account_authority,               vector< variant >,   bool )
DEFINE_API_ARGS( get_active_votes,                       vector< variant >,   vector< vote_state > )
DEFINE_API_ARGS( get_account_votes,                      vector< variant >,   vector< account_vote > )
DEFINE_API_ARGS( get_content,                            vector< variant >,   discussion )
DEFINE_API_ARGS( get_content_replies,                    vector< variant >,   vector< discussion > )
DEFINE_API_ARGS( get_tags_used_by_author,                vector< variant >,   get_tags_used_by_author_return_type )
DEFINE_API_ARGS( get_discussions_by_payout,              vector< variant >,   vector< discussion > )
DEFINE_API_ARGS( get_post_discussions_by_payout,         vector< variant >,   vector< discussion > )
DEFINE_API_ARGS( get_comment_discussions_by_payout,      vector< variant >,   vector< discussion > )
DEFINE_API_ARGS( get_discussions_by_trending,            vector< variant >,   vector< discussion > )
DEFINE_API_ARGS( get_discussions_by_created,             vector< variant >,   vector< discussion > )
DEFINE_API_ARGS( get_discussions_by_active,              vector< variant >,   vector< discussion > )
DEFINE_API_ARGS( get_discussions_by_cashout,             vector< variant >,   vector< discussion > )
DEFINE_API_ARGS( get_discussions_by_votes,               vector< variant >,   vector< discussion > )
DEFINE_API_ARGS( get_discussions_by_children,            vector< variant >,   vector< discussion > )
DEFINE_API_ARGS( get_discussions_by_hot,                 vector< variant >,   vector< discussion > )
DEFINE_API_ARGS( get_discussions_by_feed,                vector< variant >,   vector< discussion > )
DEFINE_API_ARGS( get_discussions_by_blog,                vector< variant >,   vector< discussion > )
DEFINE_API_ARGS( get_discussions_by_comments,            vector< variant >,   vector< discussion > )
DEFINE_API_ARGS( get_discussions_by_promoted,            vector< variant >,   vector< discussion > )
DEFINE_API_ARGS( get_replies_by_last_update,             vector< variant >,   vector< discussion > )
DEFINE_API_ARGS( get_discussions_by_author_before_date,  vector< variant >,   vector< discussion > )
DEFINE_API_ARGS( get_account_history,                    vector< variant >,   get_account_history_return_type )

class database_api
{
public:
   database_api( steemit::chain::database& db, bool disable_get_block );
   ~database_api();

   DECLARE_API( get_trending_tags )
   DECLARE_API( get_state )
   DECLARE_API( get_active_witnesses )
   DECLARE_API( get_miner_queue )
   DECLARE_API( get_block_header )
   DECLARE_API( get_block )
   DECLARE_API( get_ops_in_block )
   DECLARE_API( get_config )
   DECLARE_API( get_dynamic_global_properties )
   DECLARE_API( get_chain_properties )
   DECLARE_API( get_current_median_history_price )
   DECLARE_API( get_feed_history )
   DECLARE_API( get_witness_schedule )
   DECLARE_API( get_hardfork_version )
   DECLARE_API( get_next_scheduled_hardfork )
   DECLARE_API( get_reward_fund )
   DECLARE_API( get_key_references )
   DECLARE_API( get_accounts )
   DECLARE_API( get_account_references )
   DECLARE_API( lookup_account_names )
   DECLARE_API( lookup_accounts )
   DECLARE_API( get_account_count )
   DECLARE_API( get_owner_history )
   DECLARE_API( get_recovery_request )
   DECLARE_API( get_escrow )
   DECLARE_API( get_withdraw_routes )
   DECLARE_API( get_account_bandwidth )
   DECLARE_API( get_savings_withdraw_from )
   DECLARE_API( get_savings_withdraw_to )
   DECLARE_API( get_vesting_delegations )
   DECLARE_API( get_expiring_vesting_delegations )
   DECLARE_API( get_witnesses )
   DECLARE_API( get_conversion_requests )
   DECLARE_API( get_witness_by_account )
   DECLARE_API( get_witnesses_by_vote )
   DECLARE_API( lookup_witness_accounts )
   DECLARE_API( get_witness_count )
   DECLARE_API( get_order_book )
   DECLARE_API( get_open_orders )
   DECLARE_API( get_liquidity_queue )
   DECLARE_API( get_transaction_hex )
   DECLARE_API( get_transaction )
   DECLARE_API( get_required_signatures )
   DECLARE_API( get_potential_signatures )
   DECLARE_API( verify_authority )
   DECLARE_API( verify_account_authority )
   DECLARE_API( get_active_votes )
   DECLARE_API( get_account_votes )
   DECLARE_API( get_content )
   DECLARE_API( get_content_replies )
   DECLARE_API( get_tags_used_by_author )
   DECLARE_API( get_discussions_by_payout )
   DECLARE_API( get_post_discussions_by_payout )
   DECLARE_API( get_comment_discussions_by_payout )
   DECLARE_API( get_discussions_by_trending )
   DECLARE_API( get_discussions_by_created )
   DECLARE_API( get_discussions_by_active )
   DECLARE_API( get_discussions_by_cashout )
   DECLARE_API( get_discussions_by_votes )
   DECLARE_API( get_discussions_by_children )
   DECLARE_API( get_discussions_by_hot )
   DECLARE_API( get_discussions_by_feed )
   DECLARE_API( get_discussions_by_blog )
   DECLARE_API( get_discussions_by_comments )
   DECLARE_API( get_discussions_by_promoted )
   DECLARE_API( get_replies_by_last_update )
   DECLARE_API( get_discussions_by_author_before_date )
   DECLARE_API( get_account_history )

   private:
      std::unique_ptr< steemit::app::database_api > _api;
};

} } } // steemit::plugins::database_api