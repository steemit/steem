#pragma once
#include <steemit/app/state.hpp>
#include <steemit/chain/protocol/types.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/chain/steem_objects.hpp>
#include <steemit/chain/history_object.hpp>
#include <steemit/tags/tags_plugin.hpp>

#include <fc/api.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>

#include <fc/network/ip.hpp>

#include <boost/container/flat_set.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace steemit { namespace app {

using namespace steemit::chain;
using namespace std;

struct order
{
   price                order_price;
   share_type           steem;
   share_type           sbd;
   fc::time_point_sec   created;
};

struct order_book
{
   vector< order >      asks;
   vector< order >      bids;
};

struct api_context;

struct scheduled_hardfork
{
   hardfork_version     hf_version;
   fc::time_point_sec   live_time;
};


class database_api_impl;

/**
 *  Defines the arguments to a query as a struct so it can be easily extended
 */
struct discussion_query {
   void validate()const{
      FC_ASSERT( filter_tags.find(tag) == filter_tags.end() );
      FC_ASSERT( tag.size() );
      FC_ASSERT( limit <= 100 );
   }

   string           tag;
   uint32_t         limit;
   set<string>      filter_tags;
   optional<string> start_author;
   optional<string> start_permlink;
   optional<string> parent_author;
   optional<string> parent_permlink;
};

/**
 * @brief The database_api class implements the RPC API for the chain database.
 *
 * This API exposes accessors on the database which query state tracked by a blockchain validating node. This API is
 * read-only; all modifications to the database must be performed via transactions. Transactions are broadcast via
 * the @ref network_broadcast_api.
 */
class database_api
{
   public:
      database_api(steemit::chain::database& db);
      database_api(const steemit::app::api_context& ctx);
      ~database_api();

      ///////////////////
      // Subscriptions //
      ///////////////////

