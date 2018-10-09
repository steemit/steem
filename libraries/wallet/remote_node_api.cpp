#include<steem/wallet/remote_node_api.hpp>

namespace steem { namespace wallet{

// This class exists only to provide method signature information to fc::api, not to execute calls.

condenser_api::get_version_return remote_node_api::get_version()
{
   FC_ASSERT( false );
}

vector< condenser_api::api_tag_object > remote_node_api::get_trending_tags( string, uint32_t )
{
   FC_ASSERT( false );
}

condenser_api::state remote_node_api::get_state( string )
{
   FC_ASSERT( false );
}

vector< account_name_type > remote_node_api::get_active_witnesses()
{
   FC_ASSERT( false );
}

optional< block_header > remote_node_api::get_block_header( uint32_t )
{
   FC_ASSERT( false );
}

optional< condenser_api::legacy_signed_block > remote_node_api::get_block( uint32_t )
{
   FC_ASSERT( false );
}

vector< condenser_api::api_operation_object > remote_node_api::get_ops_in_block( uint32_t, bool only_virtual )
{
   FC_ASSERT( false );
}

fc::variant_object remote_node_api::get_config()
{
   FC_ASSERT( false );
}

condenser_api::extended_dynamic_global_properties remote_node_api::get_dynamic_global_properties()
{
   FC_ASSERT( false );
}

condenser_api::api_chain_properties remote_node_api::get_chain_properties()
{
   FC_ASSERT( false );
}

condenser_api::legacy_price remote_node_api::get_current_median_history_price()
{
   FC_ASSERT( false );
}

condenser_api::api_feed_history_object remote_node_api::get_feed_history()
{
   FC_ASSERT( false );
}

condenser_api::api_witness_schedule_object remote_node_api::get_witness_schedule()
{
   FC_ASSERT( false );
}

hardfork_version remote_node_api::get_hardfork_version()
{
   FC_ASSERT( false );
}

condenser_api::scheduled_hardfork remote_node_api::get_next_scheduled_hardfork()
{
   FC_ASSERT( false );
}

condenser_api::api_reward_fund_object remote_node_api::get_reward_fund( string )
{
   FC_ASSERT( false );
}

vector< vector< account_name_type > > remote_node_api::get_key_references( vector< public_key_type > )
{
   FC_ASSERT( false );
}

vector< condenser_api::extended_account > remote_node_api::get_accounts( vector< account_name_type > )
{
   FC_ASSERT( false );
}

vector< account_id_type > remote_node_api::get_account_references( account_id_type account_id )
{
   FC_ASSERT( false );
}

vector< optional< condenser_api::api_account_object > > remote_node_api::lookup_account_names( vector< account_name_type > )
{
   FC_ASSERT( false );
}

vector< account_name_type > remote_node_api::lookup_accounts( account_name_type, uint32_t )
{
   FC_ASSERT( false );
}

uint64_t remote_node_api::get_account_count()
{
   FC_ASSERT( false );
}

vector< database_api::api_owner_authority_history_object > remote_node_api::get_owner_history( account_name_type )
{
   FC_ASSERT( false );
}

optional< database_api::api_account_recovery_request_object > remote_node_api::get_recovery_request( account_name_type )
{
   FC_ASSERT( false );
}

optional< condenser_api::api_escrow_object > remote_node_api::get_escrow( account_name_type, uint32_t )
{
   FC_ASSERT( false );
}

vector< database_api::api_withdraw_vesting_route_object > remote_node_api::get_withdraw_routes( account_name_type, condenser_api::withdraw_route_type )
{
   FC_ASSERT( false );
}

vector< condenser_api::api_savings_withdraw_object > remote_node_api::get_savings_withdraw_from( account_name_type )
{
   FC_ASSERT( false );
}

vector< condenser_api::api_savings_withdraw_object > remote_node_api::get_savings_withdraw_to( account_name_type )
{
   FC_ASSERT( false );
}

vector< condenser_api::api_vesting_delegation_object > remote_node_api::get_vesting_delegations( account_name_type, account_name_type, uint32_t )
{
   FC_ASSERT( false );
}

vector< condenser_api::api_vesting_delegation_expiration_object > remote_node_api::get_expiring_vesting_delegations( account_name_type, time_point_sec, uint32_t )
{
   FC_ASSERT( false );
}

vector< optional< condenser_api::api_witness_object > > remote_node_api::get_witnesses( vector< witness_id_type > )
{
   FC_ASSERT( false );
}

vector< condenser_api::api_convert_request_object > remote_node_api::get_conversion_requests( account_name_type )
{
   FC_ASSERT( false );
}

optional< condenser_api::api_witness_object > remote_node_api::get_witness_by_account( account_name_type )
{
   FC_ASSERT( false );
}

vector< condenser_api::api_witness_object > remote_node_api::get_witnesses_by_vote( account_name_type, uint32_t )
{
   FC_ASSERT( false );
}

vector< account_name_type > remote_node_api::lookup_witness_accounts( string, uint32_t )
{
   FC_ASSERT( false );
}

uint64_t remote_node_api::get_witness_count()
{
   FC_ASSERT( false );
}

vector< condenser_api::api_limit_order_object > remote_node_api::get_open_orders( account_name_type )
{
   FC_ASSERT( false );
}

string remote_node_api::get_transaction_hex( condenser_api::legacy_signed_transaction )
{
   FC_ASSERT( false );
}

condenser_api::legacy_signed_transaction remote_node_api::get_transaction( transaction_id_type )
{
   FC_ASSERT( false );
}

set< public_key_type > remote_node_api::get_required_signatures( condenser_api::legacy_signed_transaction, flat_set< public_key_type > )
{
   FC_ASSERT( false );
}

set< public_key_type > remote_node_api::get_potential_signatures( condenser_api::legacy_signed_transaction )
{
   FC_ASSERT( false );
}

bool remote_node_api::verify_authority( condenser_api::legacy_signed_transaction )
{
   FC_ASSERT( false );
}

bool remote_node_api::verify_account_authority( string, flat_set< public_key_type > )
{
   FC_ASSERT( false );
}

vector< tags::vote_state > remote_node_api::get_active_votes( account_name_type, string )
{
   FC_ASSERT( false );
}

vector< condenser_api::account_vote > remote_node_api::get_account_votes( account_name_type )
{
   FC_ASSERT( false );
}

condenser_api::discussion remote_node_api::get_content( account_name_type, string )
{
   FC_ASSERT( false );
}

vector< condenser_api::discussion > remote_node_api::get_content_replies( account_name_type, string )
{
   FC_ASSERT( false );
}

vector< tags::tag_count_object > remote_node_api::get_tags_used_by_author( account_name_type )
{
   FC_ASSERT( false );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_payout( tags::discussion_query )
{
   FC_ASSERT( false );
}

vector< condenser_api::discussion > remote_node_api::get_post_discussions_by_payout( tags::discussion_query )
{
   FC_ASSERT( false );
}

vector< condenser_api::discussion > remote_node_api::get_comment_discussions_by_payout( tags::discussion_query )
{
   FC_ASSERT( false );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_trending( tags::discussion_query )
{
   FC_ASSERT( false );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_created( tags::discussion_query )
{
   FC_ASSERT( false );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_active( tags::discussion_query )
{
   FC_ASSERT( false );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_cashout( tags::discussion_query )
{
   FC_ASSERT( false );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_votes( tags::discussion_query )
{
   FC_ASSERT( false );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_children( tags::discussion_query )
{
   FC_ASSERT( false );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_hot( tags::discussion_query )
{
   FC_ASSERT( false );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_feed( tags::discussion_query )
{
   FC_ASSERT( false );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_blog( tags::discussion_query )
{
   FC_ASSERT( false );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_comments( tags::discussion_query )
{
   FC_ASSERT( false );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_promoted( tags::discussion_query )
{
   FC_ASSERT( false );
}

vector< condenser_api::discussion > remote_node_api::get_replies_by_last_update( tags::discussion_query )
{
   FC_ASSERT( false );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_author_before_date( tags::discussion_query )
{
   FC_ASSERT( false );
}

map< uint32_t, condenser_api::api_operation_object > remote_node_api::get_account_history( account_name_type, uint64_t, uint32_t )
{
   FC_ASSERT( false );
}

void remote_node_api::broadcast_transaction( condenser_api::legacy_signed_transaction )
{
   FC_ASSERT( false );
}

condenser_api::broadcast_transaction_synchronous_return remote_node_api::broadcast_transaction_synchronous( condenser_api::legacy_signed_transaction )
{
   FC_ASSERT( false );
}

void remote_node_api::broadcast_block( signed_block )
{
   FC_ASSERT( false );
}

vector< follow::api_follow_object > remote_node_api::get_followers( account_name_type, account_name_type, follow::follow_type, uint32_t )
{
   FC_ASSERT( false );
}

vector< follow::api_follow_object > remote_node_api::get_following( account_name_type, account_name_type, follow::follow_type, uint32_t )
{
   FC_ASSERT( false );
}

follow::get_follow_count_return remote_node_api::get_follow_count( account_name_type )
{
   FC_ASSERT( false );
}

vector< follow::feed_entry > remote_node_api::get_feed_entries( account_name_type, uint32_t, uint32_t )
{
   FC_ASSERT( false );
}

vector< follow::comment_feed_entry > remote_node_api::get_feed( account_name_type, uint32_t, uint32_t )
{
   FC_ASSERT( false );
}

vector< follow::blog_entry > remote_node_api::get_blog_entries( account_name_type, uint32_t, uint32_t )
{
   FC_ASSERT( false );
}

vector< follow::comment_blog_entry > remote_node_api::get_blog( account_name_type, uint32_t, uint32_t )
{
   FC_ASSERT( false );
}

vector< follow::account_reputation > remote_node_api::get_account_reputations( account_name_type, uint32_t )
{
   FC_ASSERT( false );
}

vector< account_name_type > remote_node_api::get_reblogged_by( account_name_type, string )
{
   FC_ASSERT( false );
}

vector< follow::reblog_count > remote_node_api::get_blog_authors( account_name_type )
{
   FC_ASSERT( false );
}

condenser_api::get_ticker_return remote_node_api::get_ticker()
{
   FC_ASSERT( false );
}

condenser_api::get_volume_return remote_node_api::get_volume()
{
   FC_ASSERT( false );
}

condenser_api::get_order_book_return remote_node_api::get_order_book( uint32_t )
{
   FC_ASSERT( false );
}

vector< condenser_api::market_trade > remote_node_api::get_trade_history( time_point_sec, time_point_sec, uint32_t )
{
   FC_ASSERT( false );
}

vector< condenser_api::market_trade > remote_node_api::get_recent_trades( uint32_t )
{
   FC_ASSERT( false );
}

vector< market_history::bucket_object > remote_node_api::get_market_history( uint32_t, time_point_sec, time_point_sec )
{
   FC_ASSERT( false );
}

flat_set< uint32_t > remote_node_api::get_market_history_buckets()
{
   FC_ASSERT( false );
}

} }
