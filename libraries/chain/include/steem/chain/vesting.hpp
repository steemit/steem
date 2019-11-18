#pragma once

#include <steem/protocol/asset.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/account_object.hpp>

namespace steem { namespace chain {

// Create vesting, then a caller-supplied callback after determining how many shares to create, but before
// we modify the database.
// This allows us to implement virtual op pre-notifications in the Before function.
template< typename Before >
asset create_vesting2( database& db, const account_object& to_account, asset liquid, bool to_reward_balance, Before&& before_vesting_callback )
{
   try
   {
      if ( to_account.name == STEEM_NULL_ACCOUNT )
      {
         FC_ASSERT( liquid.amount >= 0, "The null account cannot be withdrawn from." );
         asset new_vesting = asset( 0, liquid.symbol.get_paired_symbol() );
         before_vesting_callback( new_vesting );
         db.adjust_supply( -liquid );
         return new_vesting;
      }

      auto calculate_new_vesting = [ liquid ] ( price vesting_share_price ) -> asset
         {
         /**
          *  The ratio of total_vesting_shares / total_vesting_fund_steem should not
          *  change as the result of the user adding funds
          *
          *  V / C  = (V+Vn) / (C+Cn)
          *
          *  Simplifies to Vn = (V * Cn ) / C
          *
          *  If Cn equals o.amount, then we must solve for Vn to know how many new vesting shares
          *  the user should receive.
          *
          *  128 bit math is requred due to multiplying of 64 bit numbers. This is done in asset and price.
          */
         asset new_vesting = liquid * ( vesting_share_price );
         return new_vesting;
         };

      if( liquid.symbol.space() == asset_symbol_type::smt_nai_space )
      {
         FC_ASSERT( liquid.symbol.is_vesting() == false );

         const auto& token = db.get< smt_token_object, by_symbol >( liquid.symbol );

         if ( to_reward_balance )
            FC_ASSERT( token.allow_voting == true, "Cannot create vests for the reward balance when voting is disabled. Please report this as it should not be triggered." );

         price vesting_share_price = to_reward_balance ? token.get_reward_vesting_share_price() : token.get_vesting_share_price();

         // Calculate new vesting from provided liquid using share price.
         asset new_vesting = calculate_new_vesting( vesting_share_price );
         before_vesting_callback( new_vesting );

         // Add new vesting to owner's balance.
         if( to_reward_balance )
            db.adjust_reward_balance( to_account, liquid, new_vesting );
         else
            db.adjust_balance( to_account, new_vesting );

         // Update global vesting pool numbers.
         db.modify( token, [&]( smt_token_object& smt_object )
         {
            if( to_reward_balance )
            {
               smt_object.pending_rewarded_vesting_shares += new_vesting.amount;
               smt_object.pending_rewarded_vesting_smt += liquid.amount;
            }
            else
            {
               smt_object.total_vesting_fund_smt += liquid.amount;
               smt_object.total_vesting_shares += new_vesting.amount;
            }
         } );

         // Note: SMT vesting does not impact witness voting.

         return new_vesting;
      }

      FC_ASSERT( liquid.symbol == STEEM_SYMBOL );
      // ^ A novelty, needed but risky in case someone managed to slip SBD/TESTS here in blockchain history.
      // Get share price.
      const auto& cprops = db.get_dynamic_global_properties();
      price vesting_share_price = to_reward_balance ? cprops.get_reward_vesting_share_price() : cprops.get_vesting_share_price();
      // Calculate new vesting from provided liquid using share price.
      asset new_vesting = calculate_new_vesting( vesting_share_price );
      before_vesting_callback( new_vesting );
      // Add new vesting to owner's balance.
      if( to_reward_balance )
      {
         db.adjust_reward_balance( to_account, liquid, new_vesting );
      }
      else
      {
         if( db.has_hardfork( STEEM_HARDFORK_0_20__2539 ) )
         {
            db.modify( to_account, [&]( account_object& a )
            {
               util::update_manabar(
                  cprops,
                  a,
                  STEEM_VOTING_MANA_REGENERATION_SECONDS,
                  db.has_hardfork( STEEM_HARDFORK_0_21__3336 ),
                  new_vesting.amount.value );
            });
         }

         db.adjust_balance( to_account, new_vesting );
      }
      // Update global vesting pool numbers.
      db.modify( cprops, [&]( dynamic_global_property_object& props )
      {
         if( to_reward_balance )
         {
            props.pending_rewarded_vesting_shares += new_vesting;
            props.pending_rewarded_vesting_steem += liquid;
         }
         else
         {
            props.total_vesting_fund_steem += liquid;
            props.total_vesting_shares += new_vesting;
         }
      } );
      // Update witness voting numbers.
      if( !to_reward_balance )
         db.adjust_proxied_witness_votes( to_account, new_vesting.amount );

      return new_vesting;
   }
   FC_CAPTURE_AND_RETHROW( (to_account.name)(liquid) )
}

} } // steem::chain
