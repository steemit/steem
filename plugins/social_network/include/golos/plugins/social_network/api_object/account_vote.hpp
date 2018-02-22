#pragma once

#include <fc/reflect/reflect.hpp>
#include <fc/time.hpp>

namespace golos {
    namespace plugins {
        namespace social_network {

            struct account_vote {
                std::string authorperm;
                uint64_t weight = 0;
                int64_t rshares = 0;
                int16_t percent = 0;
                time_point_sec time;
            };

        }}}



FC_REFLECT((golos::plugins::social_network::account_vote), (authorperm)(weight)(rshares)(percent)(time));