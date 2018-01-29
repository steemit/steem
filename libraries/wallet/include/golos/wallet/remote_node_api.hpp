#pragma once

#include <golos/plugins/database_api/plugin.hpp>
#include <golos/plugins/database_api/forward.hpp>
#include <golos/plugins/database_api/state.hpp>
#include <fc/api.hpp>
#include <golos/plugins/network_broadcast_api/network_broadcast_api_plugin.hpp>
#include <plugins/account_history/include/golos/plugins/account_history/plugin.hpp>

#include <golos/plugins/social_network/api_object/tag_api_object.hpp>
#include <golos/plugins/social_network/api_object/discussion.hpp>
#include <golos/plugins/social_network/api_object/discussion_query.hpp>
#include <golos/plugins/social_network/api_object/account_vote.hpp>
#include <golos/plugins/social_network/api_object/vote_state.hpp>

#include <golos/plugins/market_history/market_history_objects.hpp>
#include <golos/plugins/follow/plugin.hpp>
#include <golos/plugins/follow/follow_api_object.hpp>


namespace golos { namespace wallet {

using std::vector;
using fc::variant;
using fc::optional;

using namespace chain;
using namespace plugins;
//using namespace plugins::condenser_api;
using namespace plugins::database_api;
using namespace plugins::account_history;
using namespace plugins::follow;
using namespace plugins::social_network;
using namespace plugins::market_history;
using namespace plugins::social_network;
//using namespace plugins::witness_plugin;

/**
 * This is a dummy API so that the wallet can create properly formatted API calls
 */
struct remote_node_api {
   //database_api::get_version_return get_version();
   vector< tag_api_object > get_trending_tags( string, uint32_t );
   vector< account_name_type > get_active_witnesses();
   optional< block_header > get_block_header( uint32_t );
   optional< database_api::signed_block > get_block( uint32_t );
   vector< operation_api_object > get_ops_in_block( uint32_t, bool only_virtual = true );
   fc::variant_object get_config();
   database_api::dynamic_global_property_object get_dynamic_global_properties();
   chain_properties get_chain_properties();
   price get_current_median_history_price();
   database_api::feed_history_api_object get_feed_history();
   database_api::witness_schedule_api_object get_witness_schedule();
   hardfork_version get_hardfork_version();
   database_api::scheduled_hardfork get_next_scheduled_hardfork();
    // TODO This method is from tolstoy_api
   //database_api::reward_fund_apit_object get_reward_fund( string );
   vector< vector< account_name_type > > get_key_references( vector< public_key_type > );
   vector< database_api::extended_account > get_accounts( vector< account_name_type > );
   vector< account_id_type > get_account_references( account_id_type account_id );
   vector< optional< database_api::account_api_object > > lookup_account_names( vector< account_name_type > );
   vector< account_name_type > lookup_accounts( account_name_type, uint32_t );
   uint64_t get_account_count();
   vector< database_api::owner_authority_history_api_object > get_owner_history( account_name_type );
   optional< database_api::account_recovery_request_api_object > get_recovery_request( account_name_type );
   optional< database_api::escrow_api_object > get_escrow( account_name_type, uint32_t );
   vector< database_api::withdraw_vesting_route_api_object > get_withdraw_routes( account_name_type, database_api::withdraw_route_type );
   optional< account_bandwidth_api_object > get_account_bandwidth( account_name_type, bandwidth_type);
   vector< database_api::savings_withdraw_api_object > get_savings_withdraw_from( account_name_type );
   vector< database_api::savings_withdraw_api_object > get_savings_withdraw_to( account_name_type );
   vector< optional< database_api::witness_api_object > > get_witnesses( vector< witness_id_type > );
   vector< database_api::convert_request_api_object > get_conversion_requests( account_name_type );
   optional< database_api::witness_api_object > get_witness_by_account( account_name_type );
   vector< database_api::witness_api_object > get_witnesses_by_vote( account_name_type, uint32_t );
   vector< account_name_type > lookup_witness_accounts( string, uint32_t );
   uint64_t get_witness_count();
   vector< database_api::extended_limit_order > get_open_orders( account_name_type );
   string get_transaction_hex( signed_transaction );
   annotated_signed_transaction get_transaction( transaction_id_type );
   set< public_key_type > get_required_signatures( signed_transaction, flat_set< public_key_type > );
   set< public_key_type > get_potential_signatures( signed_transaction );
   bool verify_authority( signed_transaction );
   bool verify_account_authority( string, flat_set< public_key_type > );
   void broadcast_transaction( signed_transaction );
   plugins::network_broadcast_api::broadcast_transaction_synchronous_return broadcast_transaction_synchronous( signed_transaction );
   void broadcast_block( signed_block );

