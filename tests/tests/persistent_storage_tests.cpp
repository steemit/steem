
#include <steem/utilities/persistent_storage.hpp>

#include <boost/test/unit_test.hpp>
#include <sys/stat.h>
#include <stdio.h>

#include <iostream>
#include <fstream>

using namespace steem::utilities;

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

void ah_sequences( rocksdb_types::key_value_items& sequences )
{
   sequences.clear();

   sequences.emplace_back( std::make_pair( "OPERATION_SEQ_ID", 0 ) );
   sequences.emplace_back( std::make_pair( "AH_SEQ_ID", 0 ) );
}

void ah_version( rocksdb_types::key_value_items& version )
{
   version.clear();

   version.emplace_back( std::make_pair( "AH_STORE_MAJOR_VERSION", 5 ) );
   version.emplace_back( std::make_pair( "AH_STORE_MINOR_VERSION", 10 ) );
}

BOOST_AUTO_TEST_SUITE(persistent_storage_tests)

BOOST_AUTO_TEST_CASE(persistent_storage_basic_tests)
{
   const std::string directory_name = "storage_configuration_tests_directory";
   const std::string config_ini_name = directory_name + "/config.ini";

   std::remove( directory_name.c_str() );
   storage_configuration_helper::create_directory( directory_name );

   std::ofstream f( config_ini_name );//config.ini is empty
   f.close();

   {
      storage_configuration_manager scm;

      const char* argv[] = {
                     "path",
                     "-d", "storage_configuration_tests_directory",
                     "--storage-root-path", "root",
                     "--account_history-storage-path", "account_history_storage"
                     };

      rocksdb_types::column_definitions_preparer preparer = ah_column_definitions;

      rocksdb_types::key_value_items seqs;
      rocksdb_types::key_value_items version;
      ah_sequences( seqs );
      ah_version( version );

      scm.add_plugin( "account_history", preparer, seqs, version );
      scm.initialize( 7, (char**)argv );

      std::unique_ptr< abstract_persistent_storage > storage( new persistent_storage( "account_history", scm ) );

      BOOST_REQUIRE( storage->create() );

      BOOST_REQUIRE( storage->open() );
      BOOST_REQUIRE( storage->is_opened() );

      BOOST_REQUIRE( storage->close() );
      BOOST_REQUIRE( storage->is_closed() );
   }
}

BOOST_AUTO_TEST_SUITE_END()
