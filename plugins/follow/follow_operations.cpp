#include <golos/plugins/follow/follow_operations.hpp>
#include <golos/protocol/operation_util_impl.hpp>

namespace golos {
    namespace plugins {
        namespace follow {

            void follow_operation::validate() const {
                FC_ASSERT(follower != following, "You cannot follow yourself");
            }

            void reblog_operation::validate() const {
                FC_ASSERT(account != author, "You cannot reblog your own content");
            }

        }
    }
} //golos::follow

DEFINE_OPERATION_TYPE(golos::plugins::follow::follow_plugin_operation)