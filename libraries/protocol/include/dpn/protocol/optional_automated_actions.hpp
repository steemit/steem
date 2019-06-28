#pragma once

#include <dpn/protocol/dpn_optional_actions.hpp>

#include <dpn/protocol/operation_util.hpp>

namespace dpn { namespace protocol {

   /** NOTE: do not change the order of any actions or it will trigger a hardfork.
    */
   typedef fc::static_variant<
#ifdef IS_TEST_NET
            example_optional_action
#endif
         > optional_automated_action;

} } // dpn::protocol

DPN_DECLARE_OPERATION_TYPE( dpn::protocol::optional_automated_action );

FC_TODO( "Remove ifdef when first optional automated action is added" )
#ifdef IS_TEST_NET
FC_REFLECT_TYPENAME( dpn::protocol::optional_automated_action );
#endif