#include <steem/plugins/sps_api/sps_api_plugin.hpp>
#include <steem/plugins/sps_api/sps_api.hpp>
#include <steem/chain/sps_objects.hpp>
#include <appbase/application.hpp>

namespace steem { namespace plugins { namespace sps {

namespace detail {

class sps_api_impl
{
  public:
    sps_api_impl();
    ~sps_api_impl();

    DECLARE_API_IMPL(
        (find_proposal)
        (list_proposals)
        (list_voter_proposals)
        )
  private:
    chain::database& _db;
};

sps_api_impl::sps_api_impl() : _db(appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db()) {}

sps_api_impl::~sps_api_impl() {}

DEFINE_API_IMPL(sps_api_impl, find_proposal) {
  ilog("find_proposal called");
  find_proposal_return result;

  return result;
}

DEFINE_API_IMPL(sps_api_impl, list_proposals) {
  ilog("list_proposals called");
  list_proposals_return result;

  return result;
}

DEFINE_API_IMPL(sps_api_impl, list_voter_proposals) {
  ilog("list_voter_proposals called");
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
  (find_proposal)
  (list_proposals)
  (list_voter_proposals)
)

} } } //steem::plugins::sps_api