
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

   u256 rs2 = to256( calculate_vshares( ctx.rshares.value ) );
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

uint64_t get_rshare_reward( const comment_reward_context& ctx, const reward_fund_name_type& rf_name )
{
   try
   {
   FC_ASSERT( ctx.rshares > 0 );
   FC_ASSERT( ctx.total_reward_shares2 > 0 );

   u256 rs(ctx.rshares.value);
   u256 rf(ctx.total_reward_fund_steem.amount.value);
   u256 total_rshares2 = to256( ctx.total_reward_shares2 );

   //idump( (ctx) );

   u256 rs2 = to256( calculate_vshares( ctx.rshares.value, rf_name ) );
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

uint128_t calculate_vshares( const uint128_t& rshares )
{
   uint128_t s = get_content_constant_s();
   uint128_t rshares_plus_s = rshares + s;
   return rshares_plus_s * rshares_plus_s - s * s;
}

uint128_t calculate_vshares( const uint128_t& rshares, const reward_fund_name_type& rf )
{
   uint128_t s = get_content_constant_s().to_uint64() / STEEMIT_VOTE_DUST_THRESHOLD;
   uint128_t rshares_plus_s = ( rshares / STEEMIT_VOTE_DUST_THRESHOLD ) + s;
   return rshares_plus_s * rshares_plus_s - s * s;
}

} } }
