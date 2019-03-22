#include <fc/io/json.hpp>
#include <steem/utilities/indices_cfg.hpp>

namespace steem { namespace utilities {

fc::variant get_default_indices_cfg()
{
   std::string default_cfg = \
R"({
   "global" : {
      "shared_cache" : {
         "capacity" : 1073741824,
         "num_shard_bits" : 4
      },

      "object_count" : 40000000,

      "statistics" : false
   },

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
   auto v = fc::json::from_string( default_cfg, fc::json::parse_type::strict_parser );
   return v;
}

} } // steem::utilities
