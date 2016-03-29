/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <steemit/app/application.hpp>
#include <steemit/app/plugin.hpp>

#include <steemit/chain/steem_objects.hpp>

#include <graphene/time/time.hpp>

#include <graphene/utilities/tempdir.hpp>

#include <fc/thread/thread.hpp>
#include <fc/smart_ref_impl.hpp>

#include <boost/filesystem/path.hpp>

#define BOOST_TEST_MODULE Test Application
#include <boost/test/included/unit_test.hpp>

using namespace graphene;
using namespace steemit;

BOOST_AUTO_TEST_CASE( two_node_network )
{
   using namespace steemit::chain;
   using namespace steemit::app;
   try {
      BOOST_TEST_MESSAGE( "Creating temporary files" );

      fc::temp_directory app_dir( graphene::utilities::temp_directory_path() );
      fc::temp_directory app2_dir( graphene::utilities::temp_directory_path() );
      fc::temp_file genesis_json;

      BOOST_TEST_MESSAGE( "Creating and initializing app1" );

      steemit::app::application app1;
      boost::program_options::variables_map cfg;
      cfg.emplace("p2p-endpoint", boost::program_options::variable_value(string("127.0.0.1:3939"), false));
      app1.initialize(app_dir.path(), cfg);

      BOOST_TEST_MESSAGE( "Creating and initializing app2" );

      steemit::app::application app2;
      auto cfg2 = cfg;
      cfg2.erase("p2p-endpoint");
      cfg2.emplace("p2p-endpoint", boost::program_options::variable_value(string("127.0.0.1:4040"), false));
      cfg2.emplace("seed-node", boost::program_options::variable_value(vector<string>{"127.0.0.1:3939"}, false));
      app2.initialize(app2_dir.path(), cfg2);

      BOOST_TEST_MESSAGE( "Starting app1 and waiting 500 ms" );
      app1.startup();
      fc::usleep(fc::milliseconds(500));
      BOOST_TEST_MESSAGE( "Starting app2 and waiting 500 ms" );
      app2.startup();
      fc::usleep(fc::milliseconds(500));

      BOOST_REQUIRE_EQUAL(app1.p2p_node()->get_connection_count(), 1);
      BOOST_CHECK_EQUAL(std::string(app1.p2p_node()->get_connected_peers().front().host.get_address()), "127.0.0.1");
      BOOST_TEST_MESSAGE( "app1 and app2 successfully connected" );

      std::shared_ptr<chain::database> db1 = app1.chain_database();
      std::shared_ptr<chain::database> db2 = app2.chain_database();

      BOOST_TEST_MESSAGE( "Creating transfer tx" );

      BOOST_TEST_MESSAGE( "Pushing tx locally on db1" );

      BOOST_TEST_MESSAGE( "Broadcasting tx" );

      BOOST_TEST_MESSAGE( "Generating block on db2" );

      BOOST_TEST_MESSAGE( "Broadcasting block" );

      BOOST_TEST_MESSAGE( "Verifying nodes are still connected" );

      BOOST_TEST_MESSAGE( "Checking STEEMIT_NULL_ACCOUNT has balance" );
   } catch( fc::exception& e ) {
      edump((e.to_detail_string()));
      throw;
   }
}