      void set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter );
      void set_pending_transaction_callback( std::function<void(const variant&)> cb );
      void set_block_applied_callback( std::function<void(const variant& block_header)> cb );
      /**
       * @brief Stop receiving any notifications
       *
       * This unsubscribes from all subscribed markets and objects.
       */
      void cancel_all_subscriptions();

      vector<tags::tag_stats_object> get_trending_tags( string after_tag, uint32_t limit )const;

      /**
       *  This API is a short-cut for returning all of the state required for a particular URL
       *  with a single query.
       */
      state get_state( string path )const;
      vector<category_object> get_trending_categories( string after, uint32_t limit )const;
      vector<category_object> get_best_categories( string after, uint32_t limit )const;
      vector<category_object> get_active_categories( string after, uint32_t limit )const;
      vector<category_object> get_recent_categories( string after, uint32_t limit )const;

      vector<string> get_active_witnesses()const;
      vector<string> get_miner_queue()const;

      /////////////////////////////
      // Blocks and transactions //
      /////////////////////////////

      /**
       * @brief Retrieve a block header
       * @param block_num Height of the block whose header should be returned
       * @return header of the referenced block, or null if no matching block was found
       */
      optional<block_header> get_block_header(uint32_t block_num)const;

      /**
       * @brief Retrieve a full, signed block
       * @param block_num Height of the block to be returned
       * @return the referenced block, or null if no matching block was found
       */
      optional<signed_block> get_block(uint32_t block_num)const;

      /////////////
      // Globals //
      /////////////

      /**
       * @brief Retrieve compile-time constants
       */
      fc::variant_object get_config()const;

      /**
       * @brief Retrieve the current @ref dynamic_global_property_object
       */
      dynamic_global_property_object get_dynamic_global_properties()const;
      chain_properties               get_chain_properties()const;
      price                          get_current_median_history_price()const;
      feed_history_object            get_feed_history()const;
      witness_schedule_object        get_witness_schedule()const;
      hardfork_version               get_hardfork_version()const;
      scheduled_hardfork             get_next_scheduled_hardfork()const;

      //////////
      // Keys //
      //////////

      vector<set<string>> get_key_references( vector<public_key_type> key )const;

      //////////////
      // Accounts //
      //////////////

      vector< extended_account > get_accounts( vector< string > names ) const;

      /**
       *  @return all accounts that referr to the key or account id in their owner or active authorities.
       */
      vector<account_id_type> get_account_references( account_id_type account_id )const;

      /**
       * @brief Get a list of accounts by name
       * @param account_names Names of the accounts to retrieve
       * @return The accounts holding the provided names
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<account_object>> lookup_account_names(const vector<string>& account_names)const;

      /**
       * @brief Get names and IDs for registered accounts
       * @param lower_bound_name Lower bound of the first name to return
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return Map of account names to corresponding IDs
       */
      set<string> lookup_accounts(const string& lower_bound_name, uint32_t limit)const;

      /**
       * @brief Get the total number of accounts registered with the blockchain
       */
      uint64_t get_account_count()const;

      ///////////////
      // Witnesses //
      ///////////////

      /**
       * @brief Get a list of witnesses by ID
       * @param witness_ids IDs of the witnesses to retrieve
       * @return The witnesses corresponding to the provided IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<witness_object>> get_witnesses(const vector<witness_id_type>& witness_ids)const;

      vector<convert_request_object> get_conversion_requests( const string& account_name )const;

      /**
       * @brief Get the witness owned by a given account
       * @param account The name of the account whose witness should be retrieved
       * @return The witness object, or null if the account does not have a witness
       */
      fc::optional< witness_object > get_witness_by_account( string account_name )const;

      /**
       *  This method is used to fetch witnesses with pagination.
       *
       *  @return an array of `count` witnesses sorted by total votes after witness `from` with at most `limit' results.
       */
      vector< witness_object > get_witnesses_by_vote( string from, uint32_t limit )const;

      /**
       * @brief Get names and IDs for registered witnesses
       * @param lower_bound_name Lower bound of the first name to return
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return Map of witness names to corresponding IDs
       */
      set<string> lookup_witness_accounts(const string& lower_bound_name, uint32_t limit)const;

      /**
       * @brief Get the total number of witnesses registered with the blockchain
       */
      uint64_t get_witness_count()const;

      ////////////
      // Market //
      ////////////

      /**
       * @breif Gets the current order book for STEEM:SBD market
       * @param limit Maximum number of orders for each side of the spread to return -- Must not exceed 1000
       */
      order_book get_order_book( uint32_t limit = 1000 )const;


      ////////////////////////////
      // Authority / validation //
      ////////////////////////////

      /// @brief Get a hexdump of the serialized binary form of a transaction
      std::string                   get_transaction_hex(const signed_transaction& trx)const;
      annotated_signed_transaction  get_transaction( transaction_id_type trx_id )const;

      /**
       *  This API will take a partially signed transaction and a set of public keys that the owner has the ability to sign for
       *  and return the minimal subset of public keys that should add signatures to the transaction.
       */
      set<public_key_type> get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const;

      /**
       *  This method will return the set of all public keys that could possibly sign for a given transaction.  This call can
       *  be used by wallets to filter their set of public keys to just the relevant subset prior to calling @ref get_required_signatures
       *  to get the minimum subset.
       */
      set<public_key_type> get_potential_signatures( const signed_transaction& trx )const;

      /**
       * @return true of the @ref trx has all of the required signatures, otherwise throws an exception
       */
      bool           verify_authority( const signed_transaction& trx )const;

      /*
       * @return true if the signers have enough authority to authorize an account
       */
      bool           verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const;

      /**
       *  if permlink is "" then it will return all votes for author
       */
      vector<vote_state> get_active_votes( string author, string permlink )const;
      vector<account_vote> get_account_votes( string voter )const;


      discussion           get_content( string author, string permlink )const;
      vector<discussion>   get_content_replies( string parent, string parent_permlink )const;

      ///@{ tags API
      vector<discussion> get_discussions_by_trending( const discussion_query& query )const;
      vector<discussion> get_discussions_by_created( const discussion_query& query )const;
      vector<discussion> get_discussions_by_active( const discussion_query& query )const;
      vector<discussion> get_discussions_by_cashout( const discussion_query& query )const;
      vector<discussion> get_discussions_by_payout( const discussion_query& query )const;
      vector<discussion> get_discussions_by_votes( const discussion_query& query )const;
      vector<discussion> get_discussions_by_children( const discussion_query& query )const;

      ///@}

      /**
       *  For each of these filters:
       *     Get root content...
       *     Get any content...
       *     Get root content in category..
       *     Get any content in category...
       *
       *  Return discussions
       *     Total Discussion Pending Payout
       *     Last Discussion Update (or reply)... think
       *     Top Discussions by Total Payout
       *
       *  Return content (comments)
       *     Pending Payout Amount
       *     Pending Payout Time
       *     Creation Date
       *
       */
      ///@{



      /**
       *  Return the active discussions with the highest cumulative pending payouts without respect to category, total
       *  pending payout means the pending payout of all children as well.
       */
      vector<discussion>   get_all_discussions_by_total_pending_payout( string start_author, string start_permlink, uint32_t limit )const;
      vector<discussion>   get_all_discussions_by_last_update( string start_author, string start_permlink, uint32_t limit )const;
      vector<discussion>   get_all_discussions_by_votes( string start_author, string start_permlink, uint32_t limit )const;
      vector<discussion>   get_all_discussions_by_created( string start_author, string start_permlink, uint32_t limit )const;
      vector<discussion>   get_all_discussions_by_responses( string start_author, string start_permlink, uint32_t limit )const;
      vector<discussion>   get_all_discussions_by_last_active( string start_author, string start_permlink, uint32_t limit )const;
      vector<discussion>   get_all_discussions_by_cashout_time( string start_author, string start_permlink, uint32_t limit )const;

      vector<discussion>   get_replies_by_last_update( string start_author, string start_permlink, uint32_t limit )const;



      /**
       *  This method is used to fetch all posts/comments by start_author that occur after before_date and start_permlink with up to limit being returned.
       *
       *  If start_permlink is empty then only before_date will be considered. If both are specified the eariler to the two metrics will be used. This
       *  should allow easy pagination.
       */
      vector<discussion>   get_discussions_by_author_before_date( string author, string start_permlink, time_point_sec before_date, uint32_t limit )const;

      /**
       *  Account operations have sequence numbers from 0 to N where N is the most recent operation. This method
       *  returns operations in the range [from-limit, from]
       *
       *  @param from - the absolute sequence number, -1 means most recent, limit is the number of operations before from.
       *  @param limit - the maximum number of items that can be queried (0 to 1000], must be less than from
       */
      map<uint32_t,operation_object> get_account_history( string account, uint64_t from, uint32_t limit )const;

      ////////////////////////////
      // Handlers - not exposed //
      ////////////////////////////
      void on_api_startup();

   private:
      void set_pending_payout( discussion& d )const;
      void set_url( discussion& d )const;
      discussion get_discussion( comment_id_type )const;

      template<typename Index, typename StartItr>
      vector<discussion> get_discussions( const discussion_query& q,
                                          const string& tag,
                                          comment_id_type parent,
                                          const Index& idx, StartItr itr )const;
      comment_id_type get_parent( const discussion_query& q )const;

      void recursively_fetch_content( state& _state, discussion& root, set<string>& referenced_accounts )const;
      std::shared_ptr< database_api_impl > my;
};

} }

