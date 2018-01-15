#include <steem/plugins/rocksdb/rocksdb_plugin.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/history_object.hpp>

#include <steem/plugins/chain/chain_plugin.hpp>

#include <steem/utilities/benchmark_dumper.hpp>

#include <appbase/application.hpp>

#include <string>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"

namespace bpo = boost::program_options;

namespace steem { namespace plugins { namespace rocksdb {

using steem::protocol::block_id_type;
using steem::protocol::operation;
using steem::protocol::signed_block;
using steem::protocol::signed_transaction;

using steem::utilities::benchmark_dumper;

using ::rocksdb::DB;
using ::rocksdb::Options;
using ::rocksdb::Slice;
using ::rocksdb::Comparator;
using ::rocksdb::ColumnFamilyDescriptor;
using ::rocksdb::ColumnFamilyOptions;
using ::rocksdb::ColumnFamilyHandle;

namespace 
{

/** Helper class to simplify construction of Slice objects holding primitive type values.
 * 
 */
template <typename T>
class PrimitiveTypeSlice final : public Slice
{
public:
   explicit PrimitiveTypeSlice(T value) : _value(value)
   {
      data_ = reinterpret_cast<const char*>(&_value);
      size_ = sizeof(T);
   }
private:
   T _value;
};

template <typename T>
class PrimitiveTypeComparatorImpl final : public Comparator
{
public:
   virtual const char* Name() const override
   {
      static const std::string name = boost::core::demangle(typeid(this).name());
      return name.c_str();
   }

  virtual int Compare(const Slice& a, const Slice& b) const override
  {
      auto id1 = retrieveKey(a);
      auto id2 = retrieveKey(b);

      if(id1 < id2)
         return -1;

      if(id1 > id2)
         return 1;

      return 0; 
  }

  virtual bool Equal(const Slice& a, const Slice& b) const override
  {
      auto id1 = retrieveKey(a);
      auto id2 = retrieveKey(b);
      return id1 == id2;
  }

  virtual void FindShortestSeparator(std::string* start, const Slice& limit) const override
  {
     /// Nothing to do.
  }

  virtual void FindShortSuccessor(std::string* key) const override
  {
     /// Nothing to do.
  }

private:
   const T& retrieveKey(const Slice& slice) const
   {
      assert(sizeof(T) == slice.size());
      const char* rawData = slice.data();
      return *reinterpret_cast<const T*>(rawData);
   }
};

typedef PrimitiveTypeComparatorImpl<size_t> by_id_ComparatorImpl;
/** Location index is nonunique. Since RocksDB supports only unique indexes, it must be extended 
 *  by some unique part (ie ID).
 * 
 */
typedef std::pair<uint32_t, size_t> location_id_pair;
typedef PrimitiveTypeComparatorImpl<location_id_pair> by_location_ComparatorImpl;

Comparator* by_id_Comparator()
{
   static by_id_ComparatorImpl c;
   return &c; 
}

Comparator* by_location_Comparator()
{
   static by_location_ComparatorImpl c;
   return &c;
}

} /// anonymous

class rocksdb_plugin::impl final
{
public:

   impl() : _mainDb(appbase::app().get_plugin<steem::plugins::chain::chain_plugin>().db()) {}
   ~impl()
   {
      shutdownDb();
   }

   void openDb(const bfs::path& path)
   {
      createDbSchema(path);

      auto columnDefs = prepareColumnDefinitions(true);

      DB* storageDb = nullptr;
      auto strPath = path.string();
      Options options;
      /// Optimize RocksDB. This is the easiest way to get RocksDB to perform well
      options.IncreaseParallelism();
      options.OptimizeLevelStyleCompaction();
      
      auto status = DB::Open(options, strPath, columnDefs, &_columnHandles, &storageDb);

      if(status.ok())
      {
         ilog("RocksDB opened successfully");

         _storage.reset(storageDb);
         importData();
      }
      else
      {
         elog("RocksDB cannot open database at location: `${p}'.\nReturned error: ${e}",
            ("p", strPath)("e", status.ToString()));
      }
   }

   void shutdownDb()
   {
      cleanupColumnHandles();
      _storage.reset();
   }

private:
   typedef std::vector<ColumnFamilyDescriptor> ColumnDefinitions;
   ColumnDefinitions prepareColumnDefinitions(bool addDefaultColumn);

   void createDbSchema(const bfs::path& path);

   void cleanupColumnHandles()
   {
      for(auto* h : _columnHandles)
         delete h;
      _columnHandles.clear();
   }

   void importOperation(const signed_block& block, const signed_transaction& tx, uint32_t txInBlock,
      const operation& op, uint16_t opInTx, size_t opSeqId);

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

      _mainDb.foreach_operation([&blockNo, &txNo, &lastBlock, &lastTx, &totalOps, this](const signed_block& block,
         const signed_transaction& tx, uint32_t txInBlock, const operation& op, uint16_t opInTx) -> bool
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

         importOperation(block, tx, txInBlock, op, opInTx, totalOps);

         ++totalOps;

         if(blockNo % 10000 == 0)
         {
            ilog( "RocksDb data import processed blocks: ${n}, containing: ${tx} transactions and ${op} operations.",
               ("n", blockNo)
               ("tx", txNo)
               ("op", totalOps));
         }

         return true;
      }
      );

