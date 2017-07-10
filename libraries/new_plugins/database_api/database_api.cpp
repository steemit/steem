#include <steemit/plugins/database_api/database_api.hpp>
#include <steemit/plugins/database_api/database_api_plugin.hpp>

#include <appbase/application.hpp>

#define CHECK_ARG_SIZE( s ) \
   FC_ASSERT( args.size() == s, "Expected #s argument(s), was ${n}", ("n", args.size()) );

namespace steemit { namespace plugins { namespace database_api {

database_api::database_api( steemit::chain::database& db, bool disable_get_block )
   : _api( new steemit::app::database_api( db, disable_get_block ) )
{
   appbase::app().get_plugin< plugins::api_register::api_register_plugin >().add_api(
      DATABASE_API_PLUGIN_NAME,
      {
         API_METHOD( get_trending_tags ),
         API_METHOD( get_state ),
         API_METHOD( get_active_witnesses ),
         API_METHOD( get_miner_queue ),
         API_METHOD( get_block_header ),
         API_METHOD( get_block ),
         API_METHOD( get_ops_in_block ),
         API_METHOD( get_config ),
         API_METHOD( get_dynamic_global_properties ),
         API_METHOD( get_chain_properties ),
         API_METHOD( get_current_median_history_price ),
         API_METHOD( get_feed_history ),
         API_METHOD( get_witness_schedule ),
         API_METHOD( get_hardfork_version ),
         API_METHOD( get_next_scheduled_hardfork ),
         API_METHOD( get_reward_fund ),
         API_METHOD( get_key_references ),
         API_METHOD( get_accounts ),
         API_METHOD( get_account_references ),
         API_METHOD( lookup_account_names ),
         API_METHOD( lookup_accounts ),
         API_METHOD( get_account_count ),
         API_METHOD( get_owner_history ),
         API_METHOD( get_recovery_request ),
         API_METHOD( get_escrow ),
         API_METHOD( get_withdraw_routes ),
         API_METHOD( get_account_bandwidth ),
         API_METHOD( get_savings_withdraw_from ),
         API_METHOD( get_savings_withdraw_to ),
         API_METHOD( get_vesting_delegations ),
         API_METHOD( get_expiring_vesting_delegations ),
         API_METHOD( get_witnesses ),
         API_METHOD( get_conversion_requests ),
         API_METHOD( get_witness_by_account ),
         API_METHOD( get_witnesses_by_vote ),
         API_METHOD( lookup_witness_accounts ),
         API_METHOD( get_witness_count ),
         API_METHOD( get_order_book ),
         API_METHOD( get_open_orders ),
         API_METHOD( get_liquidity_queue ),
         API_METHOD( get_transaction_hex ),
         API_METHOD( get_transaction ),
         API_METHOD( get_required_signatures ),
         API_METHOD( get_potential_signatures ),
         API_METHOD( verify_authority ),
         API_METHOD( verify_account_authority ),
         API_METHOD( get_active_votes ),
         API_METHOD( get_account_votes ),
         API_METHOD( get_content ),
         API_METHOD( get_content_replies ),
         API_METHOD( get_tags_used_by_author ),
         API_METHOD( get_discussions_by_payout ),
         API_METHOD( get_post_discussions_by_payout ),
         API_METHOD( get_comment_discussions_by_payout ),
         API_METHOD( get_discussions_by_trending ),
         API_METHOD( get_discussions_by_created ),
         API_METHOD( get_discussions_by_active ),
         API_METHOD( get_discussions_by_cashout ),
         API_METHOD( get_discussions_by_votes ),
         API_METHOD( get_discussions_by_children ),
         API_METHOD( get_discussions_by_hot ),
         API_METHOD( get_discussions_by_feed ),
         API_METHOD( get_discussions_by_blog ),
         API_METHOD( get_discussions_by_comments ),
         API_METHOD( get_discussions_by_promoted ),
         API_METHOD( get_replies_by_last_update ),
         API_METHOD( get_discussions_by_author_before_date ),
         API_METHOD( get_account_history )
      });
}

database_api::~database_api() {}

DEFINE_API( database_api, get_trending_tags )
{
   CHECK_ARG_SIZE( 2 )
   return _api->get_trending_tags( args[0].as< string >(), args[1].as< uint32_t >() );
}

DEFINE_API( database_api, get_state )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_state( args[0].as< string >() );
}

DEFINE_API( database_api, get_active_witnesses )
{
   return _api->get_active_witnesses();
}

DEFINE_API( database_api, get_miner_queue )
{
   return _api->get_miner_queue();
}

DEFINE_API( database_api, get_block_header )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_block_header( args[0].as< uint32_t >() );
}

DEFINE_API( database_api, get_block )
{
   CHECK_ARG_SIZE( 2 )
   return _api->get_block( args[0].as< uint32_t >() );
}

DEFINE_API( database_api, get_ops_in_block )
{
   CHECK_ARG_SIZE( 2 )
   return _api->get_ops_in_block( args[0].as< uint32_t >(), args[1].as< bool >() );
}

DEFINE_API( database_api, get_config )
{
   return _api->get_config();
}

DEFINE_API( database_api, get_dynamic_global_properties )
{
   return _api->get_dynamic_global_properties();
}

DEFINE_API( database_api, get_chain_properties )
{
   return _api->get_chain_properties();
}

DEFINE_API( database_api, get_current_median_history_price )
{
   return _api->get_current_median_history_price();
}

DEFINE_API( database_api, get_feed_history )
{
   return _api->get_feed_history();
}

DEFINE_API( database_api, get_witness_schedule )
{
   return _api->get_witness_schedule();
}

DEFINE_API( database_api, get_hardfork_version )
{
   return _api->get_hardfork_version();
}

DEFINE_API( database_api, get_next_scheduled_hardfork )
{
   return _api->get_next_scheduled_hardfork();
}

DEFINE_API( database_api, get_reward_fund )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_reward_fund( args[0].as< string >() );
}

