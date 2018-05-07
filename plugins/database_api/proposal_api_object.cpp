#include <golos/plugins/database_api/api_objects/proposal_api_object.hpp>

namespace golos { namespace plugins { namespace database_api {

    proposal_api_object::proposal_api_object(const golos::chain::proposal_object& p)
        : author(p.author),
          title(golos::chain::to_string(p.title)),
          memo(golos::chain::to_string(p.memo)),
          expiration_time(p.expiration_time),
          review_period_time(p.review_period_time),
          proposed_operations(p.operations()),
          required_active_approvals(p.required_active_approvals.begin(), p.required_active_approvals.end()),
          available_active_approvals(p.available_active_approvals.begin(), p.available_active_approvals.end()),
          required_owner_approvals(p.required_owner_approvals.begin(), p.required_owner_approvals.end()),
          available_owner_approvals(p.available_owner_approvals.begin(), p.available_owner_approvals.end()),
          required_posting_approvals(p.required_posting_approvals.begin(), p.required_posting_approvals.end()),
          available_posting_approvals(p.available_posting_approvals.begin(), p.available_posting_approvals.end()),
          available_key_approvals(p.available_key_approvals.begin(), p.available_key_approvals.end()) {
    }

}}} // golos::plugins::database_api