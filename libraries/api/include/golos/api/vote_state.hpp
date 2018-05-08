#pragma once
#include <fc/reflect/reflect.hpp>

namespace golos { namespace api {
    using golos::chain::share_type;
    struct vote_state {
        string voter;
        uint64_t weight = 0;
        int64_t rshares = 0;
        int16_t percent = 0;
        share_type reputation = 0;
        time_point_sec time;
    };

} } // golos::api


FC_REFLECT((golos::api::vote_state), (voter)(weight)(rshares)(percent)(reputation)(time));