#include<golos/wallet/remote_node_api.hpp>

namespace golos { namespace wallet{

// This class exists only to provide method signature information to fc::api, not to execute calls.

//condenser_api::get_version_return remote_node_api::get_version() {
//   FC_ASSERT( false );
//}

vector< tag_api_object > remote_node_api::get_trending_tags( string, uint32_t ) {
   FC_ASSERT( false );
}

vector< account_name_type > remote_node_api::get_active_witnesses() {
   FC_ASSERT( false );
}

optional< block_header > remote_node_api::get_block_header( uint32_t ) {
   FC_ASSERT( false );
}

optional< database_api::signed_block > remote_node_api::get_block( uint32_t ) {
   FC_ASSERT( false );
}

vector< operation_api_object > remote_node_api::get_ops_in_block( uint32_t, bool only_virtual ) {
   FC_ASSERT( false );
}

fc::variant_object remote_node_api::get_config() {
   FC_ASSERT( false );
}

database_api::dynamic_global_property_object remote_node_api::get_dynamic_global_properties() {
   FC_ASSERT( false );
}

chain_properties remote_node_api::get_chain_properties() {
   FC_ASSERT( false );
}

price remote_node_api::get_current_median_history_price() {
   FC_ASSERT( false );
}

database_api::feed_history_api_object remote_node_api::get_feed_history() {
   FC_ASSERT( false );
}

database_api::witness_schedule_api_object remote_node_api::get_witness_schedule() {
   FC_ASSERT( false );
}

hardfork_version remote_node_api::get_hardfork_version()
{
   FC_ASSERT( false );
}

database_api::scheduled_hardfork remote_node_api::get_next_scheduled_hardfork() {
   FC_ASSERT( false );
}

//database_api::api_reward_fund_object remote_node_api::get_reward_fund( string )
//{
//   FC_ASSERT( false );
//}

vector< vector< account_name_type > > remote_node_api::get_key_references( vector< public_key_type > )
{
   FC_ASSERT( false );
}

vector< database_api::extended_account > remote_node_api::get_accounts( vector< account_name_type > )
{
   FC_ASSERT( false );
}

vector< account_id_type > remote_node_api::get_account_references( account_id_type account_id )
{
   FC_ASSERT( false );
}

vector< optional< database_api::account_api_object > > remote_node_api::lookup_account_names( vector< account_name_type > )
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

vector< database_api::owner_authority_history_api_object > remote_node_api::get_owner_history( account_name_type )
{
   FC_ASSERT( false );
}

optional< database_api::account_recovery_request_api_object > remote_node_api::get_recovery_request( account_name_type )
{
   FC_ASSERT( false );
}

optional< database_api::escrow_api_object > remote_node_api::get_escrow( account_name_type, uint32_t )
{
   FC_ASSERT( false );
}

vector< database_api::withdraw_vesting_route_api_object > remote_node_api::get_withdraw_routes( account_name_type, database_api::withdraw_route_type )
{
   FC_ASSERT( false );
}

optional<account_bandwidth_api_object> remote_node_api::get_account_bandwidth( account_name_type, bandwidth_type ) {
   FC_ASSERT( false );
}

vector< database_api::savings_withdraw_api_object > remote_node_api::get_savings_withdraw_from( account_name_type )
{
   FC_ASSERT( false );
}

vector< database_api::savings_withdraw_api_object > remote_node_api::get_savings_withdraw_to( account_name_type ) {
   FC_ASSERT( false );
}

vector< optional< database_api::witness_api_object > > remote_node_api::get_witnesses( vector< witness_id_type > ) {
   FC_ASSERT( false );
}

vector< database_api::convert_request_api_object > remote_node_api::get_conversion_requests( account_name_type ) {
   FC_ASSERT( false );
}

optional< database_api::witness_api_object > remote_node_api::get_witness_by_account( account_name_type )
{
   FC_ASSERT( false );
}

vector< database_api::witness_api_object > remote_node_api::get_witnesses_by_vote( account_name_type, uint32_t )
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

vector< database_api::extended_limit_order > remote_node_api::get_open_orders( account_name_type )
{
   FC_ASSERT( false );
}

string remote_node_api::get_transaction_hex( signed_transaction )
{
   FC_ASSERT( false );
}

annotated_signed_transaction remote_node_api::get_transaction( transaction_id_type )
{
   FC_ASSERT( false );
}

set< public_key_type > remote_node_api::get_required_signatures( signed_transaction, flat_set< public_key_type > )
{
   FC_ASSERT( false );
}

set< public_key_type > remote_node_api::get_potential_signatures( signed_transaction )
{
   FC_ASSERT( false );
}

bool remote_node_api::verify_authority( signed_transaction )
{
   FC_ASSERT( false );
}

bool remote_node_api::verify_account_authority( string, flat_set< public_key_type > ) {
   FC_ASSERT( false );
}

        void remote_node_api::broadcast_transaction( signed_transaction ) {
           FC_ASSERT( false );
        }

        network_broadcast_api::broadcast_transaction_synchronous_return remote_node_api::broadcast_transaction_synchronous( signed_transaction ) {
           FC_ASSERT( false );
        }

