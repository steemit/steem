#pragma once

#include <golos/plugins/social_network/api_object/vote_state.hpp>
#include <golos/plugins/social_network/api_object/comment_api_object.hpp>


namespace golos { namespace plugins { namespace social_network {

    struct discussion: public comment_api_object {
        discussion(const comment_object& o): comment_api_object(o) {
        }

        discussion() {
        }

        string url; /// /category/@rootauthor/root_permlink#author/permlink
        string root_title;
        asset pending_payout_value = asset(0, SBD_SYMBOL); ///< sbd
        asset total_pending_payout_value = asset(0, SBD_SYMBOL); ///< sbd including replies
        std::vector<vote_state> active_votes;
        uint32_t active_votes_count = 0;
        std::vector<string> replies; ///< author/slug mapping
        share_type author_reputation = 0;
        asset promoted = asset(0, SBD_SYMBOL);
        uint32_t body_length = 0;
        std::vector<account_name_type> reblogged_by;
        optional <account_name_type> first_reblogged_by;
        optional <time_point_sec> first_reblogged_on;
    };

} } } // golos::plugins::social_network

FC_REFLECT_DERIVED((
    golos::plugins::social_network::discussion), ((golos::plugins::social_network::comment_api_object)),
        (url)(root_title)(pending_payout_value)(total_pending_payout_value)(active_votes)(active_votes_count)(replies)
        (author_reputation)(promoted)(body_length)(reblogged_by)(first_reblogged_by)(first_reblogged_on))
