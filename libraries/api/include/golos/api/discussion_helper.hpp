#pragma once
#include <golos/api/account_vote.hpp>
#include <golos/api/vote_state.hpp>
#include <golos/api/discussion.hpp>

namespace golos { namespace api {
    struct comment_metadata {
        std::set<std::string> tags;
        std::string language;
    };

    comment_metadata get_metadata(const comment_api_object &c);

    class discussion_helper {
    public:
        discussion_helper() = delete;
        discussion_helper(
            golos::chain::database& db,
            std::function<void(const golos::chain::database&, const account_name_type&, fc::optional<share_type>&)> fill_reputation,
            std::function<void(const golos::chain::database&, discussion&)> fill_promoted);
        ~discussion_helper() ;


        void set_pending_payout(discussion& d) const;
        
        void set_url(discussion& d) const;

        void select_active_votes(
            std::vector<vote_state>& result, uint32_t& total_count,
            const std::string& author, const std::string& permlink, uint32_t limit
        ) const ;

        discussion create_discussion(const comment_object& o) const;

        discussion get_discussion(const comment_object& c, uint32_t vote_limit) const;

    private:
        struct impl;
        std::unique_ptr<impl> pimpl;
    };

} } // golos::api

FC_REFLECT((golos::api::comment_metadata), (tags)(language))
