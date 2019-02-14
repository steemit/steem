#include <steem/plugins/sps_api/sps_api_plugin.hpp>
#include <steem/plugins/sps_api/sps_api.hpp>

namespace steem { namespace plugins { namespace sps {

namespace detail {

class sps_api_impl
{
  public:
    DECLARE_API_IMPL(
        (create_proposal)
        (update_proposal_votes)
        (list_proposals)
        (list_voter_proposals)
        )
};

DEFINE_API_IMPL(sps_api_impl, create_proposal) {
  ilog("create_proposal called");
  create_proposal_return result;

  return result;
}

DEFINE_API_IMPL(sps_api_impl, update_proposal_votes) {
  ilog("update_proposal_votes called");
  update_proposal_votes_return result;

  return result;
}

DEFINE_API_IMPL(sps_api_impl, list_proposals) {
  ilog("get_proposals called");
  list_proposals_return result;

  return result;
}

DEFINE_API_IMPL(sps_api_impl, list_voter_proposals) {
  ilog("get_voter_proposals called");
  list_voter_proposals_return result;

  return result;
}

} // detail

sps_api::sps_api(): my( new detail::sps_api_impl() )
{
   JSON_RPC_REGISTER_API( STEEM_SPS_API_PLUGIN_NAME );
}

sps_api::~sps_api() {}

DEFINE_LOCKLESS_APIS(sps_api,
  (list_proposals)
  (list_voter_proposals)
  (create_proposal)
  (update_proposal_votes)
)

} } } //steem::plugins::sps_api