#pragma once
#include <steem/chain/steem_fwd.hpp>

#include <steem/protocol/authority.hpp>
#include <steem/protocol/steem_operations.hpp>

#include <steem/chain/steem_object_types.hpp>
#include <steem/chain/witness_objects.hpp>


namespace steem { namespace chain {

   using protocol::beneficiary_route_type;
   using chainbase::t_vector;
   using chainbase::t_pair;
#ifdef STEEM_ENABLE_SMT
   using protocol::votable_asset_info;
#endif

   struct strcmp_less
   {
      bool operator()( const shared_string& a, const shared_string& b )const
      {
         return less( a.c_str(), b.c_str() );
      }

#ifndef ENABLE_STD_ALLOCATOR
      bool operator()( const shared_string& a, const string& b )const
      {
         return less( a.c_str(), b.c_str() );
      }

      bool operator()( const string& a, const shared_string& b )const
      {
         return less( a.c_str(), b.c_str() );
      }
#endif

      private:
         inline bool less( const char* a, const char* b )const
         {
            return std::strcmp( a, b ) < 0;
         }
   };

   class comment_object : public object < comment_object_type, comment_object >
   {
      STEEM_STD_ALLOCATOR_CONSTRUCTOR( comment_object )

      public:
         template< typename Constructor, typename Allocator >
         comment_object( Constructor&& c, allocator< Allocator > a )
            :category( a ), parent_permlink( a ), permlink( a ), beneficiaries( a )
#ifdef STEEM_ENABLE_SMT
            , allowed_vote_assets( a )
#endif
         {
            c( *this );
         }

         id_type           id;

         shared_string     category;
         account_name_type parent_author;
         shared_string     parent_permlink;
         account_name_type author;
         shared_string     permlink;

         time_point_sec    last_update;
         time_point_sec    created;
         time_point_sec    active; ///< the last time this post was "touched" by voting or reply
         time_point_sec    last_payout;

         uint16_t          depth = 0; ///< used to track max nested depth
         uint32_t          children = 0; ///< used to track the total number of children, grandchildren, etc...

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
         asset             beneficiary_payout_value = asset( 0, SBD_SYMBOL );

         share_type        author_rewards = 0;

         int32_t           net_votes = 0;

         id_type           root_comment;

         asset             max_accepted_payout = asset( 1000000000, SBD_SYMBOL );       /// SBD value of the maximum payout this post will receive
         uint16_t          percent_steem_dollars = STEEM_100_PERCENT; /// the percent of Steem Dollars to key, unkept amounts will be received as Steem Power
         bool              allow_replies = true;      /// allows a post to disable replies.
         bool              allow_votes   = true;      /// allows a post to receive votes;
         bool              allow_curation_rewards = true;

         using t_beneficiaries = t_vector< beneficiary_route_type >;
         t_beneficiaries   beneficiaries;
#ifdef STEEM_ENABLE_SMT
         using t_votable_assets = t_vector< t_pair< asset_symbol_type, votable_asset_info > >;
         t_votable_assets  allowed_vote_assets;
#endif
   };

   class comment_content_object : public object< comment_content_object_type, comment_content_object >
   {
      STEEM_STD_ALLOCATOR_CONSTRUCTOR( comment_content_object )

      public:
         template< typename Constructor, typename Allocator >
         comment_content_object( Constructor&& c, allocator< Allocator > a ) :
            title( a ), body( a ), json_metadata( a )
         {
            c( *this );
         }

         id_type           id;

         comment_id_type   comment;

         shared_string     title;
         shared_string     body;
         shared_string     json_metadata;
   };

   /**
    * This index maintains the set of voter/comment pairs that have been used, voters cannot
    * vote on the same comment more than once per payout period.
    */
   class comment_vote_object : public object< comment_vote_object_type, comment_vote_object>
   {
      STEEM_STD_ALLOCATOR_CONSTRUCTOR( comment_vote_object )

      public:
         template< typename Constructor, typename Allocator >
         comment_vote_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         id_type           id;

         account_id_type   voter;
         comment_id_type   comment;
         uint64_t          weight = 0; ///< defines the score this vote receives, used by vote payout calc. 0 if a negative vote or changed votes.
         int64_t           rshares = 0; ///< The number of rshares this vote is responsible for
         int16_t           vote_percent = 0; ///< The percent weight of the vote
         time_point_sec    last_update; ///< The time of the last update of the vote
         int8_t            num_changes = 0;
   };

   struct by_comment_voter;
   struct by_voter_comment;
   typedef multi_index_container<
      comment_vote_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< comment_vote_object, comment_vote_id_type, &comment_vote_object::id > >,
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
         >
      >,
      allocator< comment_vote_object >
   > comment_vote_index;


