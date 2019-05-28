#include <steem/chain/steem_objects.hpp>
#include <steem/plugins/chain/chain_plugin.hpp>
#include <steem/plugins/rewards_api/rewards_api_plugin.hpp>
#include <steem/plugins/rewards_api/rewards_api.hpp>

namespace steem { namespace plugins { namespace rewards_api {

namespace detail {

class rewards_api_impl
{
public:
   rewards_api_impl() :
      _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ) {}

   DECLARE_API_IMPL( (simulate_curve_payout) )

   chain::database& _db;
};

DEFINE_API_IMPL( rewards_api_impl, simulate_curve_payout )
{
   simulate_curve_payout_return ret;

   return ret;
}

} // steem::plugins::rewards_api::detail

rewards_api::rewards_api() : my( std::make_unique< detail::rewards_api_impl >() )
{
   JSON_RPC_REGISTER_API( STEEM_REWARDS_API_PLUGIN_NAME );
}

rewards_api::~rewards_api() {}

DEFINE_READ_APIS( rewards_api, (simulate_curve_payout) )

} } } // steem::plugins::rewards_api

