#pragma once

#include <fc/include/fc/reflect/reflect.hpp>
namespace golos {
    namespace plugins {
        namespace social_network {
            struct tag_index {
                std::vector<std::string> trending; /// pending payouts
            };

        }}}
FC_REFLECT((golos::plugins::social_network::tag_index), (trending))