#include <boost/test/unit_test.hpp>

#include <fc/bloom_filter.hpp>
#include <fc/exception/exception.hpp>
#include <fc/reflect/variant.hpp>
#include <iostream>
#include <fc/variant.hpp>
#include <fc/io/raw.hpp>
#include <fstream>
#include <fc/io/json.hpp>
#include <fc/crypto/base64.hpp>

using namespace fc;

static bloom_parameters setup_parameters()
{
   bloom_parameters parameters;

   // How many elements roughly do we expect to insert?
   parameters.projected_element_count = 100000;

   // Maximum tolerable false positive probability? (0,1)
   parameters.false_positive_probability = 0.0001; // 1 in 10000

   // Simple randomizer (optional)
   parameters.random_seed = 0xA5A5A5A5;

   if (!parameters)
   {
      BOOST_FAIL( "Error - Invalid set of bloom filter parameters!" );
   }

   parameters.compute_optimal_parameters();

   return parameters;
}

BOOST_AUTO_TEST_SUITE(fc_crypto)

BOOST_AUTO_TEST_CASE(bloom_test_1)
{
   try {

      //Instantiate Bloom Filter
      bloom_filter filter(setup_parameters());

      uint32_t count = 0;
      std::string line;
      std::ifstream in("README.md");
      std::ofstream words("words.txt");
      while( !in.eof() && count < 100000 )
      {
         std::getline(in, line);
//         std::cout << "'"<<line<<"'\n";
         if( !filter.contains(line) )
         {
            filter.insert( line );
            words << line << "\n";
            ++count;
         }
      }
//      wdump((filter));
      auto packed_filter = fc::raw::pack_to_vector(filter);
//      wdump((packed_filter.size()));
//      wdump((packed_filter));
      std::stringstream out;
//      std::string str = fc::json::to_string(packed_filter);
      auto b64 = fc::base64_encode( packed_filter.data(), packed_filter.size() );
      for( uint32_t i = 0; i < b64.size(); i += 1024 )
         out << '"' <<  b64.substr( i, 1024 ) << "\",\n";
   }
   catch ( const fc::exception& e )
   {
      edump((e.to_detail_string()) );
   }
}

BOOST_AUTO_TEST_CASE(bloom_test_2)
{
   try {
      //Instantiate Bloom Filter
      bloom_filter filter(setup_parameters());

      std::string str_list[] = { "AbC", "iJk", "XYZ" };

   // Insert into Bloom Filter
   {
      // Insert some strings
      for (std::size_t i = 0; i < (sizeof(str_list) / sizeof(std::string)); ++i)
      {
         filter.insert(str_list[i]);
      }

      // Insert some numbers
      for (std::size_t i = 0; i < 100; ++i)
      {
         filter.insert(i);
      }
   }

   // Query Bloom Filter
   {
      // Query the existence of strings
      for (std::size_t i = 0; i < (sizeof(str_list) / sizeof(std::string)); ++i)
      {
         BOOST_CHECK( filter.contains(str_list[i]) );
      }

      // Query the existence of numbers
      for (std::size_t i = 0; i < 100; ++i)
      {
         BOOST_CHECK( filter.contains(i) );
      }

      std::string invalid_str_list[] = { "AbCX", "iJkX", "XYZX" };

      // Query the existence of invalid strings
      for (std::size_t i = 0; i < (sizeof(invalid_str_list) / sizeof(std::string)); ++i)
      {
         BOOST_CHECK( !filter.contains(invalid_str_list[i]) );
      }

      // Query the existence of invalid numbers
      for (int i = -1; i > -100; --i)
      {
         BOOST_CHECK( !filter.contains(i) );
      }
   }
   } 
   catch ( const fc::exception& e )
   {
      edump((e.to_detail_string()) );
   }
}

BOOST_AUTO_TEST_SUITE_END()
