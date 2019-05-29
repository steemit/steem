#include <chainbase/chainbase.hpp>
#include <steem/chain/steem_objects.hpp>
#include <steem/chain/util/reward.hpp>
#include <steem/chain/util/uint256.hpp>
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

   DECLARE_API_IMPL( (simulate_curve_payouts) );

   chain::database& _db;
};

DEFINE_API_IMPL( rewards_api_impl, simulate_curve_payouts )
{
   simulate_curve_payouts_return ret;

   const auto& cidx = _db.get_index< chain::comment_index, chain::by_cashout_time >();

   auto current = cidx.begin();

   while( current != cidx.end() && current->cashout_time > _db.head_block_time() && current->cashout_time < fc::time_point_sec::maximum() )
   {
      simulate_curve_payouts_element e;
      e.author = current->author;
      e.permlink = current->permlink;
      auto vshares = chain::util::evaluate_reward_curve( current->net_rshares.value, args.curve, args.var1 );
      e.payout = protocol::asset( vshares.to_uint64(), STEEM_SYMBOL );
      ret.payouts.push_back( std::move( e ) );
   }

   return ret;
}

} // steem::plugins::rewards_api::detail

rewards_api::rewards_api() : my( std::make_unique< detail::rewards_api_impl >() )
{
   JSON_RPC_REGISTER_API( STEEM_REWARDS_API_PLUGIN_NAME );
}

rewards_api::~rewards_api() {}

DEFINE_READ_APIS( rewards_api, (simulate_curve_payouts) )

} } } // steem::plugins::rewards_api

