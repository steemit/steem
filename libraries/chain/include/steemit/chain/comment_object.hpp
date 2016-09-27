#pragma once

#include <steemit/protocol/authority.hpp>
#include <steemit/protocol/steem_operations.hpp>

#include <steemit/chain//steem_object_types.hpp>
#include <steemit/chain/witness_objects.hpp>

#include <graphene/db/generic_index.hpp>

#include <boost/multi_index/composite_key.hpp>


namespace steemit { namespace chain {

   using namespace graphene::db;

   /**
    *  Used to track the trending categories
    */
   class category_object : public abstract_object<category_object> {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_category_object_type;

         string         name;
         share_type     abs_rshares;
         asset          total_payouts = asset(0, SBD_SYMBOL);
         uint32_t       discussions = 0;
         time_point_sec last_update;
   };

   struct by_name;
   struct by_rshares;
   struct by_total_payouts;
   struct by_last_update;
   typedef multi_index_container<
      category_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< object, object_id_type, &object::id > >,
         ordered_unique< tag< by_name >, member< category_object, string, &category_object::name > >,
         ordered_unique< tag< by_rshares >,
            composite_key< category_object,
               member< category_object, share_type, &category_object::abs_rshares>,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::greater<share_type>, std::less<object_id_type> >
         >,
         ordered_unique< tag< by_total_payouts >,
            composite_key< category_object,
               member< category_object, asset, &category_object::total_payouts>,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::greater<asset>, std::less<object_id_type> >
         >,
         ordered_unique< tag< by_last_update >,
            composite_key< category_object,
               member< category_object, time_point_sec, &category_object::last_update>,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::greater<time_point_sec>, std::less<object_id_type> >
         >
      >
   > category_multi_index_type;

   enum comment_mode
   {
      first_payout,
      second_payout,
      archived
   };

   class comment_object : public abstract_object<comment_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_comment_object_type;

         string            category;
         account_name_type parent_author;
         string            parent_permlink;
         account_name_type author;
         string            permlink;

         string            title = "";
         string            body = "";
         string            json_metadata = "";
         time_point_sec    last_update;
         time_point_sec    created;
         time_point_sec    active; ///< the last time this post was "touched" by voting or reply
         time_point_sec    last_payout;

         uint8_t           depth = 0; ///< used to track max nested depth
         uint32_t          children = 0; ///< used to track the total number of children, grandchildren, etc...

         /**
          *  Used to track the total rshares^2 of all children, this is used for indexing purposes. A discussion
          *  that has a nested comment of high value should promote the entire discussion so that the comment can
          *  be reviewed.
          */
         fc::uint128_t     children_rshares2;

         /// index on pending_payout for "things happning now... needs moderation"
         /// TRENDING = UNCLAIMED + PENDING
         share_type        net_rshares; // reward is proportional to rshares^2, this is the sum of all votes (positive and negative)
         share_type        abs_rshares; /// this is used to track the total abs(weight) of votes for the purpose of calculating cashout_time
         share_type        vote_rshares; /// Total positive rshares from all votes. Used to calculate delta weights. Needed to handle vote changing and removal.

         share_type        children_abs_rshares; /// this is used to calculate cashout time of a discussion.
         time_point_sec    cashout_time; /// 24 hours from the weighted average of vote time
         time_point_sec    max_cashout_time;
         uint64_t          total_vote_weight = 0; /// the total weight of voting rewards, used to calculate pro-rata share of curation payouts

         uint16_t          reward_weight = 0;

         /** tracks the total payout this comment has received over time, measured in SBD */
         asset             total_payout_value = asset(0, SBD_SYMBOL);
         asset             curator_payout_value = asset(0, SBD_SYMBOL);

         share_type        author_rewards = 0;

         int32_t           net_votes = 0;

         comment_id_type   root_comment;

         comment_mode      mode = first_payout;

         asset    max_accepted_payout = asset( 1000000000, SBD_SYMBOL );       /// SBD value of the maximum payout this post will receive
         uint16_t percent_steem_dollars = STEEMIT_100_PERCENT; /// the percent of Steem Dollars to key, unkept amounts will be received as Steem Power
         bool     allow_replies = true;      /// allows a post to disable replies.
         bool     allow_votes   = true;      /// allows a post to receive votes;
         bool     allow_curation_rewards = true;
   };


