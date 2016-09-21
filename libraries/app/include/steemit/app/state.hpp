#pragma once
#include <steemit/app/applied_operation.hpp>

#include <steemit/chain/global_property_object.hpp>
#include <steemit/chain/account_object.hpp>
#include <steemit/chain/steem_objects.hpp>

namespace steemit { namespace app {
   using std::string;
   using std::vector;
   using namespace steemit::chain;

   struct extended_limit_order : public limit_order_object
   {
      extended_limit_order(){}
      extended_limit_order( const limit_order_object& o ):limit_order_object(o){}

      double real_price  = 0;
      bool   rewarded    = false;
   };

   struct discussion_index {
      string         category; /// category by which everything is filtered
      vector<string> trending; /// pending lifetime payout
      vector<string> trending30; /// pending lifetime payout
      vector<string> created; /// creation date
      vector<string> responses; /// creation date
      vector<string> updated; /// creation date
      vector<string> active; /// last update or reply
      vector<string> votes; /// last update or reply
      vector<string> cashout; /// last update or reply
      vector<string> maturing; /// about to be paid out
      vector<string> best; /// total lifetime payout
      vector<string> hot; /// total lifetime payout
      vector<string> promoted; /// pending lifetime payout
   };
   struct category_index {
      vector<string> trending; /// pending payouts
      vector<string> active; /// recent activity
      vector<string> recent; /// recently created
      vector<string> best; /// total lifetime payout
   };
   struct vote_state {
      string         voter;
      uint64_t       weight = 0;
      int64_t        rshares = 0;
      int16_t        percent = 0;
      share_type     reputation = 0;
      time_point_sec time;
   };

   struct account_vote {
      string         authorperm;
      uint64_t       weight = 0;
      int64_t        rshares = 0;
      int16_t        percent = 0;
      time_point_sec time;
   };

   struct  discussion : public comment_object {
      discussion( const comment_object& o ):comment_object(o){}
      discussion(){}

      string                      url; /// /category/@rootauthor/root_permlink#author/permlink
      string                      root_title;
      asset                       pending_payout_value; ///< sbd
      asset                       total_pending_payout_value; ///< sbd including replies
      vector<vote_state>          active_votes;
      vector<string>              replies; ///< author/slug mapping
      share_type                  author_reputation = 0;
      asset                       promoted = asset(0, SBD_SYMBOL);
      optional<string>            first_reblogged_by;
   };

   /**
    *  Convert's vesting shares
    */
   struct extended_account : public account_object {
      extended_account(){}
      extended_account( const account_object& a ):account_object(a){}

      asset                                   vesting_balance; /// convert vesting_shares to vesting steem
      share_type                              reputation = 0;
      map<uint64_t,applied_operation>         transfer_history; /// transfer to/from vesting
      map<uint64_t,applied_operation>         market_history; /// limit order / cancel / fill
      map<uint64_t,applied_operation>         post_history;
      map<uint64_t,applied_operation>         vote_history;
      map<uint64_t,applied_operation>         other_history;
      set<string>                             witness_votes;

      optional<map<uint32_t,extended_limit_order>> open_orders;
      optional<vector<string>>                posts; /// permlinks for this user
      optional<vector<string>>                blog; /// blog posts for this user
      optional<vector<string>>                feed; /// feed posts for this user
      optional<vector<string>>                recent_replies; /// blog posts for this user
      map<string,vector<string>>              blog_category; /// blog posts for this user
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
        string                        current_route;

        dynamic_global_property_object props;

        /**
         *  Tracks the top categories by name, any category in this index
         *  will have its full status stored in the categories map.
         */
        app::category_index           category_idx;

        /**
         * "" is the global discussion index, otherwise the indicies are ranked by category
         */
        map<string, discussion_index> discussion_idx;

        map<string, category_object>  categories;
        /**
         *  map from account/slug to full nested discussion
         */
        map<string, discussion>       content;
        map<string, extended_account> accounts;

        /**
         * The list of miners who are queued to produce work
         */
        vector<account_name_type>     pow_queue;
        map<string, witness_object>   witnesses;
        witness_schedule_object       witness_schedule;
        price                         feed_price;
        string                        error;
        optional<market>              market_data;
   };

} }

FC_REFLECT_DERIVED( steemit::app::extended_account,
                   (steemit::chain::account_object),
                   (vesting_balance)(reputation)
                   (transfer_history)(market_history)(post_history)(vote_history)(other_history)(witness_votes)(open_orders)(posts)(feed)(blog)(recent_replies)(blog_category)(recommended) )


FC_REFLECT( steemit::app::vote_state, (voter)(weight)(rshares)(percent)(reputation)(time) );
FC_REFLECT( steemit::app::account_vote, (authorperm)(weight)(rshares)(percent)(time) );

FC_REFLECT( steemit::app::discussion_index, (category)(trending)(trending30)(updated)(created)(responses)(active)(votes)(maturing)(best)(hot)(promoted)(cashout) )
FC_REFLECT( steemit::app::category_index, (trending)(active)(recent)(best) )
FC_REFLECT_DERIVED( steemit::app::discussion, (steemit::chain::comment_object), (url)(root_title)(pending_payout_value)(total_pending_payout_value)(active_votes)(replies)(author_reputation)(promoted)(first_reblogged_by) )

FC_REFLECT( steemit::app::state, (current_route)(props)(category_idx)(categories)(content)(accounts)(pow_queue)(witnesses)(discussion_idx)(witness_schedule)(feed_price)(error)(market_data) )

FC_REFLECT_DERIVED( steemit::app::extended_limit_order, (steemit::chain::limit_order_object), (real_price)(rewarded) )
FC_REFLECT( steemit::app::order_history_item, (time)(type)(sbd_quantity)(steem_quantity)(real_price) );
FC_REFLECT( steemit::app::market, (bids)(asks)(history)(price_history)(available_candlesticks)(available_zoom)(current_candlestick)(current_zoom) )
FC_REFLECT( steemit::app::candle_stick, (open_time)(period)(high)(low)(open)(close)(steem_volume)(dollar_volume) );