   vector< vote_state > get_active_votes( account_name_type, string );
   vector< social_network::account_vote > get_account_votes( account_name_type );

   vector< tag_count_object > get_tags_used_by_author( account_name_type );
   discussion get_content( account_name_type, string );
   vector< discussion > get_content_replies( account_name_type, string );

   vector< discussion > get_discussions_by_payout( discussion_query );
   vector< discussion > get_post_discussions_by_payout( discussion_query );
   vector< discussion > get_comment_discussions_by_payout( discussion_query );
   vector< discussion > get_discussions_by_trending( discussion_query );
   vector< discussion > get_discussions_by_created( discussion_query );
   vector< discussion > get_discussions_by_active( discussion_query );
   vector< discussion > get_discussions_by_cashout( discussion_query );
   vector< discussion > get_discussions_by_votes( discussion_query );
   vector< discussion > get_discussions_by_children( discussion_query );
   vector< discussion > get_discussions_by_hot( discussion_query );
   vector< discussion > get_discussions_by_feed( discussion_query );
   vector< discussion > get_discussions_by_blog( discussion_query );
   vector< discussion > get_discussions_by_comments( discussion_query );
   vector< discussion > get_discussions_by_promoted( discussion_query );
   vector< discussion > get_replies_by_last_update( discussion_query );
   vector< discussion > get_discussions_by_author_before_date( discussion_query );

    // Follow plugin
   vector< follow_api_object > get_followers( account_name_type, account_name_type, follow_type, uint32_t );
   vector< follow_api_object > get_following( account_name_type, account_name_type, follow_type, uint32_t );
   get_follow_count_return get_follow_count( account_name_type );
   vector< feed_entry > get_feed_entries( account_name_type, uint32_t, uint32_t );
   vector< comment_feed_entry > get_feed( account_name_type, uint32_t, uint32_t );
   vector< blog_entry > get_blog_entries( account_name_type, uint32_t, uint32_t );
   vector< comment_blog_entry > get_blog( account_name_type, uint32_t, uint32_t );
   vector< account_reputation > get_account_reputations( account_name_type, uint32_t );
   vector< account_name_type > get_reblogged_by( account_name_type, string );
   vector< reblog_count > get_blog_authors( account_name_type );

    // Market history
   market_ticker_r get_ticker();
   market_volume_r get_volume();
   order_book_r get_order_book( order_book_a );
   trade_history_r get_trade_history( trade_history_a );
   recent_trades_r get_recent_trades( recent_trades_a );
   market_history_r get_market_history( market_history_a );
   market_history_buckets_r get_market_history_buckets();

   map<uint32_t, operation_api_object> get_account_history( account_name_type, uint64_t, uint32_t );

   vector< account_name_type > get_miner_queue();
};

} }

FC_API( golos::wallet::remote_node_api,
        //(get_version)
        (get_trending_tags)
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
        //(get_reward_fund)
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
        (get_account_bandwidth)
        (get_savings_withdraw_from)
        (get_savings_withdraw_to)
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
        (broadcast_transaction)
        (broadcast_transaction_synchronous)
        (get_tags_used_by_author)
        (get_active_votes)
        (get_account_votes)
        (get_content)
        (get_content_replies)

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


        (get_account_history)/*
        (broadcast_block)
        */
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
        (get_miner_queue)
      )
