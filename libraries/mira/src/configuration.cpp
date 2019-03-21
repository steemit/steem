#include <mira/configuration.hpp>

#define DEFAULT_MIRA_SHARED_CACHE_SIZE (1ull * 1024 * 1024 * 1024 )
#define DEFAULT_MIRA_NUM_SHARD_BITS    (4)

namespace mira {

// Base configuration for an index
#define DEFAULT                          "default"

// Global options
#define GLOBAL                           "global"
#define SHARED_CACHE                     "shared_cache"
#define OBJECT_COUNT                     "object_count"
#define STATISTICS                       "statistics"

// Shared cache options
#define CAPACITY                         "capacity"
#define NUM_SHARD_BITS                   "num_shard_bits"

// RocksDB options
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

static std::shared_ptr< rocksdb::Cache > GLOBAL_SHARED_CACHE;

static std::map< std::string, std::function< void( ::rocksdb::Options&, fc::variant ) > > INDEX_OPTION_MAP {
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
         FC_ASSERT( v.is_object() );
         auto& obj = v.get_object();

//         table_options.block_cache = rocksdb_options_factory::get_shared_cache();
         table_options.block_cache = GLOBAL_SHARED_CACHE;

         if ( obj.contains( BLOCK_SIZE ) )
            table_options.block_size = obj[ BLOCK_SIZE ].template as< uint64_t >();

         if ( obj.contains( BLOOM_FILTER_POLICY ) )
         {
            FC_ASSERT( obj[ BLOOM_FILTER_POLICY ].is_object() );
            auto filter_policy = obj[ BLOOM_FILTER_POLICY ].get_object();
            size_t bits_per_key;

            // Bits per key is required for the bloom filter policy
            FC_ASSERT( filter_policy.contains( BITS_PER_KEY ) );

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
   FC_ASSERT( base.is_object() );
   FC_ASSERT( overlay.is_object() );

   // Start with our base configuration
   config = base.get_object();

   // Iterate through the overlay overriding the base values
   auto& overlay_obj = overlay.get_object();
   for ( auto it = overlay_obj.begin(); it != overlay_obj.end(); ++it )
      config[ it->key() ] = it->value();

   return config;
}

std::string configuration::default_configuration()
{
   return
R"({
// Global options apply to all indices
   "global" : {
      // This cache is internal to RocksDB and is not related
      // to MIRAs cache, which is determined by "object_count"
      "shared_cache" : {
         "capacity" : 1073741824,
         "num_shard_bits" : 4
      },

      // Indicates the number of objects MIRA maintains in cache
      "object_count" : 40000000,

      // WARNING: Enabling statistics severely degrades performance

      // With statistics set to true, you can use the script
      // programs/util/rocksdb_advisor.sh to analyze and adjust
      // performance options.
      "statistics" : false
   },

// The default configuration is the base configuration for all
// MIRA indices. An overlay can be placed on top of the default
// configuration in order to change a specific option for a
// specific index
   "default" : {
      "allow_mmap_reads"                 : true,
      "write_buffer_size"                : 2097152,
      "max_bytes_for_level_base"         : 5242880,
      "target_file_size_base"            : 102400,
      "max_write_buffer_number"          : 16,
      "max_background_compactions"       : 16,
      "max_background_flushes"           : 16,
      "optimize_level_style_compaction"  : true,
      "increase_parallelism"             : true,
      "min_write_buffer_number_to_merge" : 8
      "block_based_table_options" : {
         "block_size" : 8192,
         "bloom_filter_policy" : {
            "bits_per_key": 14,
            "use_block_based_builder": false
         }
      }
   },

// An example index configuration overlay. Override a specific
// value on a particular index

// NOTE: When overriding a top level value, the subsequent nested
// options must be defined
   "key_lookup_object" : {
      "allow_mmap_reads" : false,
      "block_based_table_options" : {
         "block_size" : 8192,
         "bloom_filter_policy" : {
            "bits_per_key": 10,
            "use_block_based_builder": false
         }
      }
   }
})";
}

fc::variant_object configuration::retrieve_global_configuration( const fc::variant_object& obj )
{
   fc::mutable_variant_object global_config;
   FC_ASSERT( obj[ GLOBAL ].is_object() );
   global_config = obj[ GLOBAL ].get_object();
   return global_config;
}

