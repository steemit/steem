#pragma once

#include <steem/protocol/steem_actions.hpp>

#include <steem/protocol/operation_util.hpp>

namespace steem { namespace protocol {

   /** NOTE: do not change the order of any actions or it will trigger a hardfork.
    */
   typedef fc::static_variant<
            required_action,
            optional_action
         > automated_action;

} } // steem::protocol

STEEM_DECLARE_OPERATION_TYPE( steem::protocol::automated_action );
FC_REFLECT_TYPENAME( steem::protocol::automated_action );
