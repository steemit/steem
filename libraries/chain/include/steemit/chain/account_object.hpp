#pragma once

#include <steemit/chain/protocol/authority.hpp>
#include <steemit/chain/protocol/types.hpp>
#include <steemit/chain/protocol/steem_operations.hpp>
#include <steemit/chain/witness_objects.hpp>

#include <graphene/db/generic_index.hpp>

#include <boost/multi_index/composite_key.hpp>


namespace steemit { namespace chain {

   class account_object : public abstract_object<account_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_account_object_type;

         string          name;
         authority       owner; 
         authority       active; 
         authority       posting; 
         public_key_type memo_key;
         string          json_metadata;
         string          proxy;

         time_point_sec  created;
         uint32_t        comment_count = 0;
         uint32_t        lifetime_vote_count = 0;

         uint16_t        voting_power = STEEMIT_100_PERCENT;   
         time_point_sec  last_vote_time; 

         asset           balance = asset( 0, STEEM_SYMBOL );  

         ///@{
         asset              sbd_balance = asset( 0, SBD_SYMBOL ); 
         fc::uint128_t      sbd_seconds; 
         fc::time_point_sec sbd_seconds_last_update; 
         fc::time_point_sec sbd_last_interest_payment; 
         ///@}


         asset           vesting_shares = asset( 0, VESTS_SYMBOL ); 
         asset           vesting_withdraw_rate = asset( 0, VESTS_SYMBOL ); 
         time_point_sec  next_vesting_withdrawal = fc::time_point_sec::maximum(); 
         share_type      withdrawn = 0; 
         share_type      to_withdraw = 0;

         share_type      proxied_vsf_votes; 

         uint64_t        average_bandwidth  = 0;
         uint64_t        lifetime_bandwidth = 0;
         time_point_sec  last_bandwidth_update;

         uint64_t        average_market_bandwidth  = 0;
         time_point_sec  last_market_bandwidth_update;


         account_id_type get_id()const { return id; }
         share_type      witness_vote_weight()const { return vesting_shares.amount + proxied_vsf_votes; }
   };

   class account_member_index : public secondary_index
   {
      public:
         virtual void object_inserted( const object& obj ) override;
         virtual void object_removed( const object& obj ) override;
         virtual void about_to_modify( const object& before ) override;
         virtual void object_modified( const object& after  ) override;

         map< string, set<string> >          account_to_account_memberships;
         map< public_key_type, set<string> > account_to_key_memberships;

      protected:
         set<string>             get_account_members( const account_object& a )const;
         set<public_key_type>    get_key_members( const account_object& a )const;

         set<string>             before_account_members;
         set<public_key_type>    before_key_members;
   };

   struct by_name;
   struct by_proxy;
   struct by_next_vesting_withdrawal;
   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      account_object,
      indexed_by<
         ordered_unique< tag< by_id >,
            member< object, object_id_type, &object::id > >,
         ordered_unique< tag< by_name >,
            member< account_object, string, &account_object::name > >,
         ordered_unique< tag< by_proxy >,
            composite_key< account_object,
               member< account_object, string, &account_object::proxy >,
               member<object, object_id_type, &object::id >
            > /// composite key by proxy
         >,
         ordered_unique< tag< by_next_vesting_withdrawal >,
            composite_key< account_object,
               member<account_object, time_point_sec, &account_object::next_vesting_withdrawal >,
               member<object, object_id_type, &object::id >
            > /// composite key by_next_vesting_withdrawal
         >
      >
   > account_multi_index_type;

   typedef generic_index< account_object,                   account_multi_index_type >             account_index;

} }

FC_REFLECT_DERIVED( steemit::chain::account_object, (graphene::db::object),
                    (name)(owner)(active)(posting)(memo_key)(json_metadata)(proxy)
                    (created)(comment_count)(lifetime_vote_count)(voting_power)(last_vote_time)
                    (balance)
                    (sbd_balance)(sbd_seconds)(sbd_seconds_last_update)(sbd_last_interest_payment)
                    (vesting_shares)(vesting_withdraw_rate)(next_vesting_withdrawal)(withdrawn)(to_withdraw)
                    (proxied_vsf_votes)
                    (average_bandwidth)(lifetime_bandwidth)(last_bandwidth_update)
                    (average_market_bandwidth)(last_market_bandwidth_update)
                  )
