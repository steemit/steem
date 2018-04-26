
#include <steem/utilities/persistent_storage.hpp>

#include <boost/test/unit_test.hpp>
#include <sys/stat.h>
#include <stdio.h>

#include <iostream>
#include <fstream>

using namespace steem::utilities;

void prepare_directories( std::string main_dir, std::string root_dir )
{
   const std::string config_ini_name = main_dir + "/config.ini";

   storage_configuration_helper::remove_directory( main_dir );
   storage_configuration_helper::remove_directory( root_dir );

   storage_configuration_helper::create_directory( main_dir );

   std::ofstream f( config_ini_name );//config.ini is empty
   f.close();
}

void ah_column_definitions( bool add_default_column, rocksdb_types::ColumnDefinitions& columns )
{
   if( add_default_column )
      columns.emplace_back(::rocksdb::kDefaultColumnFamilyName, rocksdb_types::ColumnFamilyOptions());

   columns.emplace_back("operation_by_id", rocksdb_types::ColumnFamilyOptions());
   auto& byIdColumn = columns.back();
   byIdColumn.options.comparator = rocksdb_types::by_id_Comparator();

   columns.emplace_back("operation_by_location", rocksdb_types::ColumnFamilyOptions());
   auto& byLocationColumn = columns.back();
   byLocationColumn.options.comparator = rocksdb_types::by_location_Comparator();

   columns.emplace_back("account_history_info_by_name", rocksdb_types::ColumnFamilyOptions());
   auto& byAccountNameColumn = columns.back();
   byAccountNameColumn.options.comparator = rocksdb_types::by_account_name_Comparator();

   columns.emplace_back("ah_info_operation_by_ids", rocksdb_types::ColumnFamilyOptions());
   auto& byAHInfoColumn = columns.back();
   byAHInfoColumn.options.comparator = rocksdb_types::by_ah_info_operation_Comparator();
}

rocksdb_types::key_value_items ah_sequences( std::string seq1, std::string seq2 )
{
   rocksdb_types::key_value_items sequences;

   sequences.emplace_back( std::make_pair( seq1, 0 ) );
   sequences.emplace_back( std::make_pair( seq2, 0 ) );

   return sequences;
}

rocksdb_types::key_value_items ah_version( int32_t major, int32_t minor )
{
   rocksdb_types::key_value_items version;

   version.emplace_back( std::make_pair( "AH_STORE_MAJOR_VERSION", major ) );
   version.emplace_back( std::make_pair( "AH_STORE_MINOR_VERSION", minor ) );

   return version;
}

BOOST_AUTO_TEST_SUITE(persistent_storage_tests)

BOOST_AUTO_TEST_CASE(persistent_storage_basic_tests)
{
   {
      prepare_directories( "storage_configuration_tests_directory", "root" );

      const char* argv[] = {
                     "path",
                     "-d", "storage_configuration_tests_directory",
                     "--storage-root-path", "root",
                     "--account_history-storage-path", "account_history_storage"
                     };

      storage_configuration_manager scm;

      BOOST_TEST_MESSAGE( "Creating 1 database" );
      scm.add_plugin( "account_history", ah_column_definitions, ah_sequences( "OPERATION_SEQ_ID", "AH_SEQ_ID" ), ah_version( 1/*major*/, 2/*minor*/ ) );
      scm.initialize( 7, (char**)argv );

      std::unique_ptr< abstract_persistent_storage > storage( new persistent_storage( "account_history", scm ) );
      BOOST_REQUIRE( storage->create() );
      BOOST_REQUIRE( storage->open() );
      BOOST_REQUIRE( storage->is_opened() );
      BOOST_REQUIRE( storage->close() );
      BOOST_REQUIRE( !storage->is_opened() );
   }

   {
      prepare_directories( "storage_configuration_tests_directory", "root" );

      const char* argv[] = {
                     "path",
                     "-d", "storage_configuration_tests_directory",
                     "--storage-root-path", "root",
                     "--account_history-storage-path", "account_history_storage"
                     };

      storage_configuration_manager scm;

      //================================================================
      BOOST_TEST_MESSAGE( "Creating 1 database" );
      scm.add_plugin( "account_history", ah_column_definitions, ah_sequences( "OPERATION_SEQ_ID", "AH_SEQ_ID" ), ah_version( 5/*major*/, 10/*minor*/ ) );
      scm.initialize( 7, (char**)argv );

      std::unique_ptr< abstract_persistent_storage > storage( new persistent_storage( "account_history", scm ) );
      BOOST_REQUIRE( !storage->is_opened() );
      BOOST_REQUIRE( storage->create() );
      BOOST_REQUIRE( !storage->is_opened() );
      //================================================================
      BOOST_TEST_MESSAGE( "Version different from database version. Fail test" );
      scm.reset();
      scm.add_plugin( "account_history", ah_column_definitions, ah_sequences( "OPERATION_SEQ_ID", "AH_SEQ_ID" ), ah_version( 6/*major*/, 10/*minor*/ ) );
      scm.initialize( 7, (char**)argv );

      BOOST_REQUIRE( !storage->is_opened() );
      BOOST_REQUIRE( !storage->open() );
      BOOST_REQUIRE( !storage->is_opened() );
      //================================================================
      BOOST_TEST_MESSAGE( "No version info. Fail test" );
      scm.reset();
      scm.add_plugin( "account_history", ah_column_definitions, ah_sequences( "OPERATION_SEQ_ID", "AH_SEQ_ID" ) );
      scm.initialize( 7, (char**)argv );

      BOOST_REQUIRE( !storage->is_opened() );
      BOOST_REQUIRE( !storage->open() );
      BOOST_REQUIRE( !storage->is_opened() );
      //================================================================
      BOOST_TEST_MESSAGE( "A sequence different from saved earlier. Fail test" );
      scm.reset();
      scm.add_plugin( "account_history", ah_column_definitions, ah_sequences( "XYZ", "AH_SEQ_ID" ), ah_version( 5/*major*/, 10/*minor*/ ) );
      scm.initialize( 7, (char**)argv );

      BOOST_REQUIRE( !storage->is_opened() );
      BOOST_REQUIRE( !storage->open() );
      BOOST_REQUIRE( !storage->is_opened() );
      //================================================================
   }
}

BOOST_AUTO_TEST_SUITE_END()
