#include <steem/chain/optional_action_evaluator.hpp>
#include <steem/chain/smt_objects.hpp>

#include <steem/chain/util/smt_token.hpp>

#include <steem/protocol/smt_util.hpp>

namespace steem { namespace chain {

using namespace protocol::smt::unit_target;

#ifdef IS_TEST_NET

void example_optional_evaluator::do_apply( const example_optional_action& a ) {}

#endif

void smt_token_emission_evaluator::do_apply( const smt_token_emission_action& a )
{
   const auto& token = _db.get< smt_token_object, by_symbol >( a.symbol );
   auto next_emission = util::smt::next_emission_time( _db, a.symbol, token.last_token_emission );

   FC_ASSERT( next_emission.valid(), "SMT ${t} has no upcoming emission events.", ("t", a.symbol) );
   FC_ASSERT( *next_emission >= _db.head_block_time(), "Emission for ${t} is occuring before next emission time. Now: ${n} Next Emission Time: ${e}",
      ("t", a.symbol)("n", _db.head_block_time())("e", *next_emission) );

   // TODO: Generate emission and check equality.
   share_type market_maker_tokens = 0;
   share_type reward_tokens = 0;
   share_type vesting_tokens = 0;
   share_type total_new_supply = 0;

   for( auto e : a.emissions )
   {
      if( is_market_maker( e.first ) )
      {
         market_maker_tokens += e.second;
      }
      else if( is_rewards( e.first ) )
      {
         reward_tokens += e.second;
      }
      else if( is_vesting( e.first ) )
      {
         vesting_tokens += e.second;
      }

      total_new_supply += e.second;
   }

   _db.modify( token, [&]( smt_token_object& t )
   {
      t.market_maker.token_balance.amount += market_maker_tokens;
      t.reward_balance.amount += reward_tokens;
      t.total_vesting_fund_smt += vesting_tokens;
      t.current_supply += total_new_supply;
   });
}

} } //steem::chain
