#pragma once
#include <steemit/plugins/json_rpc/utility.hpp>

#include <steemit/plugins/database_api/database_api.hpp>
#include <steemit/plugins/account_history_api/account_history_api.hpp>
#include <steemit/plugins/account_by_key_api/account_by_key_api.hpp>
#include <steemit/plugins/network_broadcast_api/network_broadcast_api.hpp>
#include <steemit/plugins/tags_api/tags_api.hpp>
#include <steemit/plugins/follow_api/follow_api.hpp>
#include <steemit/plugins/market_history_api/market_history_api.hpp>
#include <steemit/plugins/witness_api/witness_api.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>
#include <fc/api.hpp>

namespace steemit { namespace plugins { namespace condenser_api {

using std::vector;
using fc::variant;
using fc::optional;
using steemit::plugins::json_rpc::void_type;

using namespace chain;

namespace detail{ class condenser_api_impl; }

struct discussion_index
{
   string                        category;         /// category by which everything is filtered
   vector< tags::tag_name_type > trending;         /// trending posts over the last 24 hours
   vector< tags::tag_name_type > payout;           /// pending posts by payout
   vector< tags::tag_name_type > payout_comments;  /// pending comments by payout
   vector< tags::tag_name_type > trending30;       /// pending lifetime payout
   vector< tags::tag_name_type > created;          /// creation date
   vector< tags::tag_name_type > responses;        /// creation date
   vector< tags::tag_name_type > updated;          /// creation date
   vector< tags::tag_name_type > active;           /// last update or reply
   vector< tags::tag_name_type > votes;            /// last update or reply
   vector< tags::tag_name_type > cashout;          /// last update or reply
   vector< tags::tag_name_type > maturing;         /// about to be paid out
   vector< tags::tag_name_type > best;             /// total lifetime payout
   vector< tags::tag_name_type > hot;              /// total lifetime payout
   vector< tags::tag_name_type > promoted;         /// pending lifetime payout
};

struct extended_limit_order : public database_api::api_limit_order_object
{
   extended_limit_order( const limit_order_object& o ) :
      database_api::api_limit_order_object( o ) {}

   extended_limit_order(){}

   double real_price  = 0;
   bool   rewarded    = false;
};

/**
 *  Convert's vesting shares
 */
struct extended_account : public database_api::api_account_object
{
   extended_account(){}
   extended_account( const account_object& a, const database& db ) :
      database_api::api_account_object( a, db ) {}

   asset                                                    vesting_balance;  /// convert vesting_shares to vesting steem
   share_type                                               reputation = 0;
   map< uint64_t, account_history::api_operation_object >   transfer_history; /// transfer to/from vesting
   map< uint64_t, account_history::api_operation_object >   market_history;   /// limit order / cancel / fill
   map< uint64_t, account_history::api_operation_object >   post_history;
   map< uint64_t, account_history::api_operation_object >   vote_history;
   map< uint64_t, account_history::api_operation_object >   other_history;
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

struct tag_index
{
   vector< tags::tag_name_type > trending; /// pending payouts
};

struct state
{
   string                                             current_route;

   database_api::api_dynamic_global_property_object   props;

   tag_index                                          tag_idx;

   /**
    * "" is the global tags::discussion index
    */
   map< string, discussion_index >                    discussion_idx;

   map< string, tags::api_tag_object >                tags;

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

typedef void_type get_version_args;

struct get_version_return
{
   get_version_return() {}
   get_version_return( fc::string bc_v, fc::string s_v, fc::string fc_v )
      :blockchain_version( bc_v ), steem_revision( s_v ), fc_revision( fc_v ) {}