fc::variant_object configuration::retrieve_active_configuration( const fc::variant_object& obj, std::string type_name )
{
   fc::mutable_variant_object active_config;
   std::vector< std::string > split_v;
   boost::split( split_v, type_name, boost::is_any_of( ":" ) );
   const auto index_name = *(split_v.rbegin());

   ilog( "Retrieving active config for ${t}", ("t", index_name) );

   FC_ASSERT( obj[ DEFAULT ].is_object() );

   // We look to apply an index configuration overlay
   if ( obj.find( index_name ) != obj.end() )
      active_config = apply_configuration_overlay( obj[ DEFAULT ], obj[ index_name ] );
   else
      active_config = obj[ DEFAULT ].get_object();

   return active_config;
}

size_t configuration::get_object_count( const boost::any& cfg )
{
   size_t object_count = 0;
   try
   {
      auto c = boost::any_cast< fc::variant >( cfg );
      FC_ASSERT( c.is_object() );
      auto& obj = c.get_object();

      fc::variant_object global_config = retrieve_global_configuration( obj );

      FC_ASSERT( global_config.contains( OBJECT_COUNT ) );
      FC_ASSERT( global_config[ OBJECT_COUNT ].is_uint64() );

      object_count = global_config[ OBJECT_COUNT ].as< uint64_t >();
   }
   catch ( const std::exception& e )
   {
      elog( "Error parsing global configuration : ${e}", ("e", e.what()) );
      throw;
   }

   return object_count;
}

bool configuration::gather_statistics( const boost::any& cfg )
{
   bool statistics = false;
   try
   {
      auto c = boost::any_cast< fc::variant >( cfg );
      FC_ASSERT( c.is_object() );
      auto& obj = c.get_object();

      fc::variant_object global_config = retrieve_global_configuration( obj );

      FC_ASSERT( global_config.contains( STATISTICS ) );
      FC_ASSERT( global_config[ STATISTICS ].is_bool() );

      statistics = global_config[ STATISTICS ].as< bool >();
   }
   catch ( const std::exception& e )
   {
      elog( "Error parsing global configuration : ${e}", ("e", e.what()) );
      throw;
   }

   return statistics;
}

::rocksdb::Options configuration::get_options( const boost::any& cfg, std::string type_name )
{
   ::rocksdb::Options opts;

   try
   {
      auto c = boost::any_cast< fc::variant >( cfg );
      FC_ASSERT( c.is_object() );
      auto& obj = c.get_object();

      if ( GLOBAL_SHARED_CACHE == nullptr )
      {
         size_t capacity = 0;
         int num_shard_bits = 0;

         fc::variant_object global_config = retrieve_global_configuration( obj );

         FC_ASSERT( global_config.contains( SHARED_CACHE ) );
         FC_ASSERT( global_config[ SHARED_CACHE ].is_object() );
         auto& shared_cache_obj = global_config[ SHARED_CACHE ].get_object();

         if ( shared_cache_obj.contains( CAPACITY ) && shared_cache_obj[ CAPACITY ].is_uint64() )
            capacity = shared_cache_obj[ CAPACITY ].as< uint64_t >();
         else
            capacity = DEFAULT_MIRA_SHARED_CACHE_SIZE;

         if ( shared_cache_obj.contains( NUM_SHARD_BITS ) && shared_cache_obj[ NUM_SHARD_BITS ].is_uint64() )
            num_shard_bits = shared_cache_obj[ NUM_SHARD_BITS ].as< int >();
         else
            num_shard_bits = DEFAULT_MIRA_NUM_SHARD_BITS;

         GLOBAL_SHARED_CACHE = rocksdb::NewLRUCache( capacity, num_shard_bits );
      }

      fc::variant_object config = retrieve_active_configuration( obj, type_name );

      for ( auto it = config.begin(); it != config.end(); ++it )
      {
         try
         {
            if ( INDEX_OPTION_MAP.find( it->key() ) != INDEX_OPTION_MAP.end() )
            {
               ilog( "${t} -> option ${k} : ${v}", ("t", type_name)("k", it->key())("v", it->value()) );
               INDEX_OPTION_MAP[ it->key() ]( opts, it->value() );
            }
            else
            {
               wlog( "Encountered an unknown option: ${k}", ("k",it->key()) );
            }
         }
         catch( const std::exception& e )
         {
            elog( "Error applying option: ${k}, ${v}", ("k", it->key())("v", it->value()) );
            throw;
         }
      }

   }
   catch ( const std::exception& e )
   {
      elog( "Error parsing configuration for type '${t}': ${e}", ("t", type_name)("e", e.what()) );
      throw;
   }

   return opts;
}

} // mira
