#pragma once

#include <steemit/chain/protocol/authority.hpp>
#include <steemit/chain/protocol/types.hpp>
#include <steemit/chain/protocol/steem_operations.hpp>
#include <steemit/chain/witness_objects.hpp>

#include <steemit/chain/object/convert_request_object.hpp>
#include <steemit/chain/object/feed_history_object.hpp>
#include <steemit/chain/object/limit_order_object.hpp>
#include <steemit/chain/object/liquidity_reward_balance_object.hpp>

#include <graphene/db/generic_index.hpp>

#include <boost/multi_index/composite_key.hpp>
#include <boost/multiprecision/cpp_int.hpp>

namespace steemit { namespace chain {

   using namespace graphene::db;

   /*
   class feed_history_object  : public core::feed_history_object, abstract_object<feed_history_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_feed_history_object_type;
   };
   */

   /* TODO delete this
   struct by_price;
   struct by_expiration;
   struct by_account;
   typedef multi_index_container<
      limit_order_object,
      indexed_by<
      >
   > limit_order_multi_index_type;

   struct by_owner;
   struct by_conversion_date;
   typedef multi_index_container<
      convert_request_object,
      indexed_by<
      >
   > convert_request_index_type;

   struct by_owner;
   struct by_volume_weight;

   typedef multi_index_container<
      liquidity_reward_balance_object,
      indexed_by<
      >
   > liquidity_reward_balance_index_type;
   */

   /**
    * @ingroup object_index
    */
   /*
   typedef generic_index< convert_request_object,           convert_request_index_type >           convert_index;
   typedef generic_index< limit_order_object,               limit_order_multi_index_type >         limit_order_index;
   typedef generic_index< liquidity_reward_balance_object,  liquidity_reward_balance_index_type >  liquidity_reward_index;
   */

   typedef liquidity_reward_balance_index liquidity_reward_index;
   typedef convert_request_index convert_index;

} } // steemit::chain

#include <steemit/chain/comment_object.hpp>
#include <steemit/chain/account_object.hpp>
