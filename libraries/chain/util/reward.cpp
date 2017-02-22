
#include <steemit/chain/util/reward.hpp>
#include <steemit/chain/util/uint256.hpp>

namespace steemit { namespace chain { namespace util {

uint64_t get_rshare_reward( const comment_reward_context& ctx )
{
   try
   {
   FC_ASSERT( ctx.rshares > 0 );
   FC_ASSERT( ctx.total_reward_shares2 > 0 );

   u256 rs(ctx.rshares.value);
   u256 rf(ctx.total_reward_fund_steem.amount.value);
   u256 total_rshares2 = to256( ctx.total_reward_shares2 );

   //idump( (ctx) );

   u256 rs2 = to256( calculate_claims( ctx.rshares.value ) );
   rs2 = ( rs2 * ctx.reward_weight ) / STEEMIT_100_PERCENT;

   u256 payout_u256 = ( rf * rs2 ) / total_rshares2;
   FC_ASSERT( payout_u256 <= u256( uint64_t( std::numeric_limits<int64_t>::max() ) ) );
   uint64_t payout = static_cast< uint64_t >( payout_u256 );

   if( is_comment_payout_dust( ctx.current_steem_price, payout ) )
      payout = 0;

   asset max_steem = to_steem( ctx.current_steem_price, ctx.max_sbd );

   payout = std::min( payout, uint64_t( max_steem.amount.value ) );

   return payout;
   } FC_CAPTURE_AND_RETHROW( (ctx) )
}

uint64_t get_rshare_reward( const comment_reward_context& ctx, const reward_fund_object& rf_object )
{
   try
   {
   FC_ASSERT( ctx.rshares > 0 );
   FC_ASSERT( ctx.total_reward_shares2 > 0 );

   u256 rf(ctx.total_reward_fund_steem.amount.value);
   u256 total_claims = to256( ctx.total_reward_shares2 );

   //idump( (ctx) );

   u256 claim = to256( calculate_claims( ctx.rshares.value, rf_object ) );
   claim = ( claim * ctx.reward_weight ) / STEEMIT_100_PERCENT;

   u256 payout_u256 = ( rf * claim ) / total_claims;
   FC_ASSERT( payout_u256 <= u256( uint64_t( std::numeric_limits<int64_t>::max() ) ) );
   uint64_t payout = static_cast< uint64_t >( payout_u256 );

   if( is_comment_payout_dust( ctx.current_steem_price, payout ) )
      payout = 0;

   asset max_steem = to_steem( ctx.current_steem_price, ctx.max_sbd );

   payout = std::min( payout, uint64_t( max_steem.amount.value ) );

   return payout;
   } FC_CAPTURE_AND_RETHROW( (ctx) )
}

uint128_t calculate_claims( const uint128_t& rshares )
{
   uint128_t s = get_content_constant_s();
   uint128_t rshares_plus_s = rshares + s;
   return rshares_plus_s * rshares_plus_s - s * s;
}

uint128_t calculate_claims( const uint128_t& rshares, const reward_fund_object& rf )
{
   if( rf.name == STEEMIT_POST_REWARD_FUND_NAME )
   {
      // r^2 + 2rs
      uint128_t rshares_plus_s = rshares + rf.content_constant;
      return rshares_plus_s * rshares_plus_s + rf.content_constant * rf.content_constant;
   }
   else if( rf.name == STEEMIT_COMMENT_REWARD_FUND_NAME )
   {
      // r^2 / ( s + r )
      // Can be optimized to r - r * s / ( r + s ) Needs investigation to determine what the loss of precision is (if any)
      // This optimization is worth noting because it could be implemented using only 64-bit math
      uint128_t rshares_2 = ( rshares * rshares );
      uint128_t rshares_plus_s = rshares + rf.content_constant;
      return rshares_2 / rshares_plus_s;
   }

   wlog( "Unknown reward fund ${rf}", ("rf",rf) );

   return 0;
}

} } }