DEFINE_API( database_api, get_key_references )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_key_references( args[0].as< vector< public_key_type > >() );
}

DEFINE_API( database_api, get_accounts )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_accounts( args[0].as< vector< string > >() );
}

DEFINE_API( database_api, get_account_references )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_account_references( args[0].as< account_id_type >() );
}

DEFINE_API( database_api, lookup_account_names )
{
   CHECK_ARG_SIZE( 1 )
   return _api->lookup_account_names( args[0].as< vector< string > >() );
}

DEFINE_API( database_api, lookup_accounts )
{
   CHECK_ARG_SIZE( 2 )
   return _api->lookup_accounts( args[0].as< string >(), args[1].as< uint32_t >() );
}

DEFINE_API( database_api, get_account_count )
{
   return _api->get_account_count();
}

DEFINE_API( database_api, get_owner_history )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_owner_history( args[0].as< string >() );
}

DEFINE_API( database_api, get_recovery_request )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_recovery_request( args[0].as< string >() );
}

DEFINE_API( database_api, get_escrow )
{
   CHECK_ARG_SIZE( 2 )
   return _api->get_escrow( args[0].as< string >(), args[1].as< uint32_t >() );
}

DEFINE_API( database_api, get_withdraw_routes )
{
   FC_ASSERT( args.size() == 1 || args.size() == 2, "Expected 1-2 arguments, was ${n}", ("n", args.size()) );
   return _api->get_withdraw_routes(
      args[0].as< string >(),
      args.size() == 2 ? args[1].as< withdraw_route_type >() : outgoing
   );
}

DEFINE_API( database_api, get_account_bandwidth )
{
   CHECK_ARG_SIZE( 2 )
   return _api->get_account_bandwidth( args[0].as< string >(), args[1].as< witness::bandwidth_type >() );
}

DEFINE_API( database_api, get_savings_withdraw_from )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_savings_withdraw_from( args[0].as< string >() );
}

DEFINE_API( database_api, get_savings_withdraw_to )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_savings_withdraw_to( args[0].as< string >() );
}

DEFINE_API( database_api, get_vesting_delegations )
{
   FC_ASSERT( args.size() == 2 || args.size() == 3, "Expected 2-3 arguments, was ${n}", ("n", args.size()) );
   return _api->get_vesting_delegations(
      args[0].as< string >(),
      args[1].as< string >(),
      args.size() == 3 ? args[2].as< uint32_t >() : 100
   );
}

DEFINE_API( database_api, get_expiring_vesting_delegations )
{
   FC_ASSERT( args.size() == 2 || args.size() == 3, "Expected 2-3 arguments, was ${n}", ("n", args.size()) );
   return _api->get_expiring_vesting_delegations(
      args[0].as< string >(),
      args[1].as< time_point_sec >(),
      args.size() == 3 ? args[2].as< uint32_t >() : 100
   );
}

DEFINE_API( database_api, get_witnesses )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_witnesses( args[0].as< vector< witness_id_type > >() );
}

DEFINE_API( database_api, get_conversion_requests )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_conversion_requests( args[0].as< string >() );
}

DEFINE_API( database_api, get_witness_by_account )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_witness_by_account( args[0].as< string >() );
}

DEFINE_API( database_api, get_witnesses_by_vote )
{
   CHECK_ARG_SIZE( 2 )
   return _api->get_witnesses_by_vote( args[0].as< string >(), args[1].as< uint32_t >() );
}

DEFINE_API( database_api, lookup_witness_accounts )
{
   CHECK_ARG_SIZE( 2 )
   return _api->lookup_witness_accounts( args[0].as< string >(), args[1].as< uint32_t >() );
}

DEFINE_API( database_api, get_witness_count )
{
   return _api->get_witness_count();
}

DEFINE_API( database_api, get_order_book )
{
   FC_ASSERT( args.size() == 0 || args.size() == 1, "Expected 0-1 arguments, was ${n}", ("n", args.size()) );
   return _api->get_order_book( args.size() == 1 ? args[0].as< uint32_t >() : 1000 );
}

