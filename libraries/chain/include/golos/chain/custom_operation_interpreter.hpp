
#pragma once

#include <memory>

namespace golos {
    namespace schema {
        struct abstract_schema;
    }
}

namespace golos {
    namespace protocol {
        struct custom_json_operation;
    }
}

namespace golos {
    namespace chain {

        class custom_operation_interpreter {
        public:
            virtual void apply(const protocol::custom_json_operation &op) = 0;

            virtual void apply(const protocol::custom_binary_operation &op) = 0;

            virtual std::shared_ptr<golos::schema::abstract_schema> get_operation_schema() = 0;
        };

    }
} // golos::chain