   /**
    * This index maintains the set of voter/comment pairs that have been used, voters cannot
    * vote on the same comment more than once per payout period.
    */
   class comment_vote_object : public abstract_object<comment_vote_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_comment_vote_object_type;
         account_id_type voter;
         comment_id_type comment;
         uint64_t        weight = 0; ///< defines the score this vote receives, used by vote payout calc. 0 if a negative vote or changed votes.
         int64_t         rshares = 0; ///< The number of rshares this vote is responsible for
         int16_t         vote_percent = 0; ///< The percent weight of the vote
         time_point_sec  last_update; ///< The time of the last update of the vote
         int8_t          num_changes = 0;
   };

   struct by_comment_voter;
   struct by_voter_comment;
   struct by_comment_weight_voter;
   struct by_voter_last_update;
   typedef multi_index_container<
      comment_vote_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< object, object_id_type, &object::id > >,
         ordered_unique< tag< by_comment_voter >,
            composite_key< comment_vote_object,
               member< comment_vote_object, comment_id_type, &comment_vote_object::comment>,
               member< comment_vote_object, account_id_type, &comment_vote_object::voter>
            >
         >,
         ordered_unique< tag< by_voter_comment >,
            composite_key< comment_vote_object,
               member< comment_vote_object, account_id_type, &comment_vote_object::voter>,
               member< comment_vote_object, comment_id_type, &comment_vote_object::comment>
            >
         >,
         ordered_unique< tag< by_voter_last_update >,
            composite_key< comment_vote_object,
               member< comment_vote_object, account_id_type, &comment_vote_object::voter>,
               member< comment_vote_object, time_point_sec, &comment_vote_object::last_update>,
               member< comment_vote_object, comment_id_type, &comment_vote_object::comment>
            >,
            composite_key_compare< std::less< account_id_type >, std::greater< time_point_sec >, std::less<comment_id_type> >
         >,
         ordered_unique< tag< by_comment_weight_voter >,
            composite_key< comment_vote_object,
               member< comment_vote_object, comment_id_type, &comment_vote_object::comment>,
               member< comment_vote_object, uint64_t, &comment_vote_object::weight>,
               member< comment_vote_object, account_id_type, &comment_vote_object::voter>
            >,
            composite_key_compare< std::less< comment_id_type >, std::greater< uint64_t >, std::less<account_id_type> >
         >
      >
   > comment_vote_multi_index_type;


   struct by_cashout_time; /// cashout_time
   struct by_permlink; /// author, perm
   struct by_root;
   struct by_parent;
   struct by_active; /// parent_auth, active
   struct by_pending_payout;
   struct by_total_pending_payout;
   struct by_last_update; /// parent_auth, last_update
   struct by_created; /// parent_auth, last_update
   struct by_payout; /// parent_auth, last_update
   struct by_blog;
   struct by_votes;
   struct by_responses;
   struct by_author_last_update;
   struct by_parent;

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      comment_object,
      indexed_by<
         /// CONSENUSS INDICIES - used by evaluators
         ordered_unique< tag< by_id >, member< object, object_id_type, &object::id > >,
         ordered_unique< tag< by_cashout_time >,
            composite_key< comment_object,
               member< comment_object, time_point_sec, &comment_object::cashout_time>,
               member< object, object_id_type, &object::id >
            >
         >,
         ordered_unique< tag< by_permlink >, /// used by consensus to find posts referenced in ops
            composite_key< comment_object,
               member< comment_object, account_name_type, &comment_object::author >,
               member< comment_object, string, &comment_object::permlink >
            >,
            composite_key_compare< std::less< account_name_type >, std::less< string > >
         >,
         ordered_unique< tag< by_root >,
            composite_key< comment_object,
               member< comment_object, comment_id_type, &comment_object::root_comment >,
               member< object, object_id_type, &object::id >
            >
         >,
         ordered_unique< tag< by_parent >, /// used by consensus to find posts referenced in ops
            composite_key< comment_object,
               member< comment_object, account_name_type, &comment_object::parent_author >,
               member< comment_object, string, &comment_object::parent_permlink >,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less< account_name_type >, std::less< string >, std::less<object_id_type> >
         >
      >
   > comment_multi_index_type;

   typedef generic_index< comment_object,      comment_multi_index_type >       comment_index;
   typedef generic_index< comment_vote_object, comment_vote_multi_index_type >  comment_vote_index;
   typedef generic_index< category_object, category_multi_index_type >          category_index;
} } // steemit::chain

FC_REFLECT_ENUM( steemit::chain::comment_mode, (first_payout)(second_payout)(archived) )

FC_REFLECT_DERIVED( steemit::chain::comment_object, (graphene::db::object),
                    (author)(permlink)
                    (category)(parent_author)(parent_permlink)
                    (title)(body)(json_metadata)(last_update)(created)(active)(last_payout)
                    (depth)(children)(children_rshares2)
                    (net_rshares)(abs_rshares)(vote_rshares)
                    (children_abs_rshares)(cashout_time)(max_cashout_time)
                    (total_vote_weight)(reward_weight)(total_payout_value)(curator_payout_value)(author_rewards)(net_votes)(root_comment)(mode)
                    (max_accepted_payout)(percent_steem_dollars)(allow_replies)(allow_votes)(allow_curation_rewards) )

FC_REFLECT_DERIVED( steemit::chain::comment_vote_object, (graphene::db::object),
                    (voter)(comment)(weight)(rshares)(vote_percent)(last_update)(num_changes) )

FC_REFLECT_DERIVED( steemit::chain::category_object, (graphene::db::object), (name)(abs_rshares)(total_payouts)(discussions)(last_update) );

