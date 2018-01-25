#pragma once
#include <golos/plugins/private_message/private_message_objects.hpp>


#include <appbase/plugin.hpp>
#include <golos/chain/database.hpp>

#include <golos/plugins/json_rpc/utility.hpp>
#include <golos/plugins/json_rpc/plugin.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <fc/thread/future.hpp>
#include <fc/api.hpp>

namespace golos {
    namespace plugins {
        namespace private_message {
            using namespace golos::chain;

            DEFINE_API_ARGS(get_inbox,  json_rpc::msg_pack, inbox_r)
            DEFINE_API_ARGS(get_outbox, json_rpc::msg_pack, outbox_r)

            /**
             *   This plugin scans the blockchain for custom operations containing a valid message and authorized
             *   by the posting key.
             *
             */
            class private_message_plugin final : public appbase::plugin<private_message_plugin> {
            public:

                APPBASE_PLUGIN_REQUIRES((json_rpc::plugin))

                private_message_plugin();

                ~private_message_plugin();

                void set_program_options(
                        boost::program_options::options_description &cli,
                        boost::program_options::options_description &cfg) override;

                void plugin_initialize(const boost::program_options::variables_map &options) override;

                void plugin_startup() override;

                void plugin_shutdown() override;

                flat_map <std::string, std::string> tracked_accounts() const; /// map start_range to end_range

                friend class private_message_plugin_impl;

                constexpr const static char *plugin_name = "private_message";

                static const std::string &name() {
                    static std::string name = plugin_name;
                    return name;
                }

                DECLARE_API((get_inbox)(get_outbox))


            private:
                class private_message_plugin_impl;

                std::unique_ptr<private_message_plugin_impl> my;
            };
        }
    }
} //golos::plugins::private_message

FC_REFLECT((golos::plugins::private_message::message_body), (thread_start)(subject)(body)(json_meta)(cc));

FC_REFLECT((golos::plugins::private_message::message_object), (id)(from)(to)(from_memo_key)(to_memo_key)(sent_time)(receive_time)(checksum)(encrypted_message));
CHAINBASE_SET_INDEX_TYPE(golos::plugins::private_message::message_object, golos::plugins::private_message::message_index);

FC_REFLECT((golos::plugins::private_message::message_api_obj), (id)(from)(to)(from_memo_key)(to_memo_key)(sent_time)(receive_time)(checksum)(encrypted_message));

FC_REFLECT_DERIVED((golos::plugins::private_message::extended_message_object), ((golos::plugins::private_message::message_api_obj)), (message));
