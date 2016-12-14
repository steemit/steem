#include <boost/test/unit_test.hpp>

#include <steemit/protocol/exceptions.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/chain/hardfork.hpp>
#include <steemit/chain/steem_objects.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

#include <iostream>

using namespace steemit;
using namespace steemit::chain;
using namespace steemit::protocol;

#ifndef IS_TEST_NET

BOOST_FIXTURE_TEST_SUITE( live_tests, live_database_fixture )

/*
BOOST_AUTO_TEST_CASE( vests_stock_split )
{
   try
   {
      BOOST_TEST_MESSAGE( "Gathering state prior to split" );

      uint32_t magnitude = 1000000;

      flat_map< string, share_type > account_vests;
      flat_map< string, share_type > account_vsf_votes;
      const auto& acnt_idx = db.get_index< account_index >().indices().get< by_name >();
      auto acnt_itr = acnt_idx.begin();

      BOOST_TEST_MESSAGE( "Saving account vesting shares" );

      while( acnt_itr != acnt_idx.end() )
      {
         account_vests[acnt_itr->name] = acnt_itr->vesting_shares.amount;
         account_vsf_votes[acnt_itr->name] = acnt_itr->proxied_vsf_votes_total().value;
         acnt_itr++;
      }

      auto old_virtual_supply = db.get_dynamic_global_properties().virtual_supply;
      auto old_current_supply = db.get_dynamic_global_properties().current_supply;
      auto old_vesting_fund = db.get_dynamic_global_properties().total_vesting_fund_steem;
      auto old_vesting_shares = db.get_dynamic_global_properties().total_vesting_shares;
      auto old_rshares2 = db.get_dynamic_global_properties().total_reward_shares2;
      auto old_reward_fund = db.get_dynamic_global_properties().total_reward_fund_steem;

      flat_map< std::tuple< account_name_type, string >, share_type > comment_net_rshares;
      flat_map< std::tuple< account_name_type, string >, share_type > comment_abs_rshares;
      flat_map< comment_id_type, uint64_t > total_vote_weights;
      flat_map< comment_id_type, uint64_t > orig_vote_weight;
      flat_map< comment_id_type, uint64_t > expected_reward;
      fc::uint128_t total_rshares2 = 0;
      const auto& com_idx = db.get_index< comment_index >().indices().get< by_permlink >();
      auto com_itr = com_idx.begin();
      auto gpo = db.get_dynamic_global_properties();

      BOOST_TEST_MESSAGE( "Saving comment rshare values" );

      while( com_itr != com_idx.end() )
      {
         comment_net_rshares[ std::make_tuple( com_itr->author, com_itr->permlink ) ] = com_itr->net_rshares;
         comment_abs_rshares[ std::make_tuple( com_itr->author, com_itr->permlink ) ] = com_itr->abs_rshares;
         total_vote_weights[ com_itr->id ] = 0;
         orig_vote_weight[ com_itr->id ] = com_itr->total_vote_weight;

         if( com_itr->net_rshares.value > 0 )
         {
            total_rshares2 += com_itr->net_rshares.value > 0 ? fc::uint128_t( com_itr->net_rshares.value ) * com_itr->net_rshares.value * magnitude * magnitude : 0;
            u256 rs( com_itr->net_rshares.value );
            u256 rf( gpo.total_reward_fund_steem.amount.value );
            auto rs2 = rs * rs;
            u256 rshares2 = old_rshares2.hi;
            rshares2 = rshares2 << 64;
            rshares2 += old_rshares2.lo;
            expected_reward[ com_itr->id ] = static_cast< uint64_t >( rf * rs2 / rshares2 );
         }
         com_itr++;
      }

      BOOST_TEST_MESSAGE( "Saving category rshares" );

      const auto& cat_idx = db.get_index< category_index >().indices();
      flat_map< category_id_type, share_type > category_rshares;

      for( auto cat_itr = cat_idx.begin(); cat_itr != cat_idx.end(); cat_itr++ )
      {
         category_rshares[ cat_itr->id ] = cat_itr->abs_rshares;
      }

      BOOST_TEST_MESSAGE( "Perform split" );
      fc::time_point start = fc::time_point::now();
      db.perform_vesting_share_split( magnitude );
      fc::time_point end = fc::time_point::now();
      ilog( "Vesting split execution time: ${t} us", ("t",end - start) );

      BOOST_TEST_MESSAGE( "Verify split took place correctly" );

      BOOST_REQUIRE( db.get_dynamic_global_properties().current_supply == old_current_supply );
      BOOST_REQUIRE( db.get_dynamic_global_properties().virtual_supply == old_virtual_supply );
      BOOST_REQUIRE( db.get_dynamic_global_properties().total_vesting_fund_steem == old_vesting_fund );
      BOOST_REQUIRE( db.get_dynamic_global_properties().total_vesting_shares.amount == old_vesting_shares.amount * magnitude );
      BOOST_REQUIRE( db.get_dynamic_global_properties().total_reward_shares2 == total_rshares2 );
      BOOST_REQUIRE( db.get_dynamic_global_properties().total_reward_fund_steem == old_reward_fund );

      BOOST_TEST_MESSAGE( "Check accounts were updated" );
      acnt_itr = acnt_idx.begin();
      while( acnt_itr != acnt_idx.end() )
      {
         BOOST_REQUIRE( acnt_itr->vesting_shares.amount == account_vests[ acnt_itr->name ] * magnitude );
         BOOST_REQUIRE( acnt_itr->proxied_vsf_votes_total().value == account_vsf_votes[ acnt_itr->name ] * magnitude );
         acnt_itr++;
      }

      gpo = db.get_dynamic_global_properties();

      com_itr = com_idx.begin();
      while( com_itr != com_idx.end() )
      {
         BOOST_REQUIRE( com_itr->net_rshares == comment_net_rshares[ std::make_tuple( com_itr->author, com_itr->permlink ) ] * magnitude );
         BOOST_REQUIRE( com_itr->abs_rshares == comment_abs_rshares[ std::make_tuple( com_itr->author, com_itr->permlink ) ] * magnitude );
         BOOST_REQUIRE( com_itr->total_vote_weight == total_vote_weights[ com_itr->id ] );

         if( com_itr->net_rshares.value > 0 )
         {
            u256 rs( com_itr->net_rshares.value );
            u256 rf( gpo.total_reward_fund_steem.amount.value );
            u256 rshares2 = total_rshares2.hi;
            rshares2 = ( rshares2 << 64 ) + total_rshares2.lo;
            auto rs2 = rs * rs;

            BOOST_REQUIRE( static_cast< uint64_t >( ( rf * rs2 ) / rshares2 ) == expected_reward[ com_itr->id] );
         }
         com_itr++;
      }

      for( auto cat_itr = cat_idx.begin(); cat_itr != cat_idx.end(); cat_itr++ )
      {
         BOOST_REQUIRE( cat_itr->abs_rshares.value == category_rshares[ cat_itr->id ].value * magnitude );
      }

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}*/

BOOST_AUTO_TEST_CASE( retally_votes )
{
   try
   {
      flat_map< witness_id_type, share_type > expected_votes;

      const auto& by_account_witness_idx = db.get_index< witness_vote_index >().indices();

      for( auto vote: by_account_witness_idx )
      {
         if( expected_votes.find( vote.witness ) == expected_votes.end() )
            expected_votes[ vote.witness ] = db.get( vote.account ).witness_vote_weight();
         else
            expected_votes[ vote.witness ] += db.get( vote.account ).witness_vote_weight();
      }

      db.retally_witness_votes();

      const auto& witness_idx = db.get_index< witness_index >().indices();

      for( auto witness: witness_idx )
      {
         BOOST_REQUIRE_EQUAL( witness.votes.value, expected_votes[ witness.id ].value );
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

#endif