DEFINE_API( database_api, get_open_orders )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_open_orders( args[0].as< string >() );
}

DEFINE_API( database_api, get_liquidity_queue )
{
   FC_ASSERT( args.size() == 1 || args.size() == 2, "Expected 1-2 arguments, was ${n}", ("n", args.size()) );
   return _api->get_liquidity_queue(
      args[0].as< string >(),
      args.size() == 2 ? args[1].as< uint32_t >() : 1000
   );
}

DEFINE_API( database_api, get_transaction_hex )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_transaction_hex( args[0].as< signed_transaction >() );
}

DEFINE_API( database_api, get_transaction )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_transaction( args[0].as< transaction_id_type >() );
}

DEFINE_API( database_api, get_required_signatures )
{
   CHECK_ARG_SIZE( 2 )
   return _api->get_required_signatures( args[0].as< signed_transaction >(), args[1].as< flat_set< public_key_type > >() );
}

DEFINE_API( database_api, get_potential_signatures )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_potential_signatures( args[0].as< signed_transaction >() );
}

DEFINE_API( database_api, verify_authority )
{
   CHECK_ARG_SIZE( 1 )
   return _api->verify_authority( args[0].as< signed_transaction >() );
}

DEFINE_API( database_api, verify_account_authority )
{
   CHECK_ARG_SIZE( 2 )
   return _api->verify_account_authority( args[0].as< string >(), args[1].as< flat_set< public_key_type > >() );
}

DEFINE_API( database_api, get_active_votes )
{
   CHECK_ARG_SIZE( 2 )
   return _api->get_active_votes( args[0].as< string >(), args[1].as< string >() );
}

DEFINE_API( database_api, get_account_votes )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_account_votes( args[0].as< string >() );
}

DEFINE_API( database_api, get_content )
{
   CHECK_ARG_SIZE( 2 )
   return _api->get_content( args[0].as< string >(), args[1].as< string >() );
}

DEFINE_API( database_api, get_content_replies )
{
   CHECK_ARG_SIZE( 2 )
   return _api->get_content_replies( args[0].as< string >(), args[1].as< string >() );
}

DEFINE_API( database_api, get_tags_used_by_author )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_tags_used_by_author( args[0].as< string >() );
}

DEFINE_API( database_api, get_discussions_by_payout )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_discussions_by_payout( args[0].as< discussion_query >() );
}

DEFINE_API( database_api, get_post_discussions_by_payout )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_post_discussions_by_payout( args[0].as< discussion_query >() );
}

DEFINE_API( database_api, get_comment_discussions_by_payout )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_comment_discussions_by_payout( args[0].as< discussion_query >() );
}

DEFINE_API( database_api, get_discussions_by_trending )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_discussions_by_trending( args[0].as< discussion_query >() );
}

DEFINE_API( database_api, get_discussions_by_created )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_discussions_by_created( args[0].as< discussion_query >() );
}

DEFINE_API( database_api, get_discussions_by_active )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_discussions_by_active( args[0].as< discussion_query >() );
}

DEFINE_API( database_api, get_discussions_by_cashout )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_discussions_by_cashout( args[0].as< discussion_query >() );
}

DEFINE_API( database_api, get_discussions_by_votes )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_discussions_by_votes( args[0].as< discussion_query >() );
}

DEFINE_API( database_api, get_discussions_by_children )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_discussions_by_children( args[0].as< discussion_query >() );
}

DEFINE_API( database_api, get_discussions_by_hot )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_discussions_by_hot( args[0].as< discussion_query >() );
}

DEFINE_API( database_api, get_discussions_by_feed )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_discussions_by_feed( args[0].as< discussion_query >() );
}

DEFINE_API( database_api, get_discussions_by_blog )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_discussions_by_blog( args[0].as< discussion_query >() );
}

DEFINE_API( database_api, get_discussions_by_comments )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_discussions_by_comments( args[0].as< discussion_query >() );
}

DEFINE_API( database_api, get_discussions_by_promoted )
{
   CHECK_ARG_SIZE( 1 )
   return _api->get_discussions_by_promoted( args[0].as< discussion_query >() );
}

DEFINE_API( database_api, get_replies_by_last_update )
{
   CHECK_ARG_SIZE( 3 )
   return _api->get_replies_by_last_update(
      args[0].as< account_name_type >(),
      args[1].as< string >(),
      args[2].as< uint32_t >()
   );
}

DEFINE_API( database_api, get_discussions_by_author_before_date )
{
   CHECK_ARG_SIZE( 4 )
   return _api->get_discussions_by_author_before_date(
      args[0].as< string >(),
      args[1].as< string >(),
      args[2].as< time_point_sec >(),
      args[3].as< uint32_t >()
   );
}

DEFINE_API( database_api, get_account_history )
{
   CHECK_ARG_SIZE( 3 )
   return _api->get_account_history(
      args[0].as< string >(),
      args[1].as< uint64_t >(),
      args[2].as< uint32_t >()
   );
}

} } } // steemit::plugins::database_api