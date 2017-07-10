#pragma once
#include <steemit/protocol/types.hpp>
#include <steemit/app/database_api.hpp>

#include <steemit/plugins/api_register/utility.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>

namespace steemit { namespace plugins { namespace database_api {

using namespace steemit::app;

using std::vector;
using fc::variant;
using fc::optional;
using steemit::plugins::api_register::void_args;

typedef vector< pair< string, uint32_t > > get_tags_used_by_author_return_type;
typedef map< uint32_t, applied_operation > get_account_history_return_type;

/*               API,                                    args,                return */
DEFINE_API_ARGS( get_trending_tags,                      vector< variant >,   vector< tag_api_obj > )
DEFINE_API_ARGS( get_state,                              vector< variant >,   state )
DEFINE_API_ARGS( get_active_witnesses,                   void_args,           vector< account_name_type > )
DEFINE_API_ARGS( get_miner_queue,                        void_args,           vector< account_name_type > )
DEFINE_API_ARGS( get_block_header,                       vector< variant >,   optional< block_header > )
DEFINE_API_ARGS( get_block,                              vector< variant >,   optional< signed_block_api_obj > )
DEFINE_API_ARGS( get_ops_in_block,                       vector< variant >,   vector< applied_operation > )
DEFINE_API_ARGS( get_config,                             void_args,           fc::variant_object )
DEFINE_API_ARGS( get_dynamic_global_properties,          void_args,           dynamic_global_property_api_obj )
DEFINE_API_ARGS( get_chain_properties,                   void_args,           chain_properties )
DEFINE_API_ARGS( get_current_median_history_price,       void_args,           price )
DEFINE_API_ARGS( get_feed_history,                       void_args,           feed_history_api_obj )
DEFINE_API_ARGS( get_witness_schedule,                   void_args,           witness_schedule_api_obj )
DEFINE_API_ARGS( get_hardfork_version,                   void_args,           hardfork_version )
DEFINE_API_ARGS( get_next_scheduled_hardfork,            void_args,           scheduled_hardfork )
DEFINE_API_ARGS( get_reward_fund,                        vector< variant >,   reward_fund_api_obj )
DEFINE_API_ARGS( get_key_references,                     vector< variant >,   vector< set< string > > )
DEFINE_API_ARGS( get_accounts,                           vector< variant >,   vector< extended_account > )
DEFINE_API_ARGS( get_account_references,                 vector< variant >,   vector< account_id_type > )
DEFINE_API_ARGS( lookup_account_names,                   vector< variant >,   vector< optional< account_api_obj > > )
DEFINE_API_ARGS( lookup_accounts,                        vector< variant >,   set< string > )
DEFINE_API_ARGS( get_account_count,                      void_args,           uint64_t )
DEFINE_API_ARGS( get_owner_history,                      vector< variant >,   vector< owner_authority_history_api_obj > )
DEFINE_API_ARGS( get_recovery_request,                   vector< variant >,   optional< account_recovery_request_api_obj > )
DEFINE_API_ARGS( get_escrow,                             vector< variant >,   optional< escrow_api_obj > )
DEFINE_API_ARGS( get_withdraw_routes,                    vector< variant >,   vector< withdraw_route > )
DEFINE_API_ARGS( get_account_bandwidth,                  vector< variant >,   optional< account_bandwidth_api_obj > )
DEFINE_API_ARGS( get_savings_withdraw_from,              vector< variant >,   vector< savings_withdraw_api_obj > )
DEFINE_API_ARGS( get_savings_withdraw_to,                vector< variant >,   vector< savings_withdraw_api_obj > )
DEFINE_API_ARGS( get_vesting_delegations,                vector< variant >,   vector< vesting_delegation_api_obj > )
DEFINE_API_ARGS( get_expiring_vesting_delegations,       vector< variant >,   vector< vesting_delegation_expiration_api_obj > )
DEFINE_API_ARGS( get_witnesses,                          vector< variant >,   vector< optional< witness_api_obj > > )
DEFINE_API_ARGS( get_conversion_requests,                vector< variant >,   vector< convert_request_api_obj > )
DEFINE_API_ARGS( get_witness_by_account,                 vector< variant >,   optional< witness_api_obj > )
DEFINE_API_ARGS( get_witnesses_by_vote,                  vector< variant >,   vector< witness_api_obj > )
DEFINE_API_ARGS( lookup_witness_accounts,                vector< variant >,   set< account_name_type > )
DEFINE_API_ARGS( get_order_book,                         vector< variant >,   order_book )
DEFINE_API_ARGS( get_open_orders,                        vector< variant >,   vector< extended_limit_order > )
DEFINE_API_ARGS( get_liquidity_queue,                    vector< variant >,   vector< liquidity_balance > )
DEFINE_API_ARGS( get_witness_count,                      void_args,           uint64_t )
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