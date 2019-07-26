#pragma once

#include <steem/protocol/steem_required_actions.hpp>

#include <steem/protocol/operation_util.hpp>

namespace steem { namespace protocol {

   /** NOTE: do not change the order of any actions or it will trigger a hardfork.
    */
   typedef fc::static_variant<
#ifdef IS_TEST_NET
            example_required_action
   #ifdef STEEM_ENABLE_SMT
            ,
   #endif
#endif
#ifdef STEEM_ENABLE_SMT
            smt_event_action,
            smt_refund_action,
            smt_contributor_payout_action
#endif
         > required_automated_action;

} } // steem::protocol

STEEM_DECLARE_OPERATION_TYPE( steem::protocol::required_automated_action );

//FC_TODO( "Remove ifdef when first required automated action is added" )
//#ifdef IS_TEST_NET
FC_REFLECT_TYPENAME( steem::protocol::required_automated_action );
//#endif
