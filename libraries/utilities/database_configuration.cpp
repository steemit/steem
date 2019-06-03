#include <fc/io/json.hpp>
#include <fc/variant.hpp>
#include <fc/reflect/variant.hpp>
#include <steem/utilities/database_configuration.hpp>

#define KB(x)   ((size_t) (x) << 10)
#define MB(x)   ((size_t) (x) << 20)
#define GB(x)   ((size_t) (x) << 30)

namespace steem { namespace utilities {

namespace database { namespace configuration {

struct shared_cache {
   std::string capacity;
};

struct write_buffer_manager {
   std::string write_buffer_size;
};

struct global {
   database::configuration::shared_cache shared_cache;
   database::configuration::write_buffer_manager write_buffer_manager;
   uint64_t object_count;
   bool statistics;
};

struct bloom_filter_policy {
   uint64_t bits_per_key;
   bool use_block_based_builder;
};

struct block_based_table_options {
   uint64_t block_size;
   bool cache_index_and_filter_blocks;
   database::configuration::bloom_filter_policy bloom_filter_policy;
};

struct base_index {
   bool optimize_level_style_compaction;
   bool increase_parallelism;
   database::configuration::block_based_table_options block_based_table_options;
};

struct configuration {
   database::configuration::global global;
   database::configuration::base_index base;
};

} } // database::configuration

fc::variant default_database_configuration()
{
   database::configuration::configuration config;

   // global
   config.global.object_count = 62500; // 4GB heaviest usage
   config.global.statistics = false;   // Incurs severe performance degradation when true

   // global::shared_cache
   config.global.shared_cache.capacity = std::to_string( GB(5) );

   // global::write_buffer_manager
   config.global.write_buffer_manager.write_buffer_size = std::to_string( GB(1) ); // Write buffer manager is within the shared cache

   // base
   config.base.optimize_level_style_compaction = true;
   config.base.increase_parallelism = true;

   // base::block_based_table_options
   config.base.block_based_table_options.block_size = KB(8);
   config.base.block_based_table_options.cache_index_and_filter_blocks = true;

   // base::block_based_table_options::bloom_filter_policy
   config.base.block_based_table_options.bloom_filter_policy.bits_per_key = 10;
   config.base.block_based_table_options.bloom_filter_policy.use_block_based_builder = false;

   fc::variant config_obj;
   fc::to_variant( config, config_obj );
   return config_obj;
}

} } // steem::utilities


FC_REFLECT( steem::utilities::database::configuration::shared_cache,
   (capacity)
);

FC_REFLECT( steem::utilities::database::configuration::write_buffer_manager,
   (write_buffer_size)
);

FC_REFLECT( steem::utilities::database::configuration::global,
   (shared_cache)
   (write_buffer_manager)
   (object_count)
   (statistics)
);

FC_REFLECT( steem::utilities::database::configuration::bloom_filter_policy,
   (bits_per_key)
   (use_block_based_builder)
);

FC_REFLECT( steem::utilities::database::configuration::block_based_table_options,
   (block_size)
   (cache_index_and_filter_blocks)
   (bloom_filter_policy)
);

FC_REFLECT( steem::utilities::database::configuration::base_index,
   (optimize_level_style_compaction)
   (increase_parallelism)
   (block_based_table_options)
);

FC_REFLECT( steem::utilities::database::configuration::configuration,
   (global)
   (base)
);
