#include <steem/plugins/rocksdb/rocksdb_plugin.hpp>

#include <steem/chain/database.hpp>
#include <steem/plugins/chain/chain_plugin.hpp>

#include <steem/utilities/benchmark_dumper.hpp>

namespace bpo = boost::program_options;

namespace steem { namespace plugins { namespace rocksdb {

using steem::protocol::block_id_type;
using steem::protocol::operation;
using steem::protocol::signed_block;
using steem::protocol::signed_transaction;

using steem::utilities::benchmark_dumper;

class rocksdb_plugin::impl
{
public:
   impl() : _mainDb(appbase::app().get_plugin<steem::plugins::chain::chain_plugin>().db()) {}
   
   void openDb(const std::string& path)
   {
   }

   void importData()
   {
      ilog("Starting data import...");

      block_id_type lastBlock;
      size_t blockNo = 0;
      const signed_transaction* lastTx = nullptr;
      size_t txNo = 0;
      size_t totalOps = 0;

      benchmark_dumper dumper;
      dumper.initialize([](benchmark_dumper::database_object_sizeof_cntr_t&){}, "rocksdb_data_import.json");

      _mainDb.foreach_operation([&](const signed_block& block, const signed_transaction& tx, const operation& op) -> bool
      {
         if(lastBlock != block.previous)
         {
            blockNo = block.block_num();
            lastBlock = block.previous;
         }
         
         if(lastTx != &tx)
         {
            ++txNo;
            lastTx = &tx;
         }

         ++totalOps;

         return true;
      }
      );

      const auto& measure = dumper.measure(blockNo, [](benchmark_dumper::index_memory_details_cntr_t&, bool) {});
      ilog( "RocksDb data import - Performance report at block ${n}. Elapsed time: ${rt} ms (real), ${ct} ms (cpu). Memory usage: ${cm} (current), ${pm} (peak) kilobytes.",
         ("n", blockNo)
         ("rt", measure.real_ms)
         ("ct", measure.cpu_ms)
         ("cm", measure.current_mem)
         ("pm", measure.peak_mem) );

      ilog( "RocksDb data import finished. Processed blocks: ${n}, containing: ${tx} transactions and ${op} operations.",
         ("n", blockNo)
         ("tx", txNo)
         ("op", totalOps));
   }

private:
   const chain::database& _mainDb;
};

rocksdb_plugin::rocksdb_plugin()
{
}

rocksdb_plugin::~rocksdb_plugin()
{

}

void rocksdb_plugin::set_program_options(
   boost::program_options::options_description &command_line_options,
   boost::program_options::options_description &config_file_options)
{
   command_line_options.add_options()
      ("rocksdb-path", bpo::value<string>(), "Allows to specify path where rocksdb store will be located.")
   ;
}

void rocksdb_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   if(options.count("rocksdb-path"))
      _dbPath = options.at("rocksdb-path").as<string>();
}

void rocksdb_plugin::plugin_startup()
{
   ilog("Starting up rocksdb_plugin...");

   my = std::make_unique<impl>();
   my->importData();
}

void rocksdb_plugin::plugin_shutdown()
{
   ilog("Shutting down rocksdb_plugin...");
}

} } }
