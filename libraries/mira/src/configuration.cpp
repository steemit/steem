#include <mira/configuration.hpp>

#define DEFAULT_MIRA_SHARED_CACHE_SIZE (1ull * 1024 * 1024 * 1024 )
#define DEFAULT_MIRA_NUM_SHARD_BITS    (4)

namespace mira {

// Base configuration for an index
#define BASE                             "base"

// Global options
#define GLOBAL                           "global"
#define SHARED_CACHE                     "shared_cache"
#define OBJECT_COUNT                     "object_count"
#define STATISTICS                       "statistics"

// Shared cache options
#define CAPACITY                         "capacity"
#define NUM_SHARD_BITS                   "num_shard_bits"

// Database options
#define ALLOW_MMAP_READS                 "allow_mmap_reads"
#define WRITE_BUFFER_SIZE                "write_buffer_size"
#define MAX_BYTES_FOR_LEVEL_BASE         "max_bytes_for_level_base"
#define TARGET_FILE_SIZE_BASE            "target_file_size_base"
#define MAX_WRITE_BUFFER_NUMBER          "max_write_buffer_number"
#define MAX_BACKGROUND_COMPACTIONS       "max_background_compactions"
#define MAX_BACKGROUND_FLUSHES           "max_background_flushes"
#define MIN_WRITE_BUFFER_NUMBER_TO_MERGE "min_write_buffer_number_to_merge"
#define OPTIMIZE_LEVEL_STYLE_COMPACTION  "optimize_level_style_compaction"
#define INCREASE_PARALLELISM             "increase_parallelism"
#define BLOCK_BASED_TABLE_OPTIONS        "block_based_table_options"
#define BLOCK_SIZE                       "block_size"
#define BLOOM_FILTER_POLICY              "bloom_filter_policy"
#define BITS_PER_KEY                     "bits_per_key"
#define USE_BLOCK_BASED_BUILDER          "use_block_based_builder"

static std::shared_ptr< rocksdb::Cache > global_shared_cache;

static std::map< std::string, std::function< void( ::rocksdb::Options&, fc::variant ) > > global_database_option_map {
   { ALLOW_MMAP_READS,                  []( ::rocksdb::Options& o, fc::variant v ) { o.allow_mmap_reads = v.as< bool >(); }                 },
   { WRITE_BUFFER_SIZE,                 []( ::rocksdb::Options& o, fc::variant v ) { o.write_buffer_size = v.as< uint64_t >(); }            },
   { MAX_BYTES_FOR_LEVEL_BASE,          []( ::rocksdb::Options& o, fc::variant v ) { o.max_bytes_for_level_base = v.as< uint64_t >(); }     },
   { TARGET_FILE_SIZE_BASE,             []( ::rocksdb::Options& o, fc::variant v ) { o.target_file_size_base = v.as< uint64_t >(); }        },
   { MAX_WRITE_BUFFER_NUMBER,           []( ::rocksdb::Options& o, fc::variant v ) { o.max_write_buffer_number = v.as< int >(); }           },
   { MAX_BACKGROUND_COMPACTIONS,        []( ::rocksdb::Options& o, fc::variant v ) { o.max_background_compactions = v.as< int >(); }        },
   { MAX_BACKGROUND_FLUSHES,            []( ::rocksdb::Options& o, fc::variant v ) { o.max_background_flushes = v.as< int >(); }            },
   { MIN_WRITE_BUFFER_NUMBER_TO_MERGE,  []( ::rocksdb::Options& o, fc::variant v ) { o.min_write_buffer_number_to_merge = v.as< int >(); }  },
   { OPTIMIZE_LEVEL_STYLE_COMPACTION,   []( ::rocksdb::Options& o, fc::variant v )
      {
         if ( v.as< bool >() )
            o.OptimizeLevelStyleCompaction();
      }
   },
   { INCREASE_PARALLELISM, []( ::rocksdb::Options& o, fc::variant v )
      {
         if ( v.as< bool >() )
            o.IncreaseParallelism();
      }
   },
   { BLOCK_BASED_TABLE_OPTIONS, []( ::rocksdb::Options& o, auto v )
      {
         ::rocksdb::BlockBasedTableOptions table_options;
         FC_ASSERT( v.is_object(), "Expected '${key}' to be an object",
            ("key", BLOCK_BASED_TABLE_OPTIONS) );

         auto& obj = v.get_object();

         table_options.block_cache = global_shared_cache;

         if ( obj.contains( BLOCK_SIZE ) )
            table_options.block_size = obj[ BLOCK_SIZE ].template as< uint64_t >();

         if ( obj.contains( BLOOM_FILTER_POLICY ) )
         {
            FC_ASSERT( obj[ BLOOM_FILTER_POLICY ].is_object(), "Expected '${key}' to be an object",
               ("key", BLOOM_FILTER_POLICY) );

            auto filter_policy = obj[ BLOOM_FILTER_POLICY ].get_object();
            size_t bits_per_key;

            // Bits per key is required for the bloom filter policy
            FC_ASSERT( filter_policy.contains( BITS_PER_KEY ), "Expected '${parent}' to contain '${key}'",
               ("parent", BLOOM_FILTER_POLICY)
               ("key", BITS_PER_KEY) );

            bits_per_key = filter_policy[ BITS_PER_KEY ].template as< uint64_t >();

            if ( filter_policy.contains( USE_BLOCK_BASED_BUILDER ) )
               table_options.filter_policy.reset( rocksdb::NewBloomFilterPolicy( bits_per_key, filter_policy[ USE_BLOCK_BASED_BUILDER ].template as< bool >() ) );
            else
               table_options.filter_policy.reset( rocksdb::NewBloomFilterPolicy( bits_per_key ) );
         }

         o.table_factory.reset( ::rocksdb::NewBlockBasedTableFactory( table_options ) );
      }
   }
};

fc::variant_object configuration::apply_configuration_overlay( const fc::variant& base, const fc::variant& overlay )
{
   fc::mutable_variant_object config;
   FC_ASSERT( base.is_object(), "Expected '${key}' configuration to be an object",
      ("key", BASE) );

   FC_ASSERT( overlay.is_object(), "Expected database overlay configuration to be an object" );

   // Start with our base configuration
   config = base.get_object();

   // Iterate through the overlay overriding the base values
   auto& overlay_obj = overlay.get_object();
   for ( auto it = overlay_obj.begin(); it != overlay_obj.end(); ++it )
      config[ it->key() ] = it->value();

   return config;
}

fc::variant_object configuration::retrieve_global_configuration( const fc::variant_object& obj )
{
   fc::mutable_variant_object global_config;

   FC_ASSERT( obj[ GLOBAL ].is_object(), "Expected '${key}' configuration to be an object",
      ("key", GLOBAL) );

   global_config = obj[ GLOBAL ].get_object();
   return global_config;
}

fc::variant_object configuration::retrieve_active_configuration( const fc::variant_object& obj, std::string type_name )
{
   fc::mutable_variant_object active_config;
   std::vector< std::string > split_v;
   boost::split( split_v, type_name, boost::is_any_of( ":" ) );
   const auto index_name = *(split_v.rbegin());

   FC_ASSERT( obj[ BASE ].is_object(), "Expected '${key}' configuration to be an object",
      ("key", BASE) );

   // We look to apply an index configuration overlay
   if ( obj.find( index_name ) != obj.end() )
      active_config = apply_configuration_overlay( obj[ BASE ], obj[ index_name ] );
   else
      active_config = obj[ BASE ].get_object();

   return active_config;
}

size_t configuration::get_object_count( const boost::any& cfg )
{
   size_t object_count = 0;

   auto c = boost::any_cast< fc::variant >( cfg );
   FC_ASSERT( c.is_object(), "Expected database configuration to be an object" );
   auto& obj = c.get_object();

   fc::variant_object global_config = retrieve_global_configuration( obj );

   FC_ASSERT( global_config.contains( OBJECT_COUNT ), "Expected '${parent}' configuration to contain '${key}'",
      ("parent", GLOBAL)
      ("key", OBJECT_COUNT) );

   FC_ASSERT( global_config[ OBJECT_COUNT ].is_uint64(), "Expected '${key}' to be an unsigned integer",
      ("key", OBJECT_COUNT) );

   object_count = global_config[ OBJECT_COUNT ].as< uint64_t >();

   return object_count;
}

bool configuration::gather_statistics( const boost::any& cfg )
{
   bool statistics = false;

   auto c = boost::any_cast< fc::variant >( cfg );
   FC_ASSERT( c.is_object(), "Expected database configuration to be an object" );
   auto& obj = c.get_object();

   fc::variant_object global_config = retrieve_global_configuration( obj );

   FC_ASSERT( global_config.contains( STATISTICS ), "Expected '${parent}' configuration to contain '${key}'",
      ("parent", GLOBAL)
      ("key", STATISTICS) );

   FC_ASSERT( global_config[ STATISTICS ].is_bool(), "Expected '${key}' to be a boolean value",
      ("key", STATISTICS) );

   statistics = global_config[ STATISTICS ].as< bool >();

   return statistics;
}

::rocksdb::Options configuration::get_options( const boost::any& cfg, std::string type_name )
{
   ::rocksdb::Options opts;

   auto c = boost::any_cast< fc::variant >( cfg );
   FC_ASSERT( c.is_object(), "Expected database configuration to be an object" );
   auto& obj = c.get_object();

   if ( global_shared_cache == nullptr )
   {
      size_t capacity = 0;
      int num_shard_bits = 0;

      fc::variant_object global_config = retrieve_global_configuration( obj );

      FC_ASSERT( global_config.contains( SHARED_CACHE ), "Expected '${parent}' configuration to contain '${key}'",
         ("parent", GLOBAL)
         ("key", SHARED_CACHE) );

      FC_ASSERT( global_config[ SHARED_CACHE ].is_object(), "Expected '${key}' to be an object",
         ("key", SHARED_CACHE) );

      auto& shared_cache_obj = global_config[ SHARED_CACHE ].get_object();

      if ( shared_cache_obj.contains( CAPACITY ) && shared_cache_obj[ CAPACITY ].is_uint64() )
         capacity = shared_cache_obj[ CAPACITY ].as< uint64_t >();
      else
         capacity = DEFAULT_MIRA_SHARED_CACHE_SIZE;

      if ( shared_cache_obj.contains( NUM_SHARD_BITS ) && shared_cache_obj[ NUM_SHARD_BITS ].is_uint64() )
         num_shard_bits = shared_cache_obj[ NUM_SHARD_BITS ].as< int >();
      else
         num_shard_bits = DEFAULT_MIRA_NUM_SHARD_BITS;

      global_shared_cache = rocksdb::NewLRUCache( capacity, num_shard_bits );
   }

   fc::variant_object config = retrieve_active_configuration( obj, type_name );

   for ( auto it = config.begin(); it != config.end(); ++it )
   {
      try
      {
         if ( global_database_option_map.find( it->key() ) != global_database_option_map.end() )
            global_database_option_map[ it->key() ]( opts, it->value() );
         else
            wlog( "Encountered an unknown database configuration option: ${key}", ("key",it->key()) );
      }
      catch( ... )
      {
         elog( "Error applying database option: ${key}", ("key", it->key()) );
         throw;
      }
   }

   return opts;
}

} // mira
