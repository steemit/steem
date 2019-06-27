#pragma once

#include <dpn/protocol/dpn_required_actions.hpp>

#include <dpn/protocol/operation_util.hpp>

namespace dpn { namespace protocol {

   /** NOTE: do not change the order of any actions or it will trigger a hardfork.
    */
   typedef fc::static_variant<
#ifdef IS_TEST_NET
            example_required_action
#endif
         > required_automated_action;

} } // dpn::protocol

DPN_DECLARE_OPERATION_TYPE( dpn::protocol::required_automated_action );

FC_TODO( "Remove ifdef when first required automated action is added" )
#ifdef IS_TEST_NET
FC_REFLECT_TYPENAME( dpn::protocol::required_automated_action );
#endif
