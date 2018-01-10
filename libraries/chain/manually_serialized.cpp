#include <steem/chain/database.hpp>

#include <serialize3/h/storage/serializedumper.h>
#include <serialize3/h/storage/serializeloader.h>

namespace steem { namespace chain
{

void database::Dump(ASerializeDumper& dumper) const
{
   chain::database::Dump(dumper);

   for (int i = 0; i < (sizeof(_hardfork_times) / sizeof(fc::time_point_sec)); ++i)
      dumper & _hardfork_times[i];
   for (int i = 0; i < (sizeof(_hardfork_versions) / sizeof(protocol::hardfork_version)); ++i)
      dumper & _hardfork_times[i];

   dumper & _block_log_dir.generic_string();
   dumper & _current_trx_id;
   dumper & _current_block_num;
   dumper & _current_trx_in_block;
   dumper & _current_op_in_trx;
   dumper & _current_virtual_op;
   dumper & _checkpoints;
   dumper & _node_property_object;
   dumper & _flush_blocks;
   dumper & _next_flush_block;
   dumper & _last_free_gb_printed;
   dumper & _next_available_nai;

   dumper & _json_schema;
}

void database::Load(ASerializeLoader& loader)
{
   chain::database::Load(loader);

   for (int i = 0; i < (sizeof(_hardfork_times) / sizeof(fc::time_point_sec)); ++i)
      loader & _hardfork_times[i];
   for (int i = 0; i < (sizeof(_hardfork_versions) / sizeof(protocol::hardfork_version)); ++i)
      loader & _hardfork_times[i];

   std::string block_log_dir;
   loader & block_log_dir;
   _block_log_dir = block_log_dir;
   _block_log.open(_block_log_dir);

   loader & _current_trx_id;
   loader & _current_block_num;
   loader & _current_trx_in_block;
   loader & _current_op_in_trx;
   loader & _current_virtual_op;
   loader & _checkpoints;
   loader & _node_property_object;
   loader & _flush_blocks;
   loader & _next_flush_block;
   loader & _last_free_gb_printed;
   loader & _next_available_nai;

   loader & _json_schema;

   initialize_evaluators();
}

} } // namespace steem::chain
