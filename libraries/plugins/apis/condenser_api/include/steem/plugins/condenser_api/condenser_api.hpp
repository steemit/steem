#pragma once

#include <steem/plugins/database_api/database_api.hpp>
#include <steem/plugins/block_api/block_api.hpp>
#include <steem/plugins/account_history_api/account_history_api.hpp>
#include <steem/plugins/account_by_key_api/account_by_key_api.hpp>
#include <steem/plugins/network_broadcast_api/network_broadcast_api.hpp>
#include <steem/plugins/tags_api/tags_api.hpp>
#include <steem/plugins/follow_api/follow_api.hpp>
#include <steem/plugins/market_history_api/market_history_api.hpp>
#include <steem/plugins/witness_api/witness_api.hpp>

#include <steem/plugins/condenser_api/condenser_api_legacy_operations.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>
#include <fc/api.hpp>

namespace steem { namespace plugins { namespace condenser_api {

using std::vector;
using fc::variant;
using fc::optional;

using namespace chain;

namespace detail{ class condenser_api_impl; }

struct discussion_index
{
   string           category;         /// category by which everything is filtered
   vector< string > trending;         /// trending posts over the last 24 hours
   vector< string > payout;           /// pending posts by payout
   vector< string > payout_comments;  /// pending comments by payout
   vector< string > trending30;       /// pending lifetime payout
   vector< string > created;          /// creation date
   vector< string > responses;        /// creation date
   vector< string > updated;          /// creation date
   vector< string > active;           /// last update or reply
   vector< string > votes;            /// last update or reply
   vector< string > cashout;          /// last update or reply
   vector< string > maturing;         /// about to be paid out
   vector< string > best;             /// total lifetime payout
   vector< string > hot;              /// total lifetime payout
   vector< string > promoted;         /// pending lifetime payout
};

struct extended_limit_order : public database_api::api_limit_order_object
{
   extended_limit_order( const limit_order_object& o ) :
      database_api::api_limit_order_object( o ) {}

   extended_limit_order(){}

   double real_price  = 0;
   bool   rewarded    = false;
};


struct api_operation_object
{
   api_operation_object( const account_history::api_operation_object& obj, const legacy_operation& l_op ) :
      trx_id( obj.trx_id ),
      block( obj.block ),
      trx_in_block( obj.trx_in_block ),
      virtual_op( obj.virtual_op ),
      timestamp( obj.timestamp ),
      op( l_op )
   {}

