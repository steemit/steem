#pragma once

#include <steem/utilities/storage_configuration_manager.hpp>

#include <steem/protocol/types.hpp>

#include <rocksdb/db.h>

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

namespace steem { namespace utilities {

namespace bfs = boost::filesystem;

class abstract_persistent_storage
{
   public:

      using self = std::unique_ptr< abstract_persistent_storage >;
      using ptrDB = std::unique_ptr< DB >;

   public:

      virtual ~abstract_persistent_storage() = default;

      virtual bool create() = 0;
      virtual bool open() = 0;
      virtual bool flush() = 0;
      virtual bool close() = 0;

      virtual bool is_opened() = 0;

      virtual ptrDB& get_storage() = 0;
};

class persistent_storage: public abstract_persistent_storage
{
   private:

      using t_cachable_write_batch = cachable_write_batch< any_info >;

   private:

      //Current status for given storage.
      bool opened = false;

      t_cachable_write_batch write_buffer;
      std::vector< ColumnFamilyHandle* > columnHandles;

      rocksdb_types::key_value_items sequences;
      rocksdb_types::key_value_items version;

      const std::string& plugin_name;
      const storage_configuration_manager& config_manager;

      ptrDB storage;

      void store_sequence_ids();
      void load_seq_identifiers();

      void flush_write_buffer( DB* _db = nullptr );
      void cleanup_column_handles();

      void save_store_version_item( const rocksdb_types::key_value_pair& item );
      void verify_store_version_item( const rocksdb_types::key_value_pair& item );
      void save_store_version();
      void verify_store_version();

      bool shutdown_db();
      bool flush_storage();

      bool check( bool create_action );
      void prepare_columns( bool add_default_column, rocksdb_types::ColumnDefinitions& columns_defs );
      bool create_db();
      bool open_db();

      bool action( std::function< bool() > call );

   public:

      persistent_storage( const std::string& _plugin_name, const storage_configuration_manager& _config_manager );
      virtual ~persistent_storage();

      bool create() override;
      bool open() override;
      bool flush() override;
      bool close() override;

      virtual bool is_opened() override;

      ptrDB& get_storage() override;
};

} } // steem::utilities
