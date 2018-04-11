
#include <voting_helper.hpp>

#include <steem/chain/comment_object.hpp>
#include <steem/chain/database.hpp>

#include <steem/chain/util/reward.hpp>

//#include <fc/uint128.hpp>

namespace steem { namespace chain {

t_steem_voting_helper::t_steem_voting_helper(database& db) : _db(db)
   {
      const dynamic_global_property_object& dgpo = _db.get_dynamic_global_properties();
      _votes_per_regeneration_period = dgpo.vote_power_reserve_rate;
   }

const share_type& t_steem_voting_helper::get_comment_net_rshares( const comment_object& comment ) { return comment.net_rshares; }
const share_type& t_steem_voting_helper::get_comment_vote_rshares( const comment_object& comment ) { return comment.vote_rshares; }

void t_steem_voting_helper::increase_comment_total_vote_weight( const comment_object& comment, uint64_t delta )
   {
      _db.modify( comment, [&]( comment_object& c )
      {
         c.total_vote_weight += delta;
      });
   }

const comment_vote_object& t_steem_voting_helper::create_comment_vote_object( const account_id_type& voter_id,
                                                                              const comment_id_type& comment_id, int16_t op_weight )
{
   if( _comment_vote_object == nullptr )
      _comment_vote_object = &_db.create<comment_vote_object>( [&]( comment_vote_object& cv ){
         cv.voter   = voter_id;
         cv.comment = comment_id;
         cv.vote_percent = op_weight;
         cv.last_update = _db.head_block_time();
      });

   return *_comment_vote_object;
}

fc::uint128_t t_steem_voting_helper::calculate_avg_cashout_sec( const comment_object& comment, const comment_object& root,
                                                                int64_t voter_abs_rshares, bool is_recast )
{
   // Note that SMT implementation returns 0 from here, thus children_abs_rshares stays a STEEM only field.
   if( _db.has_hardfork( STEEM_HARDFORK_0_17__769 ) )
      return fc::uint128_t();

   auto old_root_abs_rshares = root.children_abs_rshares.value;

   fc::uint128_t cur_cashout_time_sec = _db.calculate_discussion_payout_time( comment ).sec_since_epoch();
   fc::uint128_t new_cashout_time_sec;

   if( _db.has_hardfork( STEEM_HARDFORK_0_12__177 ) && !_db.has_hardfork( STEEM_HARDFORK_0_13__257)  )
      new_cashout_time_sec = _db.head_block_time().sec_since_epoch() + STEEM_CASHOUT_WINDOW_SECONDS_PRE_HF17;
   else
      new_cashout_time_sec = _db.head_block_time().sec_since_epoch() + STEEM_CASHOUT_WINDOW_SECONDS_PRE_HF12;

   if( is_recast )
   {
      if( _db.has_hardfork( STEEM_HARDFORK_0_14__259 ) && voter_abs_rshares == 0 )
         return cur_cashout_time_sec;
      else
         return ( cur_cashout_time_sec * old_root_abs_rshares + new_cashout_time_sec * voter_abs_rshares ) /
                  ( old_root_abs_rshares + voter_abs_rshares );
   }
   else
      return ( cur_cashout_time_sec * old_root_abs_rshares + new_cashout_time_sec * voter_abs_rshares ) /
               ( old_root_abs_rshares + voter_abs_rshares );
}

uint64_t t_steem_voting_helper::calculate_comment_vote_weight( const comment_object& comment, int64_t rshares,
                                                               const share_type& old_vote_rshares )
{
   share_type comment_vote_rshares = get_comment_vote_rshares( comment );
   if( comment.created < fc::time_point_sec(STEEM_HARDFORK_0_6_REVERSE_AUCTION_TIME) ) {
      u512 rshares3(rshares);
      u256 total2( comment.abs_rshares.value );

      if( !_db.has_hardfork( STEEM_HARDFORK_0_1 ) )
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
   if( _db.has_hardfork( STEEM_HARDFORK_0_17__774 ) )
   {
#pragma message( "TODO: Make this block common with SMT implementation of this method." )
#pragma message( "TODO: Use abstract methods to aquire reward curve and content constant here (stored in SMT object)." )
      const auto& reward_fund = _db.get_reward_fund( comment );
      auto curve = !_db.has_hardfork( STEEM_HARDFORK_0_19__1052 ) && comment.created > STEEM_HF_19_SQRT_PRE_CALC
                     ? curve_id::square_root : reward_fund.curation_reward_curve;
      uint64_t old_weight = util::evaluate_reward_curve( old_vote_rshares.value, curve, reward_fund.content_constant ).to_uint64();
      uint64_t new_weight = util::evaluate_reward_curve( comment_vote_rshares.value, curve, reward_fund.content_constant ).to_uint64();
      return new_weight - old_weight;
   }
   
   if ( _db.has_hardfork( STEEM_HARDFORK_0_1 ) )
   {
      uint64_t old_weight = ( ( std::numeric_limits< uint64_t >::max() * fc::uint128_t( old_vote_rshares.value ) ) / ( two_s + old_vote_rshares.value ) ).to_uint64();
      uint64_t new_weight = ( ( std::numeric_limits< uint64_t >::max() * fc::uint128_t( comment_vote_rshares.value ) ) / ( two_s + comment_vote_rshares.value ) ).to_uint64();
      return new_weight - old_weight;
   }

   uint64_t old_weight = ( ( std::numeric_limits< uint64_t >::max() * fc::uint128_t( 1000000 * old_vote_rshares.value ) ) / ( two_s + ( 1000000 * old_vote_rshares.value ) ) ).to_uint64();
   uint64_t new_weight = ( ( std::numeric_limits< uint64_t >::max() * fc::uint128_t( 1000000 * comment_vote_rshares.value ) ) / ( two_s + ( 1000000 * comment_vote_rshares.value ) ) ).to_uint64();
   return new_weight - old_weight;
}

void t_steem_voting_helper::update_voter_params( const account_object& voter, uint16_t new_voting_power,
                                                 const time_point_sec& new_last_vote_time )
{
   _db.modify( voter, [&]( account_object& a ){
      a.voting_power = new_voting_power;
      a.last_vote_time = new_last_vote_time;
   });
}

void t_steem_voting_helper::update_comment( const comment_object& comment, int64_t vote_rshares, int64_t vote_abs_rshares )
{
   _db.modify( comment, [&]( comment_object& c ){
      c.net_rshares += vote_rshares;
      c.abs_rshares += vote_abs_rshares;
      if( vote_rshares > 0 )
         c.vote_rshares += vote_rshares;
#pragma message( "TODO: Exclude net_votes modification outside, so it was done once per vote." )
      if( vote_rshares > 0 )
         c.net_votes++;
      else
         c.net_votes--;
      if( !_db.has_hardfork( STEEM_HARDFORK_0_6__114 ) && c.net_rshares == -c.abs_rshares)
         FC_ASSERT( c.net_votes < 0, "Comment has negative net votes?" );
   });
}

void t_steem_voting_helper::update_comment_recast( const comment_object& comment, const comment_vote_object& vote, 
                                                   int64_t recast_rshares, int64_t vote_abs_rshares )
{
   _db.modify( comment, [&]( comment_object& c )
   {
      c.net_rshares -= vote.rshares;
      c.net_rshares += recast_rshares;
      c.abs_rshares += vote_abs_rshares;
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

void t_steem_voting_helper::update_root_comment( const comment_object& root, int64_t vote_abs_rshares, const fc::uint128_t& avg_cashout_sec )
{
   _db.modify( root, [&]( comment_object& c )
   {
      c.children_abs_rshares += vote_abs_rshares;

      if( !_db.has_hardfork( STEEM_HARDFORK_0_17__769 ) )
      {
         if( _db.has_hardfork( STEEM_HARDFORK_0_12__177 ) && c.last_payout > fc::time_point_sec::min() )
            c.cashout_time = c.last_payout + STEEM_SECOND_CASHOUT_WINDOW;
         else
            c.cashout_time = fc::time_point_sec( std::min( uint32_t( avg_cashout_sec.to_uint64() ), c.max_cashout_time.sec_since_epoch() ) );

         if( c.max_cashout_time == fc::time_point_sec::maximum() )
            c.max_cashout_time = _db.head_block_time() + fc::seconds( STEEM_MAX_CASHOUT_WINDOW_SECONDS );
      }
   });
}

void t_steem_voting_helper::update_comment_vote_object( const comment_vote_object& cvo, uint64_t vote_weight, int64_t rshares )
{
   _db.modify( cvo, [&]( comment_vote_object& cv ){
      cv.rshares = rshares;
      cv.weight = vote_weight;
   });
}

void t_steem_voting_helper::update_comment_rshares2( const comment_object& c, const fc::uint128_t& old_rshares2,
                                                     const fc::uint128_t& new_rshares2 )
{
#pragma message("TODO: Find out whether SMT need their counterpart of total_reward_shares2")
if( !_db.has_hardfork( STEEM_HARDFORK_0_17__774) )
   _db.adjust_rshares2( c, old_rshares2, new_rshares2 );
}

void t_steem_voting_helper::update_vote( const comment_vote_object& vote, int64_t rshares, int16_t op_weight )
{
   _db.modify( vote, [&]( comment_vote_object& cv )
   {
      cv.rshares = rshares;
      cv.vote_percent = op_weight;
      cv.last_update = _db.head_block_time();
      cv.weight = 0;
      cv.num_changes += 1;
   });
}

t_smt_voting_helper::t_smt_voting_helper(const comment_vote_object& cvo, database& db, const asset_symbol_type& symbol,
   const share_type& max_accepted_payout, bool allowed_curation_awards)
   : _comment_vote_object(cvo)
   {
      FC_ASSERT("Not implemented yet!");
   }

uint32_t t_smt_voting_helper::get_minimal_vote_interval() const  { FC_ASSERT("Not implemented yet!"); return 0; }
uint32_t t_smt_voting_helper::get_vote_regeneration_period() const  { FC_ASSERT("Not implemented yet!"); return 0; }
uint32_t t_smt_voting_helper::get_votes_per_regeneration_period() const  { FC_ASSERT("Not implemented yet!"); return 0; }
uint32_t t_smt_voting_helper::get_reverse_auction_window_seconds()  { FC_ASSERT("Not implemented yet!"); return 0; }

const share_type& t_smt_voting_helper::get_comment_net_rshares( const comment_object& comment )
   {
      FC_ASSERT("Not implemented yet!");
      static share_type temp = 0;
      return temp;
   }

const share_type& t_smt_voting_helper::get_comment_vote_rshares( const comment_object& comment )
   {
      FC_ASSERT("Not implemented yet!");
      static share_type temp = 0;
      return temp;
   }

void t_smt_voting_helper::increase_comment_total_vote_weight( const comment_object& comment, uint64_t delta )
   {
      FC_ASSERT("Not implemented yet!");
   }

const comment_vote_object& t_smt_voting_helper::create_comment_vote_object( const account_id_type& voter_id, const comment_id_type& comment_id,
                                                                            int16_t op_weight )
   {
      FC_ASSERT("Not implemented yet!");
      return _comment_vote_object;
   }

fc::uint128_t t_smt_voting_helper::calculate_avg_cashout_sec( const comment_object& comment, const comment_object& root,
                                                              int64_t voter_abs_rshares, bool is_recast )
   {
      FC_ASSERT("Not implemented yet!");
      return 0;
   }

uint64_t t_smt_voting_helper::calculate_comment_vote_weight( const comment_object& comment, int64_t rshares, 
                                                             const share_type& old_vote_rshares )
   {
      FC_ASSERT("Not implemented yet!");
      return 0;
   }

void t_smt_voting_helper::update_voter_params( const account_object& voter, uint16_t new_voting_power,
                                               const time_point_sec& new_last_vote_time )
   {
      FC_ASSERT("Not implemented yet!");
   }

void t_smt_voting_helper::update_comment( const comment_object& comment, int64_t vote_rshares, int64_t vote_abs_rshares )
   {
      FC_ASSERT("Not implemented yet!");
   }

void t_smt_voting_helper::update_comment_recast( const comment_object& comment, const comment_vote_object& vote, 
                                                 int64_t recast_rshares, int64_t vote_abs_rshares )
   {
      FC_ASSERT("Not implemented yet!");
   }

void t_smt_voting_helper::update_root_comment( const comment_object& root, int64_t vote_abs_rshares, const fc::uint128_t& avg_cashout_sec )
   {
      FC_ASSERT("Not implemented yet!");
   }

void t_smt_voting_helper::update_comment_vote_object( const comment_vote_object& cvo, uint64_t vote_weight, int64_t rshares )
   {
      FC_ASSERT("Not implemented yet!");
   }

void t_smt_voting_helper::update_comment_rshares2( const comment_object& c, const fc::uint128_t& old_rshares2,
                                                   const fc::uint128_t& new_rshares2 )
   {
      FC_ASSERT("Not implemented yet!");
   }

void t_smt_voting_helper::update_vote( const comment_vote_object& vote, int64_t rshares, int16_t op_weight )
   {
      FC_ASSERT("Not implemented yet!");
   }

} } // steem::chain