        void remote_node_api::broadcast_block( signed_block ) {
           FC_ASSERT( false );
        }

vector< vote_state > remote_node_api::get_active_votes( account_name_type, string ) {
   FC_ASSERT( false );
}

vector< account_vote > remote_node_api::get_account_votes( account_name_type ) {
   FC_ASSERT( false );
}

vector< tag_count_object > remote_node_api::get_tags_used_by_author( account_name_type )
{
    FC_ASSERT( false );
}

discussion remote_node_api::get_content( account_name_type, string ) {
   FC_ASSERT( false );
}

vector< discussion > remote_node_api::get_content_replies( account_name_type, string )
{
   FC_ASSERT( false );
}

vector< discussion > remote_node_api::get_discussions_by_payout( discussion_query )
{
   FC_ASSERT( false );
}

vector< discussion > remote_node_api::get_post_discussions_by_payout( discussion_query )
{
   FC_ASSERT( false );
}

vector< discussion > remote_node_api::get_comment_discussions_by_payout( discussion_query )
{
   FC_ASSERT( false );
}

vector< discussion > remote_node_api::get_discussions_by_trending( discussion_query )
{
   FC_ASSERT( false );
}

vector< discussion > remote_node_api::get_discussions_by_created( discussion_query )
{
   FC_ASSERT( false );
}

vector< discussion > remote_node_api::get_discussions_by_active( discussion_query )
{
   FC_ASSERT( false );
}

vector< discussion > remote_node_api::get_discussions_by_cashout( discussion_query )
{
   FC_ASSERT( false );
}

vector< discussion > remote_node_api::get_discussions_by_votes( discussion_query )
{
   FC_ASSERT( false );
}

vector< discussion > remote_node_api::get_discussions_by_children( discussion_query )
{
   FC_ASSERT( false );
}

vector< discussion > remote_node_api::get_discussions_by_hot( discussion_query )
{
   FC_ASSERT( false );
}

vector< discussion > remote_node_api::get_discussions_by_feed( discussion_query )
{
   FC_ASSERT( false );
}

vector< discussion > remote_node_api::get_discussions_by_blog( discussion_query )
{
   FC_ASSERT( false );
}

vector< discussion > remote_node_api::get_discussions_by_comments( discussion_query )
{
   FC_ASSERT( false );
}

vector< discussion > remote_node_api::get_discussions_by_promoted( discussion_query )
{
   FC_ASSERT( false );
}

vector< discussion > remote_node_api::get_replies_by_last_update( discussion_query )
{
   FC_ASSERT( false );
}

vector< discussion > remote_node_api::get_discussions_by_author_before_date( discussion_query )
{
   FC_ASSERT( false );
}

map< uint32_t, golos::plugins::database_api::operation_api_object > remote_node_api::get_account_history( account_name_type, uint64_t, uint32_t ) {
   FC_ASSERT( false );
}



vector< follow::follow_api_object > remote_node_api::get_followers( account_name_type, account_name_type, follow::follow_type, uint32_t ) {
   FC_ASSERT( false );
}

vector< follow::follow_api_object > remote_node_api::get_following( account_name_type, account_name_type, follow::follow_type, uint32_t ) {
   FC_ASSERT( false );
}

follow::get_follow_count_return remote_node_api::get_follow_count( account_name_type ) {
   FC_ASSERT( false );
}

vector< follow::feed_entry > remote_node_api::get_feed_entries( account_name_type, uint32_t, uint32_t ) {
   FC_ASSERT( false );
}

vector< follow::comment_feed_entry > remote_node_api::get_feed( account_name_type, uint32_t, uint32_t ) {
   FC_ASSERT( false );
}

vector< follow::blog_entry > remote_node_api::get_blog_entries( account_name_type, uint32_t, uint32_t ) {
   FC_ASSERT( false );
}

vector< follow::comment_blog_entry > remote_node_api::get_blog( account_name_type, uint32_t, uint32_t ) {
   FC_ASSERT( false );
}

vector< follow::account_reputation > remote_node_api::get_account_reputations( account_name_type, uint32_t ) {
   FC_ASSERT( false );
}

vector< account_name_type > remote_node_api::get_reblogged_by( account_name_type, string ) {
   FC_ASSERT( false );
}

vector< follow::reblog_count > remote_node_api::get_blog_authors( account_name_type ) {
   FC_ASSERT( false );
}

market_ticker_r remote_node_api::get_ticker() {
   FC_ASSERT( false );
}

market_volume_r remote_node_api::get_volume() {
   FC_ASSERT( false );
}

order_book_r remote_node_api::get_order_book( order_book_a ) {
   FC_ASSERT( false );
}

trade_history_r remote_node_api::get_trade_history( trade_history_a ) {
   FC_ASSERT( false );
}

recent_trades_r remote_node_api::get_recent_trades( recent_trades_a ) {
   FC_ASSERT( false );
}

market_history_r remote_node_api::get_market_history( market_history_a ) {
   FC_ASSERT( false );
}

market_history_buckets_r remote_node_api::get_market_history_buckets() {
   FC_ASSERT( false );
}

vector<account_name_type> remote_node_api::get_miner_queue() {
  FC_ASSERT( false );
}

} }
