
#include <voting_helper.hpp>

#include <steem/chain/comment_object.hpp>
#include <steem/chain/database.hpp>

#include <steem/chain/util/reward.hpp>

//#include <fc/uint128.hpp>

namespace steem { namespace chain {

const share_type& TSteemVotingHelper::GetCommentNetRshares( const comment_object& comment ) { return comment.net_rshares; }
const share_type& TSteemVotingHelper::GetCommentVoteRshares( const comment_object& comment ) { return comment.vote_rshares; }

void TSteemVotingHelper::IncreaseCommentTotalVoteWeight( const comment_object& comment, const uint64_t& delta )
   {
      DB.modify( comment, [&]( comment_object& c )
      {
         c.total_vote_weight += delta;
      });
   }

const comment_vote_object& TSteemVotingHelper::CreateCommentVoteObject( const account_id_type& voterId, const comment_id_type& commentId,
                                                            const int16_t& opWeight )
{
   if( CVO == nullptr )
      CVO = &DB.create<comment_vote_object>( [&]( comment_vote_object& cv ){
         cv.voter   = voterId;
         cv.comment = commentId;
         cv.vote_percent = opWeight;
         cv.last_update = DB.head_block_time();
      });

   return *CVO;
}

fc::uint128_t TSteemVotingHelper::CalculateAvgCashoutSec( const comment_object& comment, const comment_object& root,
                                                const int64_t& voter_abs_rshares, bool is_recast )
{
   // Note that SMT implementation returns 0 from here, thus children_abs_rshares stays a STEEM only field.
   if( DB.has_hardfork( STEEM_HARDFORK_0_17__769 ) )
      return fc::uint128_t();

   auto old_root_abs_rshares = root.children_abs_rshares.value;

   fc::uint128_t cur_cashout_time_sec = DB.calculate_discussion_payout_time( comment ).sec_since_epoch();
   fc::uint128_t new_cashout_time_sec;

   if( DB.has_hardfork( STEEM_HARDFORK_0_12__177 ) && !DB.has_hardfork( STEEM_HARDFORK_0_13__257)  )
      new_cashout_time_sec = DB.head_block_time().sec_since_epoch() + STEEM_CASHOUT_WINDOW_SECONDS_PRE_HF17;
   else
      new_cashout_time_sec = DB.head_block_time().sec_since_epoch() + STEEM_CASHOUT_WINDOW_SECONDS_PRE_HF12;

   if( is_recast )
   {
      if( DB.has_hardfork( STEEM_HARDFORK_0_14__259 ) && voter_abs_rshares == 0 )
         return cur_cashout_time_sec;
      else
         return ( cur_cashout_time_sec * old_root_abs_rshares + new_cashout_time_sec * voter_abs_rshares ) /
                  ( old_root_abs_rshares + voter_abs_rshares );
   }
   else
      return ( cur_cashout_time_sec * old_root_abs_rshares + new_cashout_time_sec * voter_abs_rshares ) /
               ( old_root_abs_rshares + voter_abs_rshares );
}

uint64_t TSteemVotingHelper::CalculateCommentVoteWeight(const comment_object& comment, const int64_t& rshares, 
                                             const share_type& old_vote_rshares )
{
   share_type comment_vote_rshares = GetCommentVoteRshares( comment );
   if( comment.created < fc::time_point_sec(STEEM_HARDFORK_0_6_REVERSE_AUCTION_TIME) ) {
      u512 rshares3(rshares);
      u256 total2( comment.abs_rshares.value );

      if( !DB.has_hardfork( STEEM_HARDFORK_0_1 ) )
      {
         rshares3 *= 1000000;
         total2 *= 1000000;
      }

      rshares3 = rshares3 * rshares3 * rshares3;

      total2 *= total2;
      return static_cast<uint64_t>( rshares3 / total2 );
   }
   
   // cv.weight = W(R_1) - W(R_0)
   const uint128_t two_s = 2 * util::get_content_constant_s();
   if( DB.has_hardfork( STEEM_HARDFORK_0_17__774 ) )
   {
#pragma message( "TODO: Make this block common with SMT implementation of this method." )
#pragma message( "TODO: Use abstract methods to aquire reward curve and content constant here (stored in SMT object)." )
      const auto& reward_fund = DB.get_reward_fund( comment );
      auto curve = !DB.has_hardfork( STEEM_HARDFORK_0_19__1052 ) && comment.created > STEEM_HF_19_SQRT_PRE_CALC
                     ? curve_id::square_root : reward_fund.curation_reward_curve;
      uint64_t old_weight = util::evaluate_reward_curve( old_vote_rshares.value, curve, reward_fund.content_constant ).to_uint64();
      uint64_t new_weight = util::evaluate_reward_curve( comment_vote_rshares.value, curve, reward_fund.content_constant ).to_uint64();
      return new_weight - old_weight;
   }
   
   if ( DB.has_hardfork( STEEM_HARDFORK_0_1 ) )
   {
      uint64_t old_weight = ( ( std::numeric_limits< uint64_t >::max() * fc::uint128_t( old_vote_rshares.value ) ) / ( two_s + old_vote_rshares.value ) ).to_uint64();
      uint64_t new_weight = ( ( std::numeric_limits< uint64_t >::max() * fc::uint128_t( comment_vote_rshares.value ) ) / ( two_s + comment_vote_rshares.value ) ).to_uint64();
      return new_weight - old_weight;
   }

   uint64_t old_weight = ( ( std::numeric_limits< uint64_t >::max() * fc::uint128_t( 1000000 * old_vote_rshares.value ) ) / ( two_s + ( 1000000 * old_vote_rshares.value ) ) ).to_uint64();
   uint64_t new_weight = ( ( std::numeric_limits< uint64_t >::max() * fc::uint128_t( 1000000 * comment_vote_rshares.value ) ) / ( two_s + ( 1000000 * comment_vote_rshares.value ) ) ).to_uint64();
   return new_weight - old_weight;
}

void TSteemVotingHelper::UpdateVoterParams(const account_object& voter, const uint16_t& newVotingPower,
                                 const time_point_sec& newLastVoteTime)
{
   DB.modify( voter, [&]( account_object& a ){
      a.voting_power = newVotingPower;
      a.last_vote_time = newLastVoteTime;
   });
}

void TSteemVotingHelper::UpdateComment( const comment_object& comment, const int64_t& vote_rshares, const int64_t& vote_absRshares )
{
   DB.modify( comment, [&]( comment_object& c ){
      c.net_rshares += vote_rshares;
      c.abs_rshares += vote_absRshares;
      if( vote_rshares > 0 )
         c.vote_rshares += vote_rshares;
#pragma message( "TODO: Exclude net_votes modification outside, so it was done once per vote." )
      if( vote_rshares > 0 )
         c.net_votes++;
      else
         c.net_votes--;
      if( !DB.has_hardfork( STEEM_HARDFORK_0_6__114 ) && c.net_rshares == -c.abs_rshares)
         FC_ASSERT( c.net_votes < 0, "Comment has negative net votes?" );
   });
}

void TSteemVotingHelper::UpdateCommentRecast( const comment_object& comment, const comment_vote_object& vote, 
                                    const int64_t& recast_rshares, const int64_t& vote_absRshares )
{
   DB.modify( comment, [&]( comment_object& c )
   {
      c.net_rshares -= vote.rshares;
      c.net_rshares += recast_rshares;
      c.abs_rshares += vote_absRshares;
#pragma message( "TODO: Exclude net_votes modification outside, so it was done once per vote." )
      /// TODO: figure out how to handle remove a vote (rshares == 0 )
      if( recast_rshares > 0 && vote.rshares < 0 )
         c.net_votes += 2;
      else if( recast_rshares > 0 && vote.rshares == 0 )
         c.net_votes += 1;
      else if( recast_rshares == 0 && vote.rshares < 0 )
         c.net_votes += 1;
      else if( recast_rshares == 0 && vote.rshares > 0 )
         c.net_votes -= 1;
      else if( recast_rshares < 0 && vote.rshares == 0 )
         c.net_votes -= 1;
      else if( recast_rshares < 0 && vote.rshares > 0 )
         c.net_votes -= 2;
   });
}

void TSteemVotingHelper::UpdateRootComment( const comment_object& root, const int64_t& vote_absRshares, const fc::uint128_t& avg_cashout_sec )
{
   DB.modify( root, [&]( comment_object& c )
   {
      c.children_abs_rshares += vote_absRshares;

      if( !DB.has_hardfork( STEEM_HARDFORK_0_17__769 ) )
      {
         if( DB.has_hardfork( STEEM_HARDFORK_0_12__177 ) && c.last_payout > fc::time_point_sec::min() )
            c.cashout_time = c.last_payout + STEEM_SECOND_CASHOUT_WINDOW;
         else
            c.cashout_time = fc::time_point_sec( std::min( uint32_t( avg_cashout_sec.to_uint64() ), c.max_cashout_time.sec_since_epoch() ) );

         if( c.max_cashout_time == fc::time_point_sec::maximum() )
            c.max_cashout_time = DB.head_block_time() + fc::seconds( STEEM_MAX_CASHOUT_WINDOW_SECONDS );
      }
   });
}

void TSteemVotingHelper::UpdateCommentVoteObject( const comment_vote_object& cvo, const uint64_t& vote_weight, const int64_t& rshares )
{
   DB.modify( cvo, [&]( comment_vote_object& cv ){
      cv.rshares = rshares;
      cv.weight = vote_weight;
   });
}

void TSteemVotingHelper::UpdateCommentRshares2( const comment_object& c, const fc::uint128_t& old_rshares2, const fc::uint128_t& new_rshares2 )
{
#pragma message("TODO: Find out whether SMT need their counterpart of total_reward_shares2")
if( !DB.has_hardfork( STEEM_HARDFORK_0_17__774) )
   DB.adjust_rshares2( c, old_rshares2, new_rshares2 );
}

void TSteemVotingHelper::UpdateVote( const comment_vote_object& vote, const int64_t& rshares, const int16_t& opWeight )
{
   DB.modify( vote, [&]( comment_vote_object& cv )
   {
      cv.rshares = rshares;
      cv.vote_percent = opWeight;
      cv.last_update = DB.head_block_time();
      cv.weight = 0;
      cv.num_changes += 1;
   });
}

} } // steem::chain
