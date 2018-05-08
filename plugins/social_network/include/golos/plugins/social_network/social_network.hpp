#pragma once

#include <appbase/application.hpp>
#include <golos/plugins/chain/plugin.hpp>
#include <golos/api/discussion.hpp>
#include <golos/plugins/tags/tag_api_object.hpp>
#include <golos/plugins/follow/plugin.hpp>
#include <golos/api/account_vote.hpp>
#include <golos/api/vote_state.hpp>
#include <golos/api/discussion.hpp>


namespace golos { namespace plugins { namespace social_network {
    using plugins::json_rpc::msg_pack;
    using golos::api::discussion;
    using golos::api::account_vote;
    using golos::api::vote_state;
    using golos::api::comment_api_object;
    using namespace golos::chain;

    DEFINE_API_ARGS(get_content,                           msg_pack, discussion)
    DEFINE_API_ARGS(get_content_replies,                   msg_pack, std::vector<discussion>)
    DEFINE_API_ARGS(get_all_content_replies,               msg_pack, std::vector<discussion>)
    DEFINE_API_ARGS(get_account_votes,                     msg_pack, std::vector<account_vote>)
    DEFINE_API_ARGS(get_active_votes,                      msg_pack, std::vector<vote_state>);

    class social_network final: public appbase::plugin<social_network> {
    public:
        APPBASE_PLUGIN_REQUIRES((chain::plugin)(follow::plugin))

        DECLARE_API(
            (get_content)

            (get_content_replies)

            (get_all_content_replies)

            (get_account_votes)
            /**
             *  if permlink is "" then it will return all votes for author
             */
            (get_active_votes)
        )

        social_network();

        ~social_network();

        void set_program_options(
            boost::program_options::options_description&,
            boost::program_options::options_description& config_file_options
        ) override;

        static const std::string& name();

        void plugin_initialize(const boost::program_options::variables_map& options) override;

        void plugin_startup() override;

        void plugin_shutdown() override;


    private:
        struct impl;
        std::unique_ptr<impl> pimpl;
    };
} } } // golos::plugins::social_network