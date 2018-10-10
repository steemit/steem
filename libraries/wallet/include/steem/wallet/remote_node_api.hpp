#pragma once
#include <steem/plugins/condenser_api/condenser_api.hpp>

namespace steem { namespace wallet {

using std::vector;
using fc::variant;
using fc::optional;

using namespace chain;
using namespace plugins;
/*using namespace plugins::condenser_api;
using namespace plugins::database_api;
using namespace plugins::account_history;
using namespace plugins::follow;
using namespace plugins::market_history;
using namespace plugins::witness;*/

/**
 * This is a dummy API so that the wallet can create properly formatted API calls
 */
struct remote_node_api
{
   condenser_api::get_version_return get_version();
   vector< condenser_api::api_tag_object > get_trending_tags( string, uint32_t );
   condenser_api::state get_state( string );
   vector< account_name_type > get_active_witnesses();
   optional< block_header > get_block_header( uint32_t );
   optional< condenser_api::legacy_signed_block > get_block( uint32_t );
   vector< condenser_api::api_operation_object > get_ops_in_block( uint32_t, bool only_virtual = true );
   fc::variant_object get_config();
   condenser_api::extended_dynamic_global_properties get_dynamic_global_properties();
   condenser_api::api_chain_properties get_chain_properties();
   condenser_api::legacy_price get_current_median_history_price();
   condenser_api::api_feed_history_object get_feed_history();
   condenser_api::api_witness_schedule_object get_witness_schedule();
   hardfork_version get_hardfork_version();
   condenser_api::scheduled_hardfork get_next_scheduled_hardfork();
   condenser_api::api_reward_fund_object get_reward_fund( string );
   vector< vector< account_name_type > > get_key_references( vector< public_key_type > );
   vector< condenser_api::extended_account > get_accounts( vector< account_name_type > );
   vector< account_id_type > get_account_references( account_id_type account_id );
   vector< optional< condenser_api::api_account_object > > lookup_account_names( vector< account_name_type > );
   vector< account_name_type > lookup_accounts( account_name_type, uint32_t );
   uint64_t get_account_count();
   vector< database_api::api_owner_authority_history_object > get_owner_history( account_name_type );
   optional< database_api::api_account_recovery_request_object > get_recovery_request( account_name_type );
   optional< condenser_api::api_escrow_object > get_escrow( account_name_type, uint32_t );
   vector< database_api::api_withdraw_vesting_route_object > get_withdraw_routes( account_name_type, condenser_api::withdraw_route_type );
   vector< condenser_api::api_savings_withdraw_object > get_savings_withdraw_from( account_name_type );
   vector< condenser_api::api_savings_withdraw_object > get_savings_withdraw_to( account_name_type );
   vector< condenser_api::api_vesting_delegation_object > get_vesting_delegations( account_name_type, account_name_type, uint32_t );
   vector< condenser_api::api_vesting_delegation_expiration_object > get_expiring_vesting_delegations( account_name_type, time_point_sec, uint32_t );
   vector< optional< condenser_api::api_witness_object > > get_witnesses( vector< witness_id_type > );
   vector< condenser_api::api_convert_request_object > get_conversion_requests( account_name_type );
   optional< condenser_api::api_witness_object > get_witness_by_account( account_name_type );
   vector< condenser_api::api_witness_object > get_witnesses_by_vote( account_name_type, uint32_t );
   vector< account_name_type > lookup_witness_accounts( string, uint32_t );
   uint64_t get_witness_count();
   vector< condenser_api::api_limit_order_object > get_open_orders( account_name_type );
   string get_transaction_hex( condenser_api::legacy_signed_transaction );
   condenser_api::legacy_signed_transaction get_transaction( transaction_id_type );
   set< public_key_type > get_required_signatures( condenser_api::legacy_signed_transaction, flat_set< public_key_type > );
   set< public_key_type > get_potential_signatures( condenser_api::legacy_signed_transaction );
   bool verify_authority( condenser_api::legacy_signed_transaction );
   bool verify_account_authority( string, flat_set< public_key_type > );
   vector< tags::vote_state > get_active_votes( account_name_type, string );
   vector< condenser_api::account_vote > get_account_votes( account_name_type );
   condenser_api::discussion get_content( account_name_type, string );
   vector< condenser_api::discussion > get_content_replies( account_name_type, string );
   vector< tags::tag_count_object > get_tags_used_by_author( account_name_type );
   vector< condenser_api::discussion > get_discussions_by_payout( tags::discussion_query );
   vector< condenser_api::discussion > get_post_discussions_by_payout( tags::discussion_query );
   vector< condenser_api::discussion > get_comment_discussions_by_payout( tags::discussion_query );
   vector< condenser_api::discussion > get_discussions_by_trending( tags::discussion_query );
   vector< condenser_api::discussion > get_discussions_by_created( tags::discussion_query );
   vector< condenser_api::discussion > get_discussions_by_active( tags::discussion_query );
   vector< condenser_api::discussion > get_discussions_by_cashout( tags::discussion_query );
   vector< condenser_api::discussion > get_discussions_by_votes( tags::discussion_query );
   vector< condenser_api::discussion > get_discussions_by_children( tags::discussion_query );
   vector< condenser_api::discussion > get_discussions_by_hot( tags::discussion_query );
   vector< condenser_api::discussion > get_discussions_by_feed( tags::discussion_query );
   vector< condenser_api::discussion > get_discussions_by_blog( tags::discussion_query );
   vector< condenser_api::discussion > get_discussions_by_comments( tags::discussion_query );
   vector< condenser_api::discussion > get_discussions_by_promoted( tags::discussion_query );
   vector< condenser_api::discussion > get_replies_by_last_update( tags::discussion_query );
   vector< condenser_api::discussion > get_discussions_by_author_before_date( tags::discussion_query );
   map< uint32_t, condenser_api::api_operation_object > get_account_history( account_name_type, uint64_t, uint32_t );
   void broadcast_transaction( condenser_api::legacy_signed_transaction );
   condenser_api::broadcast_transaction_synchronous_return broadcast_transaction_synchronous( condenser_api::legacy_signed_transaction );
   void broadcast_block( signed_block );
   vector< follow::api_follow_object > get_followers( account_name_type, account_name_type, follow::follow_type, uint32_t );
   vector< follow::api_follow_object > get_following( account_name_type, account_name_type, follow::follow_type, uint32_t );
   follow::get_follow_count_return get_follow_count( account_name_type );
   vector< follow::feed_entry > get_feed_entries( account_name_type, uint32_t, uint32_t );
   vector< follow::comment_feed_entry > get_feed( account_name_type, uint32_t, uint32_t );
   vector< follow::blog_entry > get_blog_entries( account_name_type, uint32_t, uint32_t );
   vector< follow::comment_blog_entry > get_blog( account_name_type, uint32_t, uint32_t );
   vector< follow::account_reputation > get_account_reputations( account_name_type, uint32_t );
   vector< account_name_type > get_reblogged_by( account_name_type, string );
   vector< follow::reblog_count > get_blog_authors( account_name_type );
   condenser_api::get_ticker_return get_ticker();
   condenser_api::get_volume_return get_volume();
   condenser_api::get_order_book_return get_order_book( uint32_t );
   vector< condenser_api::market_trade > get_trade_history( time_point_sec, time_point_sec, uint32_t );
   vector< condenser_api::market_trade > get_recent_trades( uint32_t );
   vector< market_history::bucket_object > get_market_history( uint32_t, time_point_sec, time_point_sec );
   flat_set< uint32_t > get_market_history_buckets();
};

} }