   transaction_id_type  trx_id;
   uint32_t             block = 0;
   uint32_t             trx_in_block = 0;
   uint16_t             op_in_trx = 0;
   uint64_t             virtual_op = 0;
   fc::time_point_sec   timestamp;
   legacy_operation     op;
};

struct api_account_object
{
   api_account_object( const database_api::api_account_object& a ) :
      id( a.id ),
      name( a.name ),
      owner( a.owner ),
      active( a.active ),
      posting( a.posting ),
      memo_key( a.memo_key ),
      json_metadata( a.json_metadata ),
      proxy( a.proxy ),
      last_owner_update( a.last_owner_update ),
      last_account_update( a.last_account_update ),
      created( a.created ),
      mined( a.mined ),
      recovery_account( a.recovery_account ),
      reset_account( a.reset_account ),
      last_account_recovery( a.last_account_recovery ),
      comment_count( a.comment_count ),
      lifetime_vote_count( a.lifetime_vote_count ),
      post_count( a.post_count ),
      can_vote( a.can_vote ),
      voting_power( a.voting_power ),
      last_vote_time( a.last_vote_time ),
      balance( legacy_asset::from_asset( a.balance ) ),
      savings_balance( legacy_asset::from_asset( a.savings_balance ) ),
      sbd_balance( legacy_asset::from_asset( a.sbd_balance ) ),
      sbd_seconds( a.sbd_seconds ),
      sbd_seconds_last_update( a.sbd_seconds_last_update ),
      sbd_last_interest_payment( a.sbd_last_interest_payment ),
      savings_sbd_balance( legacy_asset::from_asset( a.savings_sbd_balance ) ),
      savings_sbd_seconds( a.savings_sbd_seconds ),
      savings_sbd_seconds_last_update( a.savings_sbd_seconds_last_update ),
      savings_sbd_last_interest_payment( a.savings_sbd_last_interest_payment ),
      savings_withdraw_requests( a.savings_withdraw_requests ),
      reward_sbd_balance( legacy_asset::from_asset( a.reward_sbd_balance ) ),
      reward_steem_balance( legacy_asset::from_asset( a.reward_steem_balance ) ),
      reward_vesting_balance( legacy_asset::from_asset( a.reward_vesting_balance ) ),
      reward_vesting_steem( legacy_asset::from_asset( a.reward_vesting_steem ) ),
      curation_rewards( a.curation_rewards ),
      posting_rewards( a.posting_rewards ),
      vesting_shares( legacy_asset::from_asset( a.vesting_shares ) ),
      delegated_vesting_shares( legacy_asset::from_asset( a.delegated_vesting_shares ) ),
      received_vesting_shares( legacy_asset::from_asset( a.received_vesting_shares ) ),
      vesting_withdraw_rate( legacy_asset::from_asset( a.vesting_withdraw_rate ) ),
      next_vesting_withdrawal( a.next_vesting_withdrawal ),
      withdrawn( a.withdrawn ),
      to_withdraw( a.to_withdraw ),
      withdraw_routes( a.withdraw_routes ),
      witnesses_voted_for( a.witnesses_voted_for ),
      last_post( a.last_post ),
      last_root_post( a.last_root_post )
   {
      proxied_vsf_votes.insert( proxied_vsf_votes.end(), a.proxied_vsf_votes.begin(), a.proxied_vsf_votes.end() );
   }


   api_account_object(){}

   account_id_type   id;

   account_name_type name;
   authority         owner;
   authority         active;
   authority         posting;
   public_key_type   memo_key;
   string            json_metadata;
   account_name_type proxy;

   time_point_sec    last_owner_update;
   time_point_sec    last_account_update;

   time_point_sec    created;
   bool              mined = false;
   account_name_type recovery_account;
   account_name_type reset_account;
   time_point_sec    last_account_recovery;
   uint32_t          comment_count = 0;
   uint32_t          lifetime_vote_count = 0;
   uint32_t          post_count = 0;

   bool              can_vote = false;
   uint16_t          voting_power = 0;
   time_point_sec    last_vote_time;

   legacy_asset      balance;
   legacy_asset      savings_balance;

   legacy_asset      sbd_balance;
   uint128_t         sbd_seconds;
   time_point_sec    sbd_seconds_last_update;
   time_point_sec    sbd_last_interest_payment;

   legacy_asset      savings_sbd_balance;
   uint128_t         savings_sbd_seconds;
   time_point_sec    savings_sbd_seconds_last_update;
   time_point_sec    savings_sbd_last_interest_payment;

   uint8_t           savings_withdraw_requests = 0;

   legacy_asset      reward_sbd_balance;
   legacy_asset      reward_steem_balance;
   legacy_asset      reward_vesting_balance;
   legacy_asset      reward_vesting_steem;

   share_type        curation_rewards;
   share_type        posting_rewards;

   legacy_asset      vesting_shares;
   legacy_asset      delegated_vesting_shares;
   legacy_asset      received_vesting_shares;
   legacy_asset      vesting_withdraw_rate;
   time_point_sec    next_vesting_withdrawal;
   share_type        withdrawn;
   share_type        to_withdraw;
   uint16_t          withdraw_routes = 0;

   vector< share_type > proxied_vsf_votes;

   uint16_t          witnesses_voted_for;

   time_point_sec    last_post;
   time_point_sec    last_root_post;
};

struct extended_account : public api_account_object
{
   extended_account(){}
   extended_account( const database_api::api_account_object& a ) :
      api_account_object( a ) {}

   share_type                                               average_bandwidth;
   share_type                                               lifetime_bandwidth;
   time_point_sec                                           last_bandwidth_update;
   share_type                                               average_market_bandwidth;
   share_type                                               lifetime_market_bandwidth;
   time_point_sec                                           last_market_bandwidth_update;

