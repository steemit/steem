#include <steem/chain/steem_fwd.hpp>
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
   auto next_emission = util::smt::next_emission_time( _db, a.symbol, token.last_virtual_emission_time );

   FC_ASSERT( next_emission.valid(), "SMT ${t} has no upcoming emission events.", ("t", a.symbol) );
   FC_ASSERT( *next_emission == a.emission_time, "Emission for ${t} is occurring before next emission time. Now: ${n} Next Emission Time: ${e}",
      ("t", a.symbol)("n", a.emission_time)("e", *next_emission) );

   auto emission_obj = util::smt::get_emission_object( _db, token.liquid_symbol, *next_emission );
   FC_ASSERT( emission_obj != nullptr, "Unable to find applicable emission object for scheduled emission. Emission time: ${t}", ("t", *next_emission) );

   auto emissions = util::smt::generate_emissions( token, *emission_obj, *next_emission );
   FC_ASSERT( a.emissions.size() == emissions.size(),
      "Emission generation size mismatch. Expected: ${e}, Actual: ${a}", ("e", emissions.size())("a", a.emissions.size()) );

   for ( auto& e : a.emissions )
      FC_ASSERT( a.emissions.at( e.first ) == emissions.at( e.first ),
         "Emission generation mismatch on unit target '${name}'. Expected: ${a}, Actual: ${b}",
            ("name", e.first)("a", emissions.at( e.first ))("b", a.emissions.at( e.first )) );

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
      else if( is_founder_vesting( e.first ) )
      {
         _db.create_vesting( _db.get_account( get_unit_target_account( e.first ) ), asset( e.second, a.symbol.get_liquid_symbol() ) );
      }
      else
      {
         // This assertion should never fail
         FC_ASSERT( is_account_name_type( e.first ), "Invalid SMT Emission Destination! ${d}", ("d", e.first) );
         _db.adjust_balance( get_unit_target_account( e.first ), asset( e.second, a.symbol.get_liquid_symbol() ) );
      }

      total_new_supply += e.second;
   }

   _db.modify( token, [&]( smt_token_object& t )
   {
      t.market_maker.token_balance.amount += market_maker_tokens;
      t.reward_balance.amount += reward_tokens;
      t.total_vesting_fund_smt += vesting_tokens;
      t.current_supply += total_new_supply;
      t.last_virtual_emission_time = a.emission_time;
   } );
}

} } //steem::chain
