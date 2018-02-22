#ifndef GOLOS_FOLLOW_EVALUATORS_HPP
#define GOLOS_FOLLOW_EVALUATORS_HPP

#include <golos/plugins/follow/follow_operations.hpp>
#include <golos/chain/database.hpp>
#include <golos/chain/evaluator.hpp>

namespace golos {
    namespace plugins {
        namespace follow {
            using golos::chain::evaluator;
            using golos::chain::database;

            class follow_evaluator : public golos::chain::evaluator_impl<follow_evaluator, follow_plugin_operation> {
            public:
                typedef follow_operation operation_type;

                follow_evaluator(database &db, plugin *plugin) : golos::chain::evaluator_impl<follow_evaluator, follow_plugin_operation>(db), _plugin(plugin) {
                }

                void do_apply(const follow_operation &o);

                plugin *_plugin;
            };

            class reblog_evaluator : public golos::chain::evaluator_impl<reblog_evaluator, follow_plugin_operation> {
            public:
                typedef reblog_operation operation_type;

                reblog_evaluator(database &db, plugin *plugin) : golos::chain::evaluator_impl<reblog_evaluator, follow_plugin_operation>(db), _plugin(plugin) {
                }

                void do_apply(const reblog_operation &o);

                plugin *_plugin;
            };
        }
    }
}

#endif //GOLOS_FOLLOW_EVALUATORS_HPP
