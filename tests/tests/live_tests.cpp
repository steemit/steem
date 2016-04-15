#include <boost/test/unit_test.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/chain/exceptions.hpp>
#include <steemit/chain/hardfork.hpp>

#include <steemit/chain/steem_objects.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace steemit::chain;
using namespace steemit::chain::test;

#ifndef IS_TEST_NET

BOOST_FIXTURE_TEST_SUITE( live_tests, live_database_fixture )

BOOST_AUTO_TEST_CASE( vests_stock_split )
{
   try
   {
      BOOST_TEST_MESSAGE( "Gathering state prior to split" );

      uint32_t magnitude = 1000000;

      flat_map< string, share_type > account_vests;
      const auto& acnt_idx = db.get_index_type< account_index >().indices().get< by_name >();
      auto acnt_itr = acnt_idx.begin();

      idump( (acnt_idx.begin()->name)(acnt_idx.end()->name) );

      BOOST_TEST_MESSAGE( "Saving account vesting shares" );

      while( acnt_itr != acnt_idx.end() )
      {
         account_vests[acnt_itr->name] = acnt_itr->vesting_shares.amount;
         acnt_itr++;
      }

      auto old_virtual_supply = db.get_dynamic_global_properties().virtual_supply;
      auto old_current_supply = db.get_dynamic_global_properties().current_supply;
      auto old_vesting_fund = db.get_dynamic_global_properties().total_vesting_fund_steem;
      auto old_vesting_shares = db.get_dynamic_global_properties().total_vesting_shares;
      auto old_rshares2 = db.get_dynamic_global_properties().total_reward_shares2;

      flat_map< std::tuple< string, string >, share_type > comment_net_rshares;
      flat_map< std::tuple< string, string >, share_type > comment_abs_rshares;
      flat_map< std::tuple< string, string >, uint64_t > total_vote_weights;
      fc::uint128_t total_rshares2 = 0;
      const auto& com_idx = db.get_index_type< comment_index >().indices().get< by_permlink >();
      auto com_itr = com_idx.begin();

      BOOST_TEST_MESSAGE( "Saving comment rshare values" );

      while( com_itr != com_idx.end() )
      {
         comment_net_rshares[ std::make_tuple( com_itr->author, com_itr->permlink ) ] = com_itr->net_rshares;
         comment_abs_rshares[ std::make_tuple( com_itr->author, com_itr->permlink ) ] = com_itr->abs_rshares;
         total_vote_weights[ std::make_tuple( com_itr->author, com_itr->permlink ) ] = 0;
         total_rshares2 += fc::uint128_t( com_itr->net_rshares.value ) * com_itr->net_rshares.value * magnitude * magnitude;
         com_itr++;
      }

      flat_map< std::tuple< comment_id_type, account_id_type >, uint64_t > comment_vote_weights;
      const auto& vote_idx = db.get_index_type< comment_vote_index >().indices().get< by_comment_voter >();
      auto vote_itr = vote_idx.begin();

      BOOST_TEST_MESSAGE( "Saving comment vote weights" );

      while( vote_itr != vote_idx.end() )
      {
         comment_vote_weights[ std::make_tuple( vote_itr->comment, vote_itr->voter ) ] = vote_itr->weight;
         vote_itr++;
      }

      BOOST_TEST_MESSAGE( "Perform split" );
      db.perform_vesting_share_split( magnitude );

      BOOST_TEST_MESSAGE( "Verify split took place correctly" );

      BOOST_REQUIRE( db.get_dynamic_global_properties().current_supply == old_current_supply );
      BOOST_REQUIRE( db.get_dynamic_global_properties().virtual_supply == old_virtual_supply );
      BOOST_REQUIRE( db.get_dynamic_global_properties().total_vesting_fund_steem == old_vesting_fund );
      BOOST_REQUIRE( db.get_dynamic_global_properties().total_vesting_shares.amount == old_vesting_shares.amount * magnitude );
      BOOST_REQUIRE( db.get_dynamic_global_properties().total_reward_shares2 == total_rshares2 );

      acnt_itr = acnt_idx.begin();
      while( acnt_itr != acnt_idx.end() )
      {
         BOOST_REQUIRE( acnt_itr->vesting_shares.amount == account_vests[ acnt_itr->name ] * magnitude );
         acnt_itr++;
      }

      com_itr = com_idx.begin();
      while( com_itr != com_idx.end() )
      {
         BOOST_REQUIRE( com_itr->net_rshares == comment_net_rshares[ std::make_tuple( com_itr->author, com_itr->permlink ) ] * magnitude );
         BOOST_REQUIRE( com_itr->abs_rshares == comment_abs_rshares[ std::make_tuple( com_itr->author, com_itr->permlink ) ] * magnitude );
         com_itr++;
      }


      db.generate_block();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

#endif