FC_API( steem::wallet::remote_node_api,
        (get_version)
        (get_trending_tags)
        (get_state)
        (get_active_witnesses)
        (get_block_header)
        (get_block)
        (get_ops_in_block)
        (get_config)
        (get_dynamic_global_properties)
        (get_chain_properties)
        (get_current_median_history_price)
        (get_feed_history)
        (get_witness_schedule)
        (get_hardfork_version)
        (get_next_scheduled_hardfork)
        (get_reward_fund)
        (get_key_references)
        (get_accounts)
        (get_account_references)
        (lookup_account_names)
        (lookup_accounts)
        (get_account_count)
        (get_owner_history)
        (get_recovery_request)
        (get_escrow)
        (get_withdraw_routes)
        (get_savings_withdraw_from)
        (get_savings_withdraw_to)
        (get_vesting_delegations)
        (get_expiring_vesting_delegations)
        (get_witnesses)
        (get_conversion_requests)
        (get_witness_by_account)
        (get_witnesses_by_vote)
        (lookup_witness_accounts)
        (get_witness_count)
        (get_open_orders)
        (get_transaction_hex)
        (get_transaction)
        (get_required_signatures)
        (get_potential_signatures)
        (verify_authority)
        (verify_account_authority)
        (get_active_votes)
        (get_account_votes)
        (get_content)
        (get_content_replies)
        (get_tags_used_by_author)
        (get_discussions_by_payout)
        (get_post_discussions_by_payout)
        (get_comment_discussions_by_payout)
        (get_discussions_by_trending)
        (get_discussions_by_created)
        (get_discussions_by_active)
        (get_discussions_by_cashout)
        (get_discussions_by_votes)
        (get_discussions_by_children)
        (get_discussions_by_hot)
        (get_discussions_by_feed)
        (get_discussions_by_blog)
        (get_discussions_by_comments)
        (get_discussions_by_promoted)
        (get_replies_by_last_update)
        (get_discussions_by_author_before_date)
        (get_account_history)
        (broadcast_transaction)
        (broadcast_transaction_synchronous)
        (broadcast_block)
        (get_followers)
        (get_following)
        (get_follow_count)
        (get_feed_entries)
        (get_feed)
        (get_blog_entries)
        (get_blog)
        (get_account_reputations)
        (get_reblogged_by)
        (get_blog_authors)
        (get_ticker)
        (get_volume)
        (get_order_book)
        (get_trade_history)
        (get_recent_trades)
        (get_market_history)
        (get_market_history_buckets)
      )
