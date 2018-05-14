#include <golos/api/comment_api_object.hpp>

namespace golos { namespace api {

    using namespace golos::chain;

    comment_api_object::comment_api_object(const golos::chain::comment_object &o, const golos::chain::database &db)
        : id(o.id),
          parent_author(o.parent_author),
          parent_permlink(to_string(o.parent_permlink)),
          author(o.author),
          permlink(to_string(o.permlink)),
          last_update(o.last_update),
          created(o.created),
          active(o.active),
          last_payout(o.last_payout),
          depth(o.depth),
          children(o.children),
          children_rshares2(o.children_rshares2),
          net_rshares(o.net_rshares),
          abs_rshares(o.abs_rshares),
          vote_rshares(o.vote_rshares),
          children_abs_rshares(o.children_abs_rshares),
          cashout_time(o.cashout_time),
          max_cashout_time(o.max_cashout_time),
          total_vote_weight(o.total_vote_weight),
          reward_weight(o.reward_weight),
          total_payout_value(o.total_payout_value),
          curator_payout_value(o.curator_payout_value),
          author_rewards(o.author_rewards),
          net_votes(o.net_votes),
          mode(o.mode),
          root_comment(o.root_comment),
          max_accepted_payout(o.max_accepted_payout),
          percent_steem_dollars(o.percent_steem_dollars),
          allow_replies(o.allow_replies),
          allow_votes(o.allow_votes),
          allow_curation_rewards(o.allow_curation_rewards) {

        for (auto& route : o.beneficiaries) {
            beneficiaries.push_back(route);
        }
#ifndef IS_LOW_MEM
        auto& content = db.get_comment_content(o.id);

        title = to_string(content.title);
        body = to_string(content.body);
        json_metadata = to_string(content.json_metadata);
#endif
    }

    comment_api_object::comment_api_object() = default;

} } // golos::api