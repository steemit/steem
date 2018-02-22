#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/hardfork.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/database_exceptions.hpp>
#include <steem/chain/steem_objects.hpp>

#include <steem/chain/util/reward.hpp>

#include <steem/plugins/witness/witness_objects.hpp>

#include <fc/macros.hpp>
#include <fc/crypto/digest.hpp>

#include "../db_fixture/database_fixture.hpp"
#include "../undo_data/undo.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace steem;
using namespace steem::chain;
using namespace steem::protocol;
using fc::string;

BOOST_FIXTURE_TEST_SUITE( undo_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( undo_basic )
{
   try
   {
      BOOST_TEST_MESSAGE( "--- Testing: undo_basic" );

      undo_db udb( *db );
      undo_scenario< account_object > ao( *db );

      BOOST_TEST_MESSAGE( "--- No object added" );
      ao.remember_old_values< account_index >();
      udb.undo_start();
      udb.undo_end();
      BOOST_REQUIRE( ao.check< account_index >() );

      BOOST_TEST_MESSAGE( "--- Adding 1 object" );
      ao.remember_old_values< account_index >();
      udb.undo_start();

      const account_object& obj0 = ao.create( [&]( account_object& obj ){ obj.name = "name00"; } );
      BOOST_REQUIRE( std::string( obj0.name ) == "name00" );

      udb.undo_end();
      BOOST_REQUIRE( ao.check< account_index >() );

      BOOST_TEST_MESSAGE( "--- Adding 1 object and modify" );
      ao.remember_old_values< account_index >();
      udb.undo_start();

      const account_object& obj1 = ao.create( [&]( account_object& obj ){ obj.name = "name00"; } );
      BOOST_REQUIRE( std::string( obj1.name ) == "name00" );
      const account_object& obj2 = ao.modify( obj1, [&]( account_object& obj ){ obj.name = "name01"; } );
      BOOST_REQUIRE( std::string( obj2.name ) == "name01" );

      udb.undo_end();
      BOOST_REQUIRE( ao.check< account_index >() );

      BOOST_TEST_MESSAGE( "--- Adding 1 object and remove" );
      ao.remember_old_values< account_index >();
      udb.undo_start();

      const account_object& obj3 = ao.create( [&]( account_object& obj ){ obj.name = "name00"; } );
      ao.remove( obj3 );

      udb.undo_end();
      BOOST_REQUIRE( ao.check< account_index >() );

      BOOST_TEST_MESSAGE( "--- Adding 3 objects( create, create/modify, create/remove )" );
      ao.remember_old_values< account_index >();
      udb.undo_start();

      const account_object& obj_c = ao.create( [&]( account_object& obj ){ obj.name = "name00"; } );
      BOOST_REQUIRE( std::string( obj_c.name ) == "name00" );

      const account_object& obj_cm = ao.create( [&]( account_object& obj ){ obj.name = "name01"; } );
      BOOST_REQUIRE( std::string( obj_cm.name ) == "name01" );
      ao.modify( obj_cm, [&]( account_object& obj ){ obj.name = "name02"; } );
      BOOST_REQUIRE( std::string( obj_cm.name ) == "name02" );

      const account_object& obj_cr = ao.create( [&]( account_object& obj ){ obj.name = "name03"; } );
      BOOST_REQUIRE( std::string( obj_cr.name ) == "name03" );
      ao.remove( obj_cr );

      udb.undo_end();
      BOOST_REQUIRE( ao.check< account_index >() );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( undo_key_collision )
{
   try
   {
      BOOST_TEST_MESSAGE( "--- Testing: undo_key_collision" );

      undo_db udb( *db );
      undo_scenario< account_object > ao( *db );

      BOOST_TEST_MESSAGE( "--- Adding 1 object - twice with the same key" );
      ao.remember_old_values< account_index >();
      udb.undo_start();

      ao.create( [&]( account_object& obj ){ obj.name = "name00"; } );
      STEEM_REQUIRE_THROW( ao.create( [&]( account_object& obj ){ obj.name = "name00"; } ), fc::exception );

      udb.undo_end();
      BOOST_REQUIRE( ao.check< account_index >() );

      BOOST_TEST_MESSAGE( "--- Adding 2 objects. Modifying 1 object - uniqueness of complex index is violated" );
      ao.remember_old_values< account_index >();
      udb.undo_start();

      uint32_t old_size = ao.size< account_index >();

      ao.create( [&]( account_object& obj ){ obj.name = "name00"; obj.proxy = "proxy00"; } );
      BOOST_REQUIRE( old_size + 1 == ao.size< account_index >() );

      const account_object& obj1 = ao.create( [&]( account_object& obj ){ obj.name = "name01"; obj.proxy = "proxy01"; } );
      BOOST_REQUIRE( old_size + 2 == ao.size< account_index >() );

      /*
         Important!!!
            Method 'generic_index::modify' works incorrectly - after contraint violation, element is removed(!!!).
         Solution:
            It's necessary to write fix, according to newly created issue.
      */
      //Temporary. After fix, this line should be enabled.
      //STEEM_REQUIRE_THROW( ao.modify( obj1, [&]( account_object& obj ){ obj.name = "name00"; obj.proxy = "proxy00"; } ), fc::exception );
      
      //Temporary. After fix, this line should be removed.
      ao.modify( obj1, [&]( account_object& obj ){ obj.name = "nameXYZ"; obj.proxy = "proxyXYZ"; } );

      BOOST_REQUIRE( old_size + 2 == ao.size< account_index >() );

      udb.undo_end();
      BOOST_REQUIRE( ao.check< account_index >() );
   }
   FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_SUITE_END()
#endif
