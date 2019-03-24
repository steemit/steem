#include <fc/io/json.hpp>
#include <fc/variant.hpp>
#include <fc/reflect/variant.hpp>
#include <steem/utilities/indices_cfg.hpp>

namespace steem { namespace utilities {

namespace indices { namespace configuration {

struct shared_cache {
   uint64_t capacity;
   int num_shared_bits;
};

struct global {
   indices::configuration::shared_cache shared_cache;
   uint64_t object_count;
   bool statistics;
};

struct bloom_filter_policy {
   uint64_t bits_per_key;
   bool use_block_based_builder;
};

struct block_based_table_options {
   uint64_t block_size;
   indices::configuration::bloom_filter_policy bloom_filter_policy;
};

struct base_index {
   bool allow_mmap_reads;
   uint64_t write_buffer_size;
   uint64_t max_bytes_for_level_base;
   uint64_t target_file_size_base;
   int max_write_buffer_number;
   int max_background_compactions;
   int max_background_flushes;
   bool optimize_level_style_compaction;
   bool increase_parallelism;
   indices::configuration::block_based_table_options block_based_table_options;
};

struct configuration {
   indices::configuration::global global;
   indices::configuration::base_index base;
};

} } // indices::configuration

fc::variant get_default_indices_cfg()
{
   indices::configuration::configuration config;

   // global
   config.global.object_count = 40000000;
   config.global.statistics = false;

   // global::shared_cache
   config.global.shared_cache.capacity = 1073741824;
   config.global.shared_cache.num_shared_bits = 4;

   // base
   config.base.allow_mmap_reads = true;
   config.base.write_buffer_size = 2097152;
   config.base.max_bytes_for_level_base = 5242880;
   config.base.target_file_size_base = 102400;
   config.base.max_write_buffer_number = 16;
   config.base.max_background_compactions = 16;
   config.base.max_background_flushes = 16;
   config.base.optimize_level_style_compaction = true;
   config.base.increase_parallelism = true;

   // base::block_based_table_options
   config.base.block_based_table_options.block_size = 8192;

   // base::block_based_table_options::bloom_filter_policy
   config.base.block_based_table_options.bloom_filter_policy.bits_per_key = 14;
   config.base.block_based_table_options.bloom_filter_policy.use_block_based_builder = false;

   fc::variant config_obj;
   fc::to_variant( config, config_obj );
   return config_obj;
}

} } // steem::utilities


FC_REFLECT( steem::utilities::indices::configuration::shared_cache,
   (capacity)
   (num_shared_bits)
);

FC_REFLECT( steem::utilities::indices::configuration::global,
   (shared_cache)
   (object_count)
   (statistics)
);

FC_REFLECT( steem::utilities::indices::configuration::bloom_filter_policy,
   (bits_per_key)
   (use_block_based_builder)
);

FC_REFLECT( steem::utilities::indices::configuration::block_based_table_options,
   (block_size)
   (bloom_filter_policy)
);

FC_REFLECT( steem::utilities::indices::configuration::base_index,
   (allow_mmap_reads)
   (write_buffer_size)
   (max_bytes_for_level_base)
   (target_file_size_base)
   (max_write_buffer_number)
   (max_background_compactions)
   (max_background_flushes)
   (optimize_level_style_compaction)
   (increase_parallelism)
   (block_based_table_options)
);

FC_REFLECT( steem::utilities::indices::configuration::configuration,
   (global)
   (base)
);