      if(this->_collectedOps != 0)
         flushWriteBuffer();

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

   void flushWriteBuffer()
   {
      auto s = _storage->Write(::rocksdb::WriteOptions(), &_writeBuffer);
      FC_ASSERT(s.ok(), "Data write failed");
      _writeBuffer.Clear();
      _collectedOps = 0;
   }

   enum
   {
      WRITE_BUFFER_FLUSH_LIMIT = 100
   };

private:
   const chain::database&           _mainDb;
   std::unique_ptr<DB>              _storage;
   std::vector<ColumnFamilyHandle*> _columnHandles;
   ::rocksdb::WriteBatch            _writeBuffer;
   unsigned int                     _collectedOps = 0;
};


rocksdb_plugin::impl::ColumnDefinitions rocksdb_plugin::impl::prepareColumnDefinitions(bool addDefaultColumn)
{
   ColumnDefinitions columnDefs;
   if(addDefaultColumn)
      columnDefs.emplace_back(::rocksdb::kDefaultColumnFamilyName, ColumnFamilyOptions());

   columnDefs.emplace_back("operation_by_id", ColumnFamilyOptions());
   auto& byIdColumn = columnDefs.back();
   byIdColumn.options.comparator = by_id_Comparator();

   columnDefs.emplace_back("operation_by_location", ColumnFamilyOptions());
   auto& byLocationColumn = columnDefs.back();
   byLocationColumn.options.comparator = by_location_Comparator();

   return columnDefs;
}

void rocksdb_plugin::impl::createDbSchema(const bfs::path& path)
{
   DB* db = nullptr;

   auto columnDefs = prepareColumnDefinitions(true);
   auto strPath = path.string();
   Options options;
   /// Optimize RocksDB. This is the easiest way to get RocksDB to perform well
   options.IncreaseParallelism();
   options.OptimizeLevelStyleCompaction();
   auto s = DB::OpenForReadOnly(options, strPath, columnDefs, &_columnHandles, &db);

   if(s.ok())
   {
      cleanupColumnHandles();
      delete db;
      return;
   }

   options.create_if_missing = true;
   
   s = DB::Open(options, strPath, &db);
   if(s.ok())
   {
      columnDefs = prepareColumnDefinitions(false);
      s = db->CreateColumnFamilies(columnDefs, &_columnHandles);
      if(s.ok())
      {
         ilog("RockDB column definitions created successfully.");
         cleanupColumnHandles();
      }
      else
      {
         elog("RocksDB can not create column definitions at location: `${p}'.\nReturned error: ${e}",
            ("p", strPath)("e", s.ToString()));
      }

      delete db;
   }
   else
   {
      elog("RocksDB can not create storage at location: `${p}'.\nReturned error: ${e}",
         ("p", strPath)("e", s.ToString()));
   }
}

void rocksdb_plugin::impl::importOperation(const signed_block& block, const signed_transaction& tx,
   uint32_t txInBlock, const operation& op, uint16_t opInTx, size_t opSeqNo)
{
   std::allocator<steem::chain::operation_object> a;
   steem::chain::operation_object obj([](steem::chain::operation_object&){}, a);
   obj.id._id       = opSeqNo;
   obj.trx_id       = tx.id();
   obj.block        = block.block_num();
   obj.trx_in_block = txInBlock;
   obj.timestamp    = _mainDb.head_block_time();
   auto size = fc::raw::pack_size(op);
   obj.serialized_op.resize(size);
   {
      fc::datastream<char*> ds(obj.serialized_op.data(), size);
      fc::raw::pack(ds, op);
   }

   steem::chain::buffer_type serializedObj;
   size = fc::raw::pack_size(obj);
   serializedObj.resize(size);
   {
      fc::datastream<char*> ds(serializedObj.data(), size);
      fc::raw::pack(ds, obj);
   }

   PrimitiveTypeSlice<size_t> idSlice(obj.id._id);
   PrimitiveTypeSlice<location_id_pair> blockNoIdSlice(location_id_pair(obj.block, obj.id._id));
   auto s = _writeBuffer.Put(_columnHandles[1], idSlice, Slice(serializedObj.data(), serializedObj.size()));
   FC_ASSERT(s.ok(), "Data write failed");
   s = _writeBuffer.Put(_columnHandles[2], blockNoIdSlice, idSlice);
   FC_ASSERT(s.ok(), "Data write failed");
   if(++this->_collectedOps >= WRITE_BUFFER_FLUSH_LIMIT)
      flushWriteBuffer();
}

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
      ("rocksdb-path", bpo::value<bfs::path>()->default_value("rocksdb_storage"),
         "Allows to specify path where rocksdb store will be located.")
   ;
}

void rocksdb_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   if(options.count("rocksdb-path"))
      _dbPath = options.at("rocksdb-path").as<bfs::path>();
}

void rocksdb_plugin::plugin_startup()
{
   ilog("Starting up rocksdb_plugin...");

   _my = std::make_unique<impl>();
   if(_dbPath.is_absolute())
   {
      _my->openDb(_dbPath);
   }
   else
   {
      auto actualPath = appbase::app().data_dir() / _dbPath;
      _my->openDb(actualPath);
   }
}

void rocksdb_plugin::plugin_shutdown()
{
   ilog("Shutting down rocksdb_plugin...");
   _my->shutdownDb();
}

} } }