   fc::string blockchain_version;
   fc::string steem_revision;
   fc::string fc_revision;
};

typedef map< uint32_t, account_history::api_operation_object > get_account_history_return_type;

/*               API,                                    args,                return */
DEFINE_API_ARGS( get_trending_tags,                      vector< variant >,   vector< tags::api_tag_object > )
DEFINE_API_ARGS( get_state,                              vector< variant >,   state )
DEFINE_API_ARGS( get_active_witnesses,                   void_type,           vector< account_name_type > )
DEFINE_API_ARGS( get_block_header,                       vector< variant >,   optional< block_header > )
DEFINE_API_ARGS( get_block,                              vector< variant >,   optional< database_api::api_signed_block_object > )
DEFINE_API_ARGS( get_ops_in_block,                       vector< variant >,   vector< account_history::api_operation_object > )
DEFINE_API_ARGS( get_config,                             void_type,           fc::variant_object )
DEFINE_API_ARGS( get_dynamic_global_properties,          void_type,           database_api::api_dynamic_global_property_object )
DEFINE_API_ARGS( get_chain_properties,                   void_type,           chain_properties )
DEFINE_API_ARGS( get_current_median_history_price,       void_type,           price )
DEFINE_API_ARGS( get_feed_history,                       void_type,           database_api::api_feed_history_object )
DEFINE_API_ARGS( get_witness_schedule,                   void_type,           database_api::api_witness_schedule_object )
DEFINE_API_ARGS( get_hardfork_version,                   void_type,           hardfork_version )
DEFINE_API_ARGS( get_next_scheduled_hardfork,            void_type,           scheduled_hardfork )
DEFINE_API_ARGS( get_reward_fund,                        vector< variant >,   database_api::api_reward_fund_object )
DEFINE_API_ARGS( get_key_references,                     vector< variant >,   vector< vector< account_name_type > > )
DEFINE_API_ARGS( get_accounts,                           vector< variant >,   vector< extended_account > )
DEFINE_API_ARGS( get_account_references,                 vector< variant >,   vector< account_id_type > )
DEFINE_API_ARGS( lookup_account_names,                   vector< variant >,   vector< optional< database_api::api_account_object > > )
DEFINE_API_ARGS( lookup_accounts,                        vector< variant >,   set< string > )
DEFINE_API_ARGS( get_account_count,                      void_type,           uint64_t )
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
DEFINE_API_ARGS( get_witness_count,                      void_type,           uint64_t )
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
DEFINE_API_ARGS( get_discussions_by_payout,              vector< variant >,   vector< tags::discussion > )
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
DEFINE_API_ARGS( broadcast_transaction,                  vector< variant >,   void_type )
DEFINE_API_ARGS( broadcast_transaction_synchronous,      vector< variant >,   network_broadcast_api::broadcast_transaction_synchronous_return )
DEFINE_API_ARGS( broadcast_block,                        vector< variant >,   void_type )
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
DEFINE_API_ARGS( get_ticker,                             void_type,           market_history::get_ticker_return )
DEFINE_API_ARGS( get_volume,                             void_type,           market_history::get_volume_return )
DEFINE_API_ARGS( get_order_book,                         vector< variant >,   market_history::get_order_book_return )
DEFINE_API_ARGS( get_trade_history,                      vector< variant >,   vector< market_history::market_trade > )
DEFINE_API_ARGS( get_recent_trades,                      vector< variant >,   vector< market_history::market_trade > )
DEFINE_API_ARGS( get_market_history,                     vector< variant >,   vector< market_history::bucket_object > )
DEFINE_API_ARGS( get_market_history_buckets,             void_type,           flat_set< uint32_t > )

class condenser_api
{
public:
   condenser_api();
   ~condenser_api();

   DECLARE_API( get_version )
   DECLARE_API( get_trending_tags )
   DECLARE_API( get_state )
   DECLARE_API( get_active_witnesses )
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
   DECLARE_API( get_open_orders )
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
   DECLARE_API( broadcast_transaction )
   DECLARE_API( broadcast_transaction_synchronous )
   DECLARE_API( broadcast_block )
   DECLARE_API( get_followers )
   DECLARE_API( get_following )
   DECLARE_API( get_follow_count )
   DECLARE_API( get_feed_entries )
   DECLARE_API( get_feed )
   DECLARE_API( get_blog_entries )
   DECLARE_API( get_blog )
   DECLARE_API( get_account_reputations )
   DECLARE_API( get_reblogged_by )
   DECLARE_API( get_blog_authors )
   DECLARE_API( get_ticker )
   DECLARE_API( get_volume )
   DECLARE_API( get_order_book )
   DECLARE_API( get_trade_history )
   DECLARE_API( get_recent_trades )
   DECLARE_API( get_market_history )
   DECLARE_API( get_market_history_buckets )

   private:
      friend class condenser_api_plugin;
      void api_startup();

      std::unique_ptr< detail::condenser_api_impl > my;
};

} } } // steemit::plugins::condenser_api

FC_REFLECT( steemit::plugins::condenser_api::discussion_index,
            (category)(trending)(payout)(payout_comments)(trending30)(updated)(created)(responses)(active)(votes)(maturing)(best)(hot)(promoted)(cashout) )

FC_REFLECT( steemit::plugins::condenser_api::state,
            (current_route)(props)(tag_idx)(tags)(content)(accounts)(witnesses)(discussion_idx)(witness_schedule)(feed_price)(error) )

FC_REFLECT_DERIVED( steemit::plugins::condenser_api::extended_limit_order, (steemit::plugins::database_api::api_limit_order_object),
            (real_price)(rewarded) )

FC_REFLECT_DERIVED( steemit::plugins::condenser_api::extended_account, (steemit::plugins::database_api::api_account_object),
            (vesting_balance)(reputation)(transfer_history)(market_history)(post_history)(vote_history)(other_history)(witness_votes)(tags_usage)(guest_bloggers)(open_orders)(comments)(feed)(blog)(recent_replies)(recommended) )

FC_REFLECT( steemit::plugins::condenser_api::scheduled_hardfork,
            (hf_version)(live_time) )

FC_REFLECT( steemit::plugins::condenser_api::account_vote,
            (authorperm)(weight)(rshares)(percent)(time) )

FC_REFLECT( steemit::plugins::condenser_api::tag_index, (trending) )

FC_REFLECT_ENUM( steemit::plugins::condenser_api::withdraw_route_type, (incoming)(outgoing)(all) )

FC_REFLECT( steemit::plugins::condenser_api::get_version_return,
            (blockchain_version)(steem_revision)(fc_revision) )
