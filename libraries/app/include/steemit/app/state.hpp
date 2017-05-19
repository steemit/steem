#pragma once
#include <steemit/app/applied_operation.hpp>
#include <steemit/app/steem_api_objects.hpp>

#include <steemit/chain/global_property_object.hpp>
#include <steemit/chain/account_object.hpp>
#include <steemit/chain/steem_objects.hpp>

namespace steemit { namespace app {
   using std::string;
   using std::vector;

   struct extended_limit_order : public limit_order_api_obj
   {
      extended_limit_order(){}
      extended_limit_order( const limit_order_object& o ):limit_order_api_obj(o){}

      double real_price  = 0;
      bool   rewarded    = false;
   };

   struct discussion_index
   {
      string           category;    /// category by which everything is filtered
      vector< string > trending;    /// trending posts over the last 24 hours
      vector< string > payout;      /// pending posts by payout
      vector< string > payout_comments; /// pending comments by payout
      vector< string > trending30;  /// pending lifetime payout
      vector< string > created;     /// creation date
      vector< string > responses;   /// creation date
      vector< string > updated;     /// creation date
      vector< string > active;      /// last update or reply
      vector< string > votes;       /// last update or reply
      vector< string > cashout;     /// last update or reply
      vector< string > maturing;    /// about to be paid out
      vector< string > best;        /// total lifetime payout
      vector< string > hot;         /// total lifetime payout
      vector< string > promoted;    /// pending lifetime payout
   };

   struct tag_index
   {
      vector< string > trending; /// pending payouts
   };

   struct vote_state
   {
      string         voter;
      uint64_t       weight = 0;
      int64_t        rshares = 0;
      int16_t        percent = 0;
      share_type     reputation = 0;
      time_point_sec time;
   };

   struct account_vote
   {
      string         authorperm;
      uint64_t       weight = 0;
      int64_t        rshares = 0;
      int16_t        percent = 0;
      time_point_sec time;
   };

   struct  discussion : public comment_api_obj {
      discussion( const comment_object& o ):comment_api_obj(o){}
      discussion(){}

      string                      url; /// /category/@rootauthor/root_permlink#author/permlink
      string                      root_title;
      asset                       pending_payout_value; ///< sbd
      asset                       total_pending_payout_value; ///< sbd including replies
      vector<vote_state>          active_votes;
      vector<string>              replies; ///< author/slug mapping
      share_type                  author_reputation = 0;
      asset                       promoted = asset(0, SBD_SYMBOL);
      uint32_t                    body_length = 0;
      vector<account_name_type>   reblogged_by;
      optional<account_name_type> first_reblogged_by;
      optional<time_point_sec>    first_reblogged_on;
   };

   /**
    *  Convert's vesting shares
    */
   struct extended_account : public account_api_obj
   {
      extended_account(){}
      extended_account( const account_object& a, const database& db ):account_api_obj( a, db ){}

      asset                                   vesting_balance; /// convert vesting_shares to vesting steem
      share_type                              reputation = 0;
      map<uint64_t,applied_operation>         transfer_history; /// transfer to/from vesting
      map<uint64_t,applied_operation>         market_history; /// limit order / cancel / fill
      map<uint64_t,applied_operation>         post_history;
      map<uint64_t,applied_operation>         vote_history;
      map<uint64_t,applied_operation>         other_history;
      set<string>                             witness_votes;
      vector<pair<string,uint32_t>>            tags_usage;
      vector<pair<account_name_type,uint32_t>> guest_bloggers;

      optional<map<uint32_t,extended_limit_order>> open_orders;
      optional<vector<string>>                comments; /// permlinks for this user
      optional<vector<string>>                blog; /// blog posts for this user
      optional<vector<string>>                feed; /// feed posts for this user
      optional<vector<string>>                recent_replies; /// blog posts for this user
      optional<vector<string>>                recommended; /// posts recommened for this user
   };



   struct candle_stick {
      time_point_sec  open_time;
      uint32_t        period = 0;
      double          high = 0;
      double          low = 0;
      double          open = 0;
      double          close = 0;
      double          steem_volume = 0;
      double          dollar_volume = 0;
   };

   struct order_history_item {
      time_point_sec time;
      string         type; // buy or sell
      asset          sbd_quantity;
      asset          steem_quantity;
      double         real_price = 0;
   };

   struct market {
      vector<extended_limit_order> bids;
      vector<extended_limit_order> asks;
      vector<order_history_item>   history;
      vector<int>                  available_candlesticks;
      vector<int>                  available_zoom;
      int                          current_candlestick = 0;
      int                          current_zoom = 0;
      vector<candle_stick>         price_history;
   };

   /**
    *  This struct is designed
    */
   struct state {
        string                            current_route;

        dynamic_global_property_api_obj   props;

        app::tag_index                    tag_idx;

        /**
         * "" is the global discussion index
         */
        map<string, discussion_index>     discussion_idx;

        map< string, tag_api_obj >        tags;

        /**
         *  map from account/slug to full nested discussion
         */
        map< string, discussion >         content;
        map< string, extended_account >   accounts;

        /**
         * The list of miners who are queued to produce work
         */
        vector< account_name_type >       pow_queue;
        map< string, witness_api_obj >    witnesses;
        witness_schedule_api_obj          witness_schedule;
        price                             feed_price;
        string                            error;
        optional< market >                market_data;
   };

} }

FC_REFLECT_DERIVED( steemit::app::extended_account,
                   (steemit::app::account_api_obj),
                   (vesting_balance)(reputation)
                   (transfer_history)(market_history)(post_history)(vote_history)(other_history)(witness_votes)(tags_usage)(guest_bloggers)(open_orders)(comments)(feed)(blog)(recent_replies)(recommended) )


FC_REFLECT( steemit::app::vote_state, (voter)(weight)(rshares)(percent)(reputation)(time) );
FC_REFLECT( steemit::app::account_vote, (authorperm)(weight)(rshares)(percent)(time) );

FC_REFLECT( steemit::app::discussion_index, (category)(trending)(payout)(payout_comments)(trending30)(updated)(created)(responses)(active)(votes)(maturing)(best)(hot)(promoted)(cashout) )
FC_REFLECT( steemit::app::tag_index, (trending) )
FC_REFLECT_DERIVED( steemit::app::discussion, (steemit::app::comment_api_obj), (url)(root_title)(pending_payout_value)(total_pending_payout_value)(active_votes)(replies)(author_reputation)(promoted)(body_length)(reblogged_by)(first_reblogged_by)(first_reblogged_on) )

FC_REFLECT( steemit::app::state, (current_route)(props)(tag_idx)(tags)(content)(accounts)(pow_queue)(witnesses)(discussion_idx)(witness_schedule)(feed_price)(error)(market_data) )

FC_REFLECT_DERIVED( steemit::app::extended_limit_order, (steemit::app::limit_order_api_obj), (real_price)(rewarded) )
FC_REFLECT( steemit::app::order_history_item, (time)(type)(sbd_quantity)(steem_quantity)(real_price) );
FC_REFLECT( steemit::app::market, (bids)(asks)(history)(price_history)(available_candlesticks)(available_zoom)(current_candlestick)(current_zoom) )
FC_REFLECT( steemit::app::candle_stick, (open_time)(period)(high)(low)(open)(close)(steem_volume)(dollar_volume) );
