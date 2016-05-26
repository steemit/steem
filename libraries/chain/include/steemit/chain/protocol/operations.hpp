#pragma once
#include <steemit/chain/protocol/base.hpp>
#include <steemit/chain/protocol/steem_operations.hpp>

namespace steemit { namespace chain {

   /** NOTE: do not change the order of any operations prior to the virtual operations
    * or it will trigger a hardfork.
    */
   typedef fc::static_variant<
            vote_operation,
            comment_operation,

            transfer_operation,
            transfer_to_vesting_operation,
            withdraw_vesting_operation,

            limit_order_create_operation,
            limit_order_cancel_operation,

            feed_publish_operation,
            convert_operation,

            account_create_operation,
            account_update_operation,

            witness_update_operation,
            account_witness_vote_operation,
            account_witness_proxy_operation,

            pow_operation,

            custom_operation,

            report_over_production_operation,

            delete_comment_operation,
            custom_json_operation,

            /// virtual operations below this point
            fill_convert_request_operation,
            comment_reward_operation,
            curate_reward_operation,
            liquidity_reward_operation,
            interest_operation,
            fill_vesting_withdraw_operation,
            fill_order_operation,
            comment_payout_operation

         > operation;

   void operation_get_required_authorities( const operation& op,
                                            flat_set<string>& active,
                                            flat_set<string>& owner,
                                            flat_set<string>& posting,
                                            vector<authority>&  other );

   void operation_validate( const operation& op );

   struct op_wrapper
   {
      public:
         op_wrapper(const operation& op = operation()):op(op){}
         operation op;
   };

   bool is_market_operation( const operation& op );

} } // steemit::chain

namespace fc {
    void to_variant( const steemit::chain::operation& var,  fc::variant& vo );
    void from_variant( const fc::variant& var,  steemit::chain::operation& vo );
}

FC_REFLECT_TYPENAME( steemit::chain::operation )
FC_REFLECT( steemit::chain::op_wrapper, (op) )
