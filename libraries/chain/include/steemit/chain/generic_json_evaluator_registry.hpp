
#pragma once

namespace steemit {
    namespace chain {

        struct custom_json_operation;

        class generic_json_evaluator_registry {
        public:
            virtual void apply(const custom_json_operation &op) = 0;

            virtual void apply(const custom_binary_operation &op) = 0;
        };

    }
} // steemit::chain