   struct by_cashout_time; /// cashout_time
   struct by_permlink; /// author, perm
   struct by_root;
   struct by_parent;
   struct by_last_update; /// parent_auth, last_update
   struct by_author_last_update;

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      comment_object,
      indexed_by<
         /// CONSENSUS INDICES - used by evaluators
         ordered_unique< tag< by_id >, member< comment_object, comment_id_type, &comment_object::id > >,
         ordered_unique< tag< by_cashout_time >,
            composite_key< comment_object,
               member< comment_object, time_point_sec, &comment_object::cashout_time>,
               member< comment_object, comment_id_type, &comment_object::id >
            >
         >,
         ordered_unique< tag< by_permlink >, /// used by consensus to find posts referenced in ops
            composite_key< comment_object,
               member< comment_object, account_name_type, &comment_object::author >,
               member< comment_object, shared_string, &comment_object::permlink >
            >,
            composite_key_compare< std::less< account_name_type >, strcmp_less >
         >,
         ordered_unique< tag< by_root >,
            composite_key< comment_object,
               member< comment_object, comment_id_type, &comment_object::root_comment >,
               member< comment_object, comment_id_type, &comment_object::id >
            >
         >,
         ordered_unique< tag< by_parent >, /// used by consensus to find posts referenced in ops
            composite_key< comment_object,
               member< comment_object, account_name_type, &comment_object::parent_author >,
               member< comment_object, shared_string, &comment_object::parent_permlink >,
               member< comment_object, comment_id_type, &comment_object::id >
            >,
            composite_key_compare< std::less< account_name_type >, strcmp_less, std::less< comment_id_type > >
         >
         /// NON_CONSENSUS INDICIES - used by APIs
#ifndef IS_LOW_MEM
         ,
         ordered_unique< tag< by_last_update >,
            composite_key< comment_object,
               member< comment_object, account_name_type, &comment_object::parent_author >,
               member< comment_object, time_point_sec, &comment_object::last_update >,
               member< comment_object, comment_id_type, &comment_object::id >
            >,
            composite_key_compare< std::less< account_name_type >, std::greater< time_point_sec >, std::less< comment_id_type > >
         >,
         ordered_unique< tag< by_author_last_update >,
            composite_key< comment_object,
               member< comment_object, account_name_type, &comment_object::author >,
               member< comment_object, time_point_sec, &comment_object::last_update >,
               member< comment_object, comment_id_type, &comment_object::id >
            >,
            composite_key_compare< std::less< account_name_type >, std::greater< time_point_sec >, std::less< comment_id_type > >
         >
#endif
      >,
      allocator< comment_object >
   > comment_index;

   struct by_comment;

   typedef multi_index_container<
      comment_content_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< comment_content_object, comment_content_id_type, &comment_content_object::id > >,
         ordered_unique< tag< by_comment >, member< comment_content_object, comment_id_type, &comment_content_object::comment > >
      >,
      allocator< comment_content_object >
   > comment_content_index;

} } // steem::chain

#ifdef ENABLE_STD_ALLOCATOR
namespace mira {

template<> struct is_static_length< steem::chain::comment_vote_object > : public boost::true_type {};

} // mira
#endif

FC_REFLECT( steem::chain::comment_object,
             (id)(author)(permlink)
             (category)(parent_author)(parent_permlink)
             (last_update)(created)(active)(last_payout)
             (depth)(children)
             (net_rshares)(abs_rshares)(vote_rshares)
             (children_abs_rshares)(cashout_time)(max_cashout_time)
             (total_vote_weight)(reward_weight)(total_payout_value)(curator_payout_value)(beneficiary_payout_value)(author_rewards)(net_votes)(root_comment)
             (max_accepted_payout)(percent_steem_dollars)(allow_replies)(allow_votes)(allow_curation_rewards)
             (beneficiaries)
#ifdef STEEM_ENABLE_SMT
             (allowed_vote_assets)
#endif
          )

CHAINBASE_SET_INDEX_TYPE( steem::chain::comment_object, steem::chain::comment_index )

FC_REFLECT( steem::chain::comment_content_object,
            (id)(comment)(title)(body)(json_metadata) )
CHAINBASE_SET_INDEX_TYPE( steem::chain::comment_content_object, steem::chain::comment_content_index )

FC_REFLECT( steem::chain::comment_vote_object,
             (id)(voter)(comment)(weight)(rshares)(vote_percent)(last_update)(num_changes)
          )
CHAINBASE_SET_INDEX_TYPE( steem::chain::comment_vote_object, steem::chain::comment_vote_index )

namespace helpers
{
   using steem::chain::shared_string;

   template <>
   class index_statistic_provider<steem::chain::comment_index>
   {
   public:
      typedef steem::chain::comment_index IndexType;
      typedef typename steem::chain::comment_object::t_beneficiaries t_beneficiaries;
#ifdef STEEM_ENABLE_SMT
      typedef typename steem::chain::comment_object::t_votable_assets t_votable_assets;
#endif
      index_statistic_info gather_statistics(const IndexType& index, bool onlyStaticInfo) const
      {
         index_statistic_info info;
         gather_index_static_data(index, &info);

         if(onlyStaticInfo == false)
         {
            for(const auto& o : index)
            {
               info._item_additional_allocation += o.category.capacity()*sizeof(shared_string::value_type);
               info._item_additional_allocation += o.parent_permlink.capacity()*sizeof(shared_string::value_type);
               info._item_additional_allocation += o.permlink.capacity()*sizeof(shared_string::value_type);
               info._item_additional_allocation += o.beneficiaries.capacity()*sizeof(t_beneficiaries::value_type);
#ifdef STEEM_ENABLE_SMT
               info._item_additional_allocation += o.allowed_vote_assets.capacity()*sizeof(t_votable_assets::value_type);
#endif
            }
         }

         return info;
      }
   };

   template <>
   class index_statistic_provider<steem::chain::comment_content_index>
   {
   public:
      typedef steem::chain::comment_content_index IndexType;

      index_statistic_info gather_statistics(const IndexType& index, bool onlyStaticInfo) const
      {
         index_statistic_info info;
         gather_index_static_data(index, &info);

         if(onlyStaticInfo == false)
         {
            for(const auto& o : index)
            {
               info._item_additional_allocation += o.title.capacity()*sizeof(shared_string::value_type);
               info._item_additional_allocation += o.body.capacity()*sizeof(shared_string::value_type);
               info._item_additional_allocation += o.json_metadata.capacity()*sizeof(shared_string::value_type);
            }
         }

         return info;
      }
   };

} /// namespace helpers