   legacy_asset                                             vesting_balance;  /// convert vesting_shares to vesting steem
   share_type                                               reputation = 0;
   map< uint64_t, api_operation_object >   transfer_history; /// transfer to/from vesting
   map< uint64_t, api_operation_object >   market_history;   /// limit order / cancel / fill
   map< uint64_t, api_operation_object >   post_history;
   map< uint64_t, api_operation_object >   vote_history;
   map< uint64_t, api_operation_object >   other_history;
   set< string >                                            witness_votes;
   vector< tags::tag_count_object >                         tags_usage;
   vector< follow::reblog_count >                           guest_bloggers;

   optional< map< uint32_t, extended_limit_order > >        open_orders;
   optional< vector< string > >                             comments;         /// permlinks for this user
   optional< vector< string > >                             blog;             /// blog posts for this user
   optional< vector< string > >                             feed;             /// feed posts for this user
   optional< vector< string > >                             recent_replies;   /// blog posts for this user
   optional< vector< string > >                             recommended;      /// posts recommened for this user
};

struct extended_dynamic_global_properties
{
   extended_dynamic_global_properties() {}
   extended_dynamic_global_properties( const database_api::api_dynamic_global_property_object& o ) :
      head_block_number( o.head_block_number ),
      head_block_id( o.head_block_id ),
      time( o.time ),
      current_witness( o.current_witness ),
      total_pow( o.total_pow ),
      num_pow_witnesses( o.num_pow_witnesses ),
      virtual_supply( legacy_asset::from_asset( o.virtual_supply ) ),
      current_supply( legacy_asset::from_asset( o.current_supply ) ),
      confidential_supply( legacy_asset::from_asset( o.confidential_supply ) ),
      current_sbd_supply( legacy_asset::from_asset( o.current_sbd_supply ) ),
      confidential_sbd_supply( legacy_asset::from_asset( o.confidential_sbd_supply ) ),
      total_vesting_fund_steem( legacy_asset::from_asset( o.total_vesting_fund_steem ) ),
      total_vesting_shares( legacy_asset::from_asset( o.total_vesting_shares ) ),
      total_reward_fund_steem( legacy_asset::from_asset( o.total_reward_fund_steem ) ),
      total_reward_shares2( o.total_reward_shares2 ),
      pending_rewarded_vesting_shares( legacy_asset::from_asset( o.pending_rewarded_vesting_shares ) ),
      pending_rewarded_vesting_steem( legacy_asset::from_asset( o.pending_rewarded_vesting_steem ) ),
      sbd_interest_rate( o.sbd_interest_rate ),
      sbd_print_rate( o.sbd_print_rate ),
      maximum_block_size( o.maximum_block_size ),
      current_aslot( o.current_aslot ),
      recent_slots_filled( o.recent_slots_filled ),
      participation_count( o.participation_count ),
      last_irreversible_block_num( o.last_irreversible_block_num ),
      vote_power_reserve_rate( o.vote_power_reserve_rate )
   {}

   uint32_t          head_block_number = 0;
   block_id_type     head_block_id;
   time_point_sec    time;
   account_name_type current_witness;

   uint64_t          total_pow = -1;

   uint32_t          num_pow_witnesses = 0;

   legacy_asset      virtual_supply;
   legacy_asset      current_supply;
   legacy_asset      confidential_supply;
   legacy_asset      current_sbd_supply;
   legacy_asset      confidential_sbd_supply;
   legacy_asset      total_vesting_fund_steem;
   legacy_asset      total_vesting_shares;
   legacy_asset      total_reward_fund_steem;
   fc::uint128       total_reward_shares2;
   legacy_asset      pending_rewarded_vesting_shares;
   legacy_asset      pending_rewarded_vesting_steem;

   uint16_t          sbd_interest_rate = 0;
   uint16_t          sbd_print_rate = STEEM_100_PERCENT;

   uint32_t          maximum_block_size = 0;
   uint64_t          current_aslot = 0;
   fc::uint128_t     recent_slots_filled;
   uint8_t           participation_count = 0;

   uint32_t          last_irreversible_block_num = 0;

