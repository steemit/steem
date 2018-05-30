#pragma once

#include <golos/protocol/base.hpp>
#include <golos/chain/evaluator.hpp>
#include <golos/plugins/follow/plugin.hpp>

namespace golos {
    namespace plugins {
        namespace follow {
            using golos::chain::base_operation;
            using golos::protocol::account_name_type;
            struct follow_operation : base_operation {
                protocol::account_name_type follower;
                protocol::account_name_type following;
                std::set<std::string> what; /// blog, mute

                void validate() const;
                void get_required_posting_authorities(flat_set<account_name_type>& a) const {
                    a.insert(follower);
                }
            };

            struct reblog_operation : base_operation{
                protocol::account_name_type account;
                protocol::account_name_type author;
                std::string permlink;

                void validate() const;
                void get_required_posting_authorities(flat_set<account_name_type>& a) const {
                    a.insert(account);
                }
            };

            using follow_plugin_operation = fc::static_variant<follow_operation, reblog_operation>;

        }
    }
} // golos::follow

FC_REFLECT((golos::plugins::follow::follow_operation), (follower)(following)(what));
FC_REFLECT((golos::plugins::follow::reblog_operation), (account)(author)(permlink));

FC_REFLECT_TYPENAME((golos::plugins::follow::follow_plugin_operation));
DECLARE_OPERATION_TYPE(golos::plugins::follow::follow_plugin_operation)
