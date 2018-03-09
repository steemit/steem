#ifndef GOLOS_FOLLOW_FORWARD_HPP
#define GOLOS_FOLLOW_FORWARD_HPP

#include <fc/reflect/reflect.hpp>

namespace golos {
    namespace plugins {
        namespace follow {

        enum follow_type {
            undefined,
            blog,
            ignore
        };

            }}}

FC_REFLECT_ENUM(golos::plugins::follow::follow_type, (undefined)(blog)(ignore))

#endif //GOLOS_FORWARD_HPP