   uint32_t          vote_power_reserve_rate = STEEM_INITIAL_VOTE_POWER_RATE;

   int32_t           average_block_size = 0;
   int64_t           current_reserve_ratio = 1;
   uint128_t         max_virtual_bandwidth = 0;
};

struct tag_index
{
   vector< tags::tag_name_type > trending; /// pending payouts
};

struct api_tag_object
{
   api_tag_object( const tags::api_tag_object& o ) :
      name( o.name ),
      total_payouts( protocol::legacy_asset::from_asset( o.total_payouts ) ),
      net_votes( o.net_votes ),
      top_posts( o.top_posts ),
      comments( o.comments ),
      trending( o.trending ) {}

   api_tag_object() {}

   string               name;
   legacy_asset         total_payouts;
   int32_t              net_votes = 0;
   uint32_t             top_posts = 0;
   uint32_t             comments = 0;
   fc::uint128          trending = 0;
};

struct state
{
   string                                             current_route;

   extended_dynamic_global_properties                 props;

   tag_index                                          tag_idx;

   /**
    * "" is the global tags::discussion index
    */
   map< string, discussion_index >                    discussion_idx;

   map< string, api_tag_object >                tags;

   /**
    *  map from account/slug to full nested tags::discussion
    */
   map< string, tags::discussion >                    content;
   map< string, extended_account >                    accounts;

   map< string, database_api::api_witness_object >    witnesses;
   database_api::api_witness_schedule_object          witness_schedule;
   price                                              feed_price;
   string                                             error;
};

struct scheduled_hardfork
{
   hardfork_version     hf_version;
   fc::time_point_sec   live_time;
};

struct account_vote
{
   string         authorperm;
   uint64_t       weight = 0;
   int64_t        rshares = 0;
   int16_t        percent = 0;
   time_point_sec time;
};

enum withdraw_route_type
{
   incoming,
   outgoing,
   all
};

typedef vector< variant > get_version_args;

struct get_version_return
{
   get_version_return() {}
   get_version_return( fc::string bc_v, fc::string s_v, fc::string fc_v )
      :blockchain_version( bc_v ), steem_revision( s_v ), fc_revision( fc_v ) {}

   fc::string blockchain_version;
   fc::string steem_revision;
   fc::string fc_revision;
};

typedef map< uint32_t, api_operation_object > get_account_history_return_type;

#define DEFINE_API_ARGS( api_name, arg_type, return_type )  \
typedef arg_type api_name ## _args;                         \
typedef return_type api_name ## _return;

/*               API,                                    args,                return */
DEFINE_API_ARGS( get_trending_tags,                      vector< variant >,   vector< api_tag_object > )
DEFINE_API_ARGS( get_state,                              vector< variant >,   state )
DEFINE_API_ARGS( get_active_witnesses,                   vector< variant >,   vector< account_name_type > )
DEFINE_API_ARGS( get_block_header,                       vector< variant >,   optional< block_header > )
DEFINE_API_ARGS( get_block,                              vector< variant >,   optional< block_api::api_signed_block_object > )
DEFINE_API_ARGS( get_ops_in_block,                       vector< variant >,   vector< api_operation_object > )
DEFINE_API_ARGS( get_config,                             vector< variant >,   fc::variant_object )
DEFINE_API_ARGS( get_dynamic_global_properties,          vector< variant >,   extended_dynamic_global_properties )
DEFINE_API_ARGS( get_chain_properties,                   vector< variant >,   chain_properties )
DEFINE_API_ARGS( get_current_median_history_price,       vector< variant >,   price )
DEFINE_API_ARGS( get_feed_history,                       vector< variant >,   database_api::api_feed_history_object )
DEFINE_API_ARGS( get_witness_schedule,                   vector< variant >,   database_api::api_witness_schedule_object )
DEFINE_API_ARGS( get_hardfork_version,                   vector< variant >,   hardfork_version )
DEFINE_API_ARGS( get_next_scheduled_hardfork,            vector< variant >,   scheduled_hardfork )
DEFINE_API_ARGS( get_reward_fund,                        vector< variant >,   database_api::api_reward_fund_object )
DEFINE_API_ARGS( get_key_references,                     vector< variant >,   vector< vector< account_name_type > > )
DEFINE_API_ARGS( get_accounts,                           vector< variant >,   vector< extended_account > )
DEFINE_API_ARGS( get_account_references,                 vector< variant >,   vector< account_id_type > )
DEFINE_API_ARGS( lookup_account_names,                   vector< variant >,   vector< optional< api_account_object > > )
DEFINE_API_ARGS( lookup_accounts,                        vector< variant >,   set< string > )
DEFINE_API_ARGS( get_account_count,                      vector< variant >,   uint64_t )
DEFINE_API_ARGS( get_owner_history,                      vector< variant >,   vector< database_api::api_owner_authority_history_object > )
DEFINE_API_ARGS( get_recovery_request,                   vector< variant >,   optional< database_api::api_account_recovery_request_object > )
DEFINE_API_ARGS( get_escrow,                             vector< variant >,   optional< database_api::api_escrow_object > )
DEFINE_API_ARGS( get_withdraw_routes,                    vector< variant >,   vector< database_api::api_withdraw_vesting_route_object > )
DEFINE_API_ARGS( get_account_bandwidth,                  vector< variant >,   optional< witness::api_account_bandwidth_object > )
DEFINE_API_ARGS( get_savings_withdraw_from,              vector< variant >,   vector< database_api::api_savings_withdraw_object > )
DEFINE_API_ARGS( get_savings_withdraw_to,                vector< variant >,   vector< database_api::api_savings_withdraw_object > )
DEFINE_API_ARGS( get_vesting_delegations,                vector< variant >,   vector< database_api::api_vesting_delegation_object > )
DEFINE_API_ARGS( get_expiring_vesting_delegations,       vector< variant >,   vector< database_api::api_vesting_delegation_expiration_object > )
DEFINE_API_ARGS( get_witnesses,                          vector< variant >,   vector< optional< database_api::api_witness_object > > )
DEFINE_API_ARGS( get_conversion_requests,                vector< variant >,   vector< database_api::api_convert_request_object > )
DEFINE_API_ARGS( get_witness_by_account,                 vector< variant >,   optional< database_api::api_witness_object > )
DEFINE_API_ARGS( get_witnesses_by_vote,                  vector< variant >,   vector< database_api::api_witness_object > )
DEFINE_API_ARGS( lookup_witness_accounts,                vector< variant >,   vector< account_name_type > )
DEFINE_API_ARGS( get_open_orders,                        vector< variant >,   vector< extended_limit_order > )
DEFINE_API_ARGS( get_witness_count,                      vector< variant >,   uint64_t )
DEFINE_API_ARGS( get_transaction_hex,                    vector< variant >,   string )
DEFINE_API_ARGS( get_transaction,                        vector< variant >,   annotated_signed_transaction )
DEFINE_API_ARGS( get_required_signatures,                vector< variant >,   set< public_key_type > )
DEFINE_API_ARGS( get_potential_signatures,               vector< variant >,   set< public_key_type > )
DEFINE_API_ARGS( verify_authority,                       vector< variant >,   bool )
DEFINE_API_ARGS( verify_account_authority,               vector< variant >,   bool )
DEFINE_API_ARGS( get_active_votes,                       vector< variant >,   vector< tags::vote_state > )
DEFINE_API_ARGS( get_account_votes,                      vector< variant >,   vector< account_vote > )
DEFINE_API_ARGS( get_content,                            vector< variant >,   tags::discussion )
DEFINE_API_ARGS( get_content_replies,                    vector< variant >,   vector< tags::discussion > )
DEFINE_API_ARGS( get_tags_used_by_author,                vector< variant >,   vector< tags::tag_count_object > )
DEFINE_API_ARGS( get_post_discussions_by_payout,         vector< variant >,   vector< tags::discussion > )
DEFINE_API_ARGS( get_comment_discussions_by_payout,      vector< variant >,   vector< tags::discussion > )
DEFINE_API_ARGS( get_discussions_by_trending,            vector< variant >,   vector< tags::discussion > )
DEFINE_API_ARGS( get_discussions_by_created,             vector< variant >,   vector< tags::discussion > )
DEFINE_API_ARGS( get_discussions_by_active,              vector< variant >,   vector< tags::discussion > )
DEFINE_API_ARGS( get_discussions_by_cashout,             vector< variant >,   vector< tags::discussion > )
DEFINE_API_ARGS( get_discussions_by_votes,               vector< variant >,   vector< tags::discussion > )
DEFINE_API_ARGS( get_discussions_by_children,            vector< variant >,   vector< tags::discussion > )
DEFINE_API_ARGS( get_discussions_by_hot,                 vector< variant >,   vector< tags::discussion > )
DEFINE_API_ARGS( get_discussions_by_feed,                vector< variant >,   vector< tags::discussion > )
DEFINE_API_ARGS( get_discussions_by_blog,                vector< variant >,   vector< tags::discussion > )
DEFINE_API_ARGS( get_discussions_by_comments,            vector< variant >,   vector< tags::discussion > )
DEFINE_API_ARGS( get_discussions_by_promoted,            vector< variant >,   vector< tags::discussion > )
DEFINE_API_ARGS( get_replies_by_last_update,             vector< variant >,   vector< tags::discussion > )
DEFINE_API_ARGS( get_discussions_by_author_before_date,  vector< variant >,   vector< tags::discussion > )
DEFINE_API_ARGS( get_account_history,                    vector< variant >,   get_account_history_return_type )
DEFINE_API_ARGS( broadcast_transaction,                  vector< variant >,   json_rpc::void_type )
DEFINE_API_ARGS( broadcast_transaction_synchronous,      vector< variant >,   network_broadcast_api::broadcast_transaction_synchronous_return )
DEFINE_API_ARGS( broadcast_block,                        vector< variant >,   json_rpc::void_type )
DEFINE_API_ARGS( get_followers,                          vector< variant >,   vector< follow::api_follow_object > )
DEFINE_API_ARGS( get_following,                          vector< variant >,   vector< follow::api_follow_object > )
DEFINE_API_ARGS( get_follow_count,                       vector< variant >,   follow::get_follow_count_return )
DEFINE_API_ARGS( get_feed_entries,                       vector< variant >,   vector< follow::feed_entry > )
DEFINE_API_ARGS( get_feed,                               vector< variant >,   vector< follow::comment_feed_entry > )
DEFINE_API_ARGS( get_blog_entries,                       vector< variant >,   vector< follow::blog_entry > )
DEFINE_API_ARGS( get_blog,                               vector< variant >,   vector< follow::comment_blog_entry > )
DEFINE_API_ARGS( get_account_reputations,                vector< variant >,   vector< follow::account_reputation > )
DEFINE_API_ARGS( get_reblogged_by,                       vector< variant >,   vector< account_name_type > )
DEFINE_API_ARGS( get_blog_authors,                       vector< variant >,   vector< follow::reblog_count > )
DEFINE_API_ARGS( get_ticker,                             vector< variant >,   market_history::get_ticker_return )
DEFINE_API_ARGS( get_volume,                             vector< variant >,   market_history::get_volume_return )
DEFINE_API_ARGS( get_order_book,                         vector< variant >,   market_history::get_order_book_return )
DEFINE_API_ARGS( get_trade_history,                      vector< variant >,   vector< market_history::market_trade > )
DEFINE_API_ARGS( get_recent_trades,                      vector< variant >,   vector< market_history::market_trade > )
DEFINE_API_ARGS( get_market_history,                     vector< variant >,   vector< market_history::bucket_object > )
DEFINE_API_ARGS( get_market_history_buckets,             vector< variant >,   flat_set< uint32_t > )

#undef DEFINE_API_ARGS

class condenser_api
{
public:
   condenser_api();
   ~condenser_api();

   DECLARE_API(
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
      (get_account_bandwidth)
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

   private:
      friend class condenser_api_plugin;
      void api_startup();

      std::unique_ptr< detail::condenser_api_impl > my;
};

} } } // steem::plugins::condenser_api

FC_REFLECT( steem::plugins::condenser_api::discussion_index,
            (category)(trending)(payout)(payout_comments)(trending30)(updated)(created)(responses)(active)(votes)(maturing)(best)(hot)(promoted)(cashout) )

FC_REFLECT( steem::plugins::condenser_api::api_tag_object,
            (name)(total_payouts)(net_votes)(top_posts)(comments)(trending) )

FC_REFLECT( steem::plugins::condenser_api::state,
            (current_route)(props)(tag_idx)(tags)(content)(accounts)(witnesses)(discussion_idx)(witness_schedule)(feed_price)(error) )

FC_REFLECT_DERIVED( steem::plugins::condenser_api::extended_limit_order, (steem::plugins::database_api::api_limit_order_object),
            (real_price)(rewarded) )

FC_REFLECT( steem::plugins::condenser_api::api_operation_object,
             (trx_id)(block)(trx_in_block)(op_in_trx)(virtual_op)(timestamp)(op) )

FC_REFLECT( steem::plugins::condenser_api::api_account_object,
             (id)(name)(owner)(active)(posting)(memo_key)(json_metadata)(proxy)(last_owner_update)(last_account_update)
             (created)(mined)
             (recovery_account)(last_account_recovery)(reset_account)
             (comment_count)(lifetime_vote_count)(post_count)(can_vote)(voting_power)(last_vote_time)
             (balance)
             (savings_balance)
             (sbd_balance)(sbd_seconds)(sbd_seconds_last_update)(sbd_last_interest_payment)
             (savings_sbd_balance)(savings_sbd_seconds)(savings_sbd_seconds_last_update)(savings_sbd_last_interest_payment)(savings_withdraw_requests)
             (reward_sbd_balance)(reward_steem_balance)(reward_vesting_balance)(reward_vesting_steem)
             (vesting_shares)(delegated_vesting_shares)(received_vesting_shares)(vesting_withdraw_rate)(next_vesting_withdrawal)(withdrawn)(to_withdraw)(withdraw_routes)
             (curation_rewards)
             (posting_rewards)
             (proxied_vsf_votes)(witnesses_voted_for)
             (last_post)(last_root_post)
          )

FC_REFLECT_DERIVED( steem::plugins::condenser_api::extended_account, (steem::plugins::condenser_api::api_account_object),
            (average_bandwidth)(lifetime_bandwidth)(last_bandwidth_update)(average_market_bandwidth)(lifetime_market_bandwidth)(last_market_bandwidth_update)
            (vesting_balance)(reputation)(transfer_history)(market_history)(post_history)(vote_history)(other_history)(witness_votes)(tags_usage)(guest_bloggers)(open_orders)(comments)(feed)(blog)(recent_replies)(recommended) )

FC_REFLECT( steem::plugins::condenser_api::extended_dynamic_global_properties,
            (head_block_number)(head_block_id)(time)
            (current_witness)(total_pow)(num_pow_witnesses)
            (virtual_supply)(current_supply)(confidential_supply)(current_sbd_supply)(confidential_sbd_supply)
            (total_vesting_fund_steem)(total_vesting_shares)
            (total_reward_fund_steem)(total_reward_shares2)(pending_rewarded_vesting_shares)(pending_rewarded_vesting_steem)
            (sbd_interest_rate)(sbd_print_rate)
            (maximum_block_size)(current_aslot)(recent_slots_filled)(participation_count)(last_irreversible_block_num)(vote_power_reserve_rate)
            (average_block_size)(current_reserve_ratio)(max_virtual_bandwidth) )

FC_REFLECT( steem::plugins::condenser_api::scheduled_hardfork,
            (hf_version)(live_time) )

FC_REFLECT( steem::plugins::condenser_api::account_vote,
            (authorperm)(weight)(rshares)(percent)(time) )

FC_REFLECT( steem::plugins::condenser_api::tag_index, (trending) )

FC_REFLECT_ENUM( steem::plugins::condenser_api::withdraw_route_type, (incoming)(outgoing)(all) )

FC_REFLECT( steem::plugins::condenser_api::get_version_return,
            (blockchain_version)(steem_revision)(fc_revision) )
