#pragma once
#include <steemit/chain/global_property_object.hpp>
#include <steemit/chain/account_object.hpp>
#include <steemit/chain/steem_objects.hpp>

namespace steemit { namespace app {
   using std::string;
   using std::vector;
   using namespace steemit::chain;

   struct discussion_index {
      string         category; /// category by which everything is filtered
      vector<string> trending; /// pending lifetime payout
      vector<string> recent; /// creation date
      vector<string> active; /// last update or reply
      vector<string> maturing; /// about to be paid out
      vector<string> best; /// total lifetime payout
   };
   struct category_index {
      vector<string> trending; /// pending payouts
      vector<string> active; /// recent activity 
      vector<string> recent; /// recently created
      vector<string> best; /// total lifetime payout
   };
   struct vote_state {
      string   voter;
      uint64_t weight;
   };

   struct account_vote {
      string   authorperm;
      uint64_t weight;
   };

   struct  discussion : public comment_object {
      discussion( const comment_object& o ):comment_object(o){}
      discussion(){}

      asset                       pending_payout_value; ///< sbd
      asset                       total_pending_payout_value; ///< sbd including replies
      vector<vote_state>          active_votes;
      vector<string>              replies; ///< author/slug mapping
   };

   /**
    *  Convert's vesting shares
    */
   struct extended_account : public account_object {
      extended_account(){}
      extended_account( const account_object& a ):account_object(a){}

      asset                    vesting_balance; /// convert vesting_shares to vesting steem
      map<uint64_t,operation_object>  transfer_history; /// transfer to/from vesting
      map<uint64_t,operation_object>  market_history; /// limit order / cancel / fill
      map<uint64_t,operation_object>  post_history;
      map<uint64_t,operation_object>  vote_history; 
      map<uint64_t,operation_object>  other_history; 
      vector<string>                  posts; /// permlinks for this user
      vector<string>                  blog; /// blog posts for this user
      map<string,vector<string>>      blog_category; /// blog posts for this user
   };


#if 0
   struct extended_limit_order : public limit_order_object 
   {
      double price;
   };

   /**
    *  This is provided to help 3rd parites convert price ratio's to real, and
    *  to normalize everything to be priced in SBD
    */
   struct extended_price {
      double price; /// SBD per STEEM 
      price  ratio; /// the exact ratio
   };

   struct order_history_item {
      time_point_sec time;
      string         type; // buy or sell
      asset          sbd_quantity;
      asset          steem_quantity;
      double         price;
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
#endif 

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
        vector<string>                pow_queue;
        map<string, witness_object>   witnesses;
        witness_schedule_object       witness_schedule;
   };

} }

FC_REFLECT_DERIVED( steemit::app::extended_account, 
                   (steemit::chain::account_object), 
                   (vesting_balance)
                   (transfer_history)(market_history)(post_history)(vote_history)(other_history)(posts)(blog)(blog_category) )


FC_REFLECT( steemit::app::vote_state, (voter)(weight) );
FC_REFLECT( steemit::app::account_vote, (authorperm)(weight) );

FC_REFLECT( steemit::app::discussion_index, (category)(trending)(recent)(active)(maturing)(best) )
FC_REFLECT( steemit::app::category_index, (trending)(active)(recent)(best) )
FC_REFLECT( steemit::app::state, (current_route)(props)(category_idx)(categories)(content)(accounts)(pow_queue)(witnesses)(discussion_idx)(witness_schedule) )
FC_REFLECT_DERIVED( steemit::app::discussion, (steemit::chain::comment_object), (pending_payout_value)(total_pending_payout_value)(active_votes)(replies) )
