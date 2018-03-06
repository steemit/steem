#pragma once

#include <fc/reflect/reflect.hpp>

namespace golos {
    namespace chain {
        enum class reward_curve: uint8_t {
            quadratic,
            linear
        };
    }
} // golos::chain

FC_REFLECT_ENUM(golos::chain::reward_curve, (quadratic)(linear))