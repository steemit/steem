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

                void get_required_posting_authorities(flat_set<account_name_type> &a) const {
                    a.insert(follower);
                }
            };

            struct reblog_operation : base_operation{
                protocol::account_name_type account;
                protocol::account_name_type author;
                std::string permlink;

                void validate() const;

                void get_required_posting_authorities(flat_set<account_name_type> &a) const {
                    a.insert(account);
                }
            };

            typedef fc::static_variant<follow_operation, reblog_operation> follow_plugin_operation;


        }
    }
} // golos::follow

FC_REFLECT((golos::plugins::follow::follow_operation), (follower)(following)(what));
FC_REFLECT((golos::plugins::follow::reblog_operation), (account)(author)(permlink));

namespace fc {

    void to_variant(const golos::plugins::follow::follow_plugin_operation &, fc::variant &);

    void from_variant(const fc::variant &, golos::plugins::follow::follow_plugin_operation &);

} /* fc */

namespace golos {
    namespace protocol {

        void operation_validate(const plugins::follow::follow_plugin_operation &o);

        void operation_get_required_authorities(const plugins::follow::follow_plugin_operation &op,
                                                flat_set<account_name_type> &active, flat_set<account_name_type> &owner,
                                                flat_set<account_name_type> &posting, std::vector<authority> &other);

    }
}

FC_REFLECT_TYPENAME((golos::plugins::follow::follow_plugin_operation));