FC_REFLECT( steemit::app::order, (order_price)(steem)(sbd)(created) );
FC_REFLECT( steemit::app::order_book, (asks)(bids) );
FC_REFLECT( steemit::app::scheduled_hardfork, (hf_version)(live_time) );

FC_REFLECT( steemit::app::discussion_query, (tag)(filter_tags)(start_author)(start_permlink)(parent_author)(parent_permlink)(limit) );

FC_API(steemit::app::database_api,
   // Subscriptions
   (set_subscribe_callback)
   (set_pending_transaction_callback)
   (set_block_applied_callback)
   (cancel_all_subscriptions)

   // tags
   (get_trending_tags)
   (get_discussions_by_trending)
   (get_discussions_by_created)
   (get_discussions_by_active)
   (get_discussions_by_cashout)
   (get_discussions_by_payout)
   (get_discussions_by_votes)
   (get_discussions_by_children)

   // Blocks and transactions
   (get_block_header)
   (get_block)
   (get_state)
   (get_trending_categories)
   (get_best_categories)
   (get_active_categories)
   (get_recent_categories)

   // Globals
   (get_config)
   (get_dynamic_global_properties)
   (get_chain_properties)
   (get_feed_history)
   (get_current_median_history_price)
   (get_witness_schedule)
   (get_hardfork_version)
   (get_next_scheduled_hardfork)

   // Keys
   (get_key_references)

   // Accounts
   (get_accounts)
   (get_account_references)
   (lookup_account_names)
   (lookup_accounts)
   (get_account_count)
   (get_conversion_requests)
   (get_account_history)

   // Market
   (get_order_book)

   // Authority / validation
   (get_transaction_hex)
   (get_transaction)
   (get_required_signatures)
   (get_potential_signatures)
   (verify_authority)
   (verify_account_authority)

   // votes
   (get_active_votes)
   (get_account_votes)

   // content
   (get_content)
   (get_content_replies)
   (get_all_discussions_by_total_pending_payout)
   (get_all_discussions_by_last_update)
   (get_all_discussions_by_created)
   (get_all_discussions_by_votes)
   (get_all_discussions_by_responses)
   (get_all_discussions_by_last_active)
   (get_all_discussions_by_cashout_time)
   (get_discussions_by_author_before_date)
   (get_replies_by_last_update)


   // Witnesses
   (get_witnesses)
   (get_witness_by_account)
   (get_witnesses_by_vote)
   (lookup_witness_accounts)
   (get_witness_count)
   (get_active_witnesses)
   (get_miner_queue)
)

