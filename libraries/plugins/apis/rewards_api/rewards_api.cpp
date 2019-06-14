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

   fc::uint128_t sum_current_curve_vshares = 0;
   fc::uint128_t sum_simulated_vshares = 0;

   std::vector< fc::uint128_t > element_vshares;

   auto reward_fund_object = _db.get< chain::reward_fund_object, chain::by_name >( STEEM_POST_REWARD_FUND_NAME );

   fc::uint128_t var1{ args.var1 };

   while( current != cidx.end() && current->cashout_time < fc::time_point_sec::maximum() )
   {
      if ( current->net_rshares.value <= 0)
      {
         ++current;
         continue;
      }

      simulate_curve_payouts_element e;
      e.author = current->author;
      e.permlink = current->permlink;

      auto new_curve_vshares = chain::util::evaluate_reward_curve( current->net_rshares.value, args.curve, var1 );
      sum_simulated_vshares = sum_simulated_vshares + new_curve_vshares;

      auto current_curve_vshares = chain::util::evaluate_reward_curve( current->net_rshares.value, reward_fund_object.author_reward_curve, reward_fund_object.content_constant );
      sum_current_curve_vshares = sum_current_curve_vshares + current_curve_vshares;


      ret.payouts.push_back( std::move( e ) );
      element_vshares.push_back( new_curve_vshares );

      ++current;
   }

   auto current_estimated_recent_claims = reward_fund_object.recent_claims + sum_current_curve_vshares;

   u256 simulated_recent_claims_u256;
   fc::uint128_t simulated_recent_claims;

   simulated_recent_claims_u256 = ( chain::util::to256( current_estimated_recent_claims ) * chain::util::to256( sum_simulated_vshares ) ) / chain::util::to256( sum_current_curve_vshares );
   FC_ASSERT( ( simulated_recent_claims_u256 >> 64 ) <= u256( uint64_t( std::numeric_limits<int64_t>::max() ) ) );
   simulated_recent_claims = fc::uint128_t( static_cast< int64_t >( simulated_recent_claims_u256 >> 64 ), static_cast< uint64_t >( simulated_recent_claims_u256 ) );


   auto rf = chain::util::to256( reward_fund_object.reward_balance.amount.value );
   auto total_claims = chain::util::to256( simulated_recent_claims );

   for ( std::size_t i = 0; i < ret.payouts.size(); ++i )
   {
      auto payout_u256 = ( rf * chain::util::to256( element_vshares[ i ] ) ) / total_claims;
      FC_ASSERT( payout_u256 <= u256( uint64_t( std::numeric_limits<int64_t>::max() ) ) );
      ret.payouts[ i ].payout = protocol::asset( static_cast< uint64_t >( payout_u256 ), STEEM_SYMBOL );
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

