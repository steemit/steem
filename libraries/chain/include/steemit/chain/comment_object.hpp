#pragma once

#include <steemit/chain/protocol/authority.hpp>
#include <steemit/chain/protocol/types.hpp>
#include <steemit/chain/protocol/steem_operations.hpp>
#include <steemit/chain/witness_objects.hpp>

#include <graphene/db/generic_index.hpp>

#include <boost/multi_index/composite_key.hpp>


namespace steemit { namespace chain {

   using namespace graphene::db;

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


   class comment_object : public abstract_object<comment_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_comment_object_type;

         string            category;
         string            parent_author;
         string            parent_permlink;
         string            author;
         string            permlink;

         string            title;
         string            body;
         string            json_metadata;
         time_point_sec    last_update;
         time_point_sec    created;

         uint8_t           depth = 0; 
         uint32_t          children = 0; 

         fc::uint128_t     children_rshares2;


         share_type        net_rshares; 
         share_type        abs_rshares; 
         time_point_sec    cashout_time;
         uint64_t          total_vote_weight = 0; 

         asset             total_payout_value = asset(0, SBD_SYMBOL);
   };


   class comment_vote_object : public abstract_object<comment_vote_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_comment_vote_object_type;
         account_id_type voter;
         comment_id_type comment;
         uint64_t        weight = 0; 
   };

   struct by_comment_voter;
   struct by_comment_weight_voter;
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



   struct by_permlink;
   struct by_pending_payout;
   struct by_payout;
   struct by_created;
   struct by_parent_pending_payout;
   struct by_parent_payout;
   struct by_parent_date;
   struct by_author_pending_payout;
   struct by_author_payout;
   struct by_author_date;
   struct by_parent;
   struct by_parent_created;
   struct by_cashout;
   struct by_parent_cashout;
   struct by_pending_payout; /// rshares
   struct by_parent_pending_payout;
   struct by_payout; /// rshares
   struct by_parent_payout;
   struct by_total_pending_payout;
   struct by_total_pending_payout_in_category;
   struct by_last_update;
   struct by_last_update_in_category;
   struct by_cashout_time;
   struct by_cashout_time_in_category;
   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      comment_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< object, object_id_type, &object::id > >,
         ordered_unique< tag< by_cashout_time >,
            composite_key< comment_object,
               member< comment_object, time_point_sec, &comment_object::cashout_time>,
               member< object, object_id_type, &object::id >
            >
         >,
         ordered_unique< tag< by_cashout_time_in_category >,
            composite_key< comment_object,
               member< comment_object, string, &comment_object::category >,
               member< comment_object, time_point_sec, &comment_object::cashout_time >,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less< string >, std::less<time_point_sec>, std::less<object_id_type> >
         >,
         ordered_unique< tag< by_permlink >,
            composite_key< comment_object,
               member< comment_object, string, &comment_object::author >,
               member< comment_object, string, &comment_object::permlink >
            >,
            composite_key_compare< std::less< string >, std::less< string > >
         >,
         ordered_unique< tag< by_parent >,
            composite_key< comment_object,
               member< comment_object, string, &comment_object::parent_author >,
               member< comment_object, string, &comment_object::parent_permlink >,
               member< object, object_id_type, &object::id >
            >
         >,
         ordered_unique< tag< by_parent_created >,
            composite_key< comment_object,
               member< comment_object, string, &comment_object::parent_author >,
               member< comment_object, string, &comment_object::parent_permlink >,
               member< comment_object, time_point_sec, &comment_object::created >,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less< string >, std::less< string >, std::greater<time_point_sec>, std::less<object_id_type> >
         >,
         ordered_unique< tag< by_created >,
            composite_key< comment_object,
               member< comment_object, time_point_sec, &comment_object::created >,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::greater<time_point_sec>, std::less<object_id_type> >
         >,
         ordered_unique< tag< by_pending_payout >,
            composite_key< comment_object,
               member< comment_object, string, &comment_object::parent_author >,
               member< comment_object, share_type, &comment_object::net_rshares >,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less<string>, std::greater<share_type>, std::less<object_id_type> >
         >,
         ordered_unique< tag< by_total_pending_payout >,
            composite_key< comment_object,
               member< comment_object, string, &comment_object::parent_author >, /// parent author of "" is root topic
               member< comment_object, fc::uint128_t, &comment_object::children_rshares2 >,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less<string>, std::greater<fc::uint128_t>, std::less<object_id_type> >
         >,
         ordered_unique< tag<by_last_update>,
            composite_key< comment_object,
               member< comment_object, string, &comment_object::parent_author >, /// parent author of "" is root topic
               member< comment_object, time_point_sec, &comment_object::last_update >,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less<string>, std::greater<time_point_sec>, std::less<object_id_type> >
         >,
         ordered_unique< tag< by_total_pending_payout_in_category >,
            composite_key< comment_object,
               member< comment_object, string, &comment_object::parent_author >, /// parent author of "" is root topic
               member< comment_object, string, &comment_object::parent_permlink >, /// permlink is the category
               member< comment_object, fc::uint128_t, &comment_object::children_rshares2 >,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less<string>, std::less<string>, std::greater<fc::uint128_t>, std::less<object_id_type> >
         >,
         ordered_unique< tag< by_last_update_in_category >,
            composite_key< comment_object,
               member< comment_object, string, &comment_object::parent_author >, /// parent author of "" is root topic
               member< comment_object, string, &comment_object::parent_permlink >, /// permlink is the category
               member< comment_object, time_point_sec, &comment_object::last_update >,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less<string>, std::less<string>, std::greater<time_point_sec>, std::less<object_id_type> >
         >,
         ordered_unique< tag< by_parent_pending_payout >,
            composite_key< comment_object,
               member< comment_object, string, &comment_object::parent_author >,
               member< comment_object, string, &comment_object::parent_permlink >,
               member< comment_object, share_type, &comment_object::net_rshares >,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less< string >, std::less< string >, std::greater<share_type>, std::less<object_id_type> >
         >,
         ordered_unique< tag< by_payout >,
            composite_key< comment_object,
               member< comment_object, asset, &comment_object::total_payout_value >,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::greater<asset>, std::less<object_id_type> >
         >,
         ordered_unique< tag< by_parent_payout >,
            composite_key< comment_object,
               member< comment_object, string, &comment_object::parent_author >,
               member< comment_object, string, &comment_object::parent_permlink >,
               member< comment_object, asset, &comment_object::total_payout_value >,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less< string >, std::less< string >, std::greater<asset>, std::less<object_id_type> >
         >,
         ordered_unique< tag< by_author_date >,
            composite_key< comment_object,
               member< comment_object, string, &comment_object::author >,
               member< comment_object, time_point_sec, &comment_object::created >,
               member< object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less< string >, std::greater<time_point_sec>, std::less<object_id_type> >
         >
      >
   > comment_multi_index_type;

   typedef generic_index< comment_object,      comment_multi_index_type >      comment_index;
   typedef generic_index< comment_vote_object, comment_vote_multi_index_type > comment_vote_index;
   typedef generic_index< category_object, category_multi_index_type >         category_index;
} } // steemit::chain

FC_REFLECT_DERIVED( steemit::chain::comment_object, (graphene::db::object),
                    (author)(permlink)
                    (category)(parent_author)(parent_permlink)
                    (title)(body)(json_metadata)(last_update)(created)
                    (depth)(children)(children_rshares2)
                    (net_rshares)(abs_rshares)(cashout_time)(total_vote_weight)(total_payout_value) )

FC_REFLECT_DERIVED( steemit::chain::comment_vote_object, (graphene::db::object),
                    (voter)(comment)(weight) )

FC_REFLECT_DERIVED( steemit::chain::category_object, (graphene::db::object), (name)(abs_rshares)(total_payouts)(discussions)(last_update) );
