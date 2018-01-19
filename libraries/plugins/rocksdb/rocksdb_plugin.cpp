#include <steem/plugins/rocksdb/rocksdb_plugin.hpp>

#include "rocksdb_api.hpp"

#include <steem/chain/database.hpp>
#include <steem/chain/history_object.hpp>
#include <steem/chain/util/impacted.hpp>

#include <steem/plugins/chain/chain_plugin.hpp>

#include <steem/utilities/benchmark_dumper.hpp>

#include <appbase/application.hpp>

#include <string>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"

namespace bpo = boost::program_options;

namespace steem { namespace plugins { namespace rocksdb {

using steem::protocol::account_name_type;
using steem::protocol::block_id_type;
using steem::protocol::operation;
using steem::protocol::signed_block;
using steem::protocol::signed_transaction;

using steem::chain::account_history_object;
using steem::chain::operation_object;
using steem::chain::operation_notification;
using steem::chain::transaction_id_type;

using steem::utilities::benchmark_dumper;

using ::rocksdb::DB;
using ::rocksdb::Options;
using ::rocksdb::ReadOptions;
using ::rocksdb::Slice;
using ::rocksdb::Comparator;
using ::rocksdb::ColumnFamilyDescriptor;
using ::rocksdb::ColumnFamilyOptions;
using ::rocksdb::ColumnFamilyHandle;

namespace 
{

   template <class T>
   steem::chain::buffer_type dump(const T& obj)
   {
      steem::chain::buffer_type serializedObj;
      auto size = fc::raw::pack_size(obj);
      serializedObj.resize(size);
      fc::datastream<char*> ds(serializedObj.data(), size);
      fc::raw::pack(ds, obj);
      return serializedObj;
   }

   template <class T>
   void load(T& obj, const char* data, size_t size)
   {
      fc::datastream<const char*> ds(data, size);
      fc::raw::unpack(ds, obj);
   }

   template <class T>
   void load(T& obj, const steem::chain::buffer_type& source)
   {
      load(obj, source.data(), source.size());
   }

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

   static const T& unpackSlice(const Slice& s)
   {
      assert(sizeof(T) == s.size());
      return *reinterpret_cast<const T*>(s.data());
   }

private:
   T _value;
};

/** Helper base class to cover all common functionality across defined comparators.
 * 
 */
class AComparator : public Comparator
{
public:
   virtual const char* Name() const override final
   {
      static const std::string name = boost::core::demangle(typeid(this).name());
      return name.c_str();
   }

  virtual void FindShortestSeparator(std::string* start, const Slice& limit) const override final
  {
     /// Nothing to do.
  }

  virtual void FindShortSuccessor(std::string* key) const override final
  {
     /// Nothing to do.
  }

protected:
   AComparator() = default;
};

/// Pairs account_name storage type and the ID to make possible nonunique index definition over names. 
typedef std::pair<account_name_type::Storage, size_t> account_name_storage_id_pair;

template <typename T>
class PrimitiveTypeComparatorImpl final : public AComparator
{
public:
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

private:
   const T& retrieveKey(const Slice& slice) const
   {
      assert(sizeof(T) == slice.size());
      const char* rawData = slice.data();
      return *reinterpret_cast<const T*>(rawData);
   }
};

class account_name_id_ComparatorImpl final : public AComparator
{
public:
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

private:
   const account_name_storage_id_pair& retrieveKey(const Slice& slice) const
   {
      assert(sizeof(account_name_storage_id_pair) == slice.size());
      const char* rawData = slice.data();
      const account_name_storage_id_pair* data = reinterpret_cast<const account_name_storage_id_pair*>(rawData);
      return *data;
   }
  
};

typedef PrimitiveTypeComparatorImpl<size_t> by_id_ComparatorImpl;
/** Location index is nonunique. Since RocksDB supports only unique indexes, it must be extended 
 *  by some unique part (ie ID).
 * 
 */
typedef std::pair<uint32_t, size_t> location_id_pair;
typedef PrimitiveTypeComparatorImpl<location_id_pair> by_location_ComparatorImpl;

const Comparator* by_id_Comparator()
{
   return ::rocksdb::BytewiseComparator();

   static by_id_ComparatorImpl c;
   return &c; 
}

const Comparator* by_location_Comparator()
{
   return ::rocksdb::BytewiseComparator();

   static by_location_ComparatorImpl c;
   return &c;
}

const Comparator* by_account_name_storage_id_pair_Comparator()
{
   return ::rocksdb::BytewiseComparator();

   static account_name_id_ComparatorImpl c;
   return &c;
}

} /// anonymous

class rocksdb_plugin::impl final
{
public:

   impl(const rocksdb_plugin& mainPlugin) :
      _mainDb(appbase::app().get_plugin<steem::plugins::chain::chain_plugin>().db()),
      _api(mainPlugin)
      {
      _pre_apply_connection = _mainDb.pre_apply_operation.connect( [&]( const operation_notification& note ){ on_operation(note); } );
      }

   ~impl()
   {
      shutdownDb();
   }

   void openDb(const bfs::path& path, uint32_t blockLimit)
   {
      bool doDataImport = createDbSchema(path);

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
         if(doDataImport)
            importData(blockLimit);
      }
      else
      {
         elog("RocksDB cannot open database at location: `${p}'.\nReturned error: ${e}",
            ("p", strPath)("e", status.ToString()));
      }
   }

   bool find_account_history_data(const account_name_type& name, account_history_object* data) const;
   bool find_operation_object(size_t opId, operation_object* data) const;
   /// Allows to look for all operations present in given block and call `processor` for them.
   void find_operations_by_block(size_t blockNum,
      std::function<void(const operation_object&)> processor) const;
   void shutdownDb()
   {
      chain::util::disconnect_signal(_pre_apply_connection);
      cleanupColumnHandles();
      _storage.reset();
   }

private:
   typedef std::vector<ColumnFamilyDescriptor> ColumnDefinitions;
   ColumnDefinitions prepareColumnDefinitions(bool addDefaultColumn);

   /// Returns true if database will need data import.
   bool createDbSchema(const bfs::path& path);

   void cleanupColumnHandles()
   {
      for(auto* h : _columnHandles)
         delete h;
      _columnHandles.clear();
   }

   void importOperation(uint32_t blockNum, const transaction_id_type& txId, uint32_t txInBlock,
      const operation& op, uint16_t opInTx, size_t opSeqId);

   void importData(unsigned int blockLimit)
   {
      ilog("Starting data import...");

      block_id_type lastBlock;
      size_t blockNo = 0;

      _lastTx = transaction_id_type();
      _txNo = 0;
      _totalOps = 0;

      benchmark_dumper dumper;
      dumper.initialize([](benchmark_dumper::database_object_sizeof_cntr_t&){}, "rocksdb_data_import.json");

      _mainDb.foreach_operation([blockLimit, &blockNo, &lastBlock, this](
         const signed_block& block, const signed_transaction& tx, uint32_t txInBlock, const operation& op, uint16_t opInTx) -> bool
      {
         if(lastBlock != block.previous)
         {
            blockNo = block.block_num();
            lastBlock = block.previous;

            if(blockLimit != 0 && blockNo > blockLimit)
            {
               ilog( "RocksDb data import stopped because of block limit reached.");
               return false;
            }
         }

         importOperation(blockNo, tx.id(), txInBlock, op, opInTx, _totalOps);

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
         ("tx", _txNo)
         ("op", _totalOps));
   }

   void flushWriteBuffer()
   {
      ::rocksdb::WriteOptions wOptions;
      auto s = _storage->Write(wOptions, &_writeBuffer);
      FC_ASSERT(s.ok(), "Data write failed");
      _writeBuffer.Clear();
      _collectedOps = 0;
   }

   void on_operation(const operation_notification& opNote);

   enum
   {
      WRITE_BUFFER_FLUSH_LIMIT = 100
   };

private:
   chain::database&                 _mainDb;
   rocksdb_api                      _api;
   std::unique_ptr<DB>              _storage;
   std::vector<ColumnFamilyHandle*> _columnHandles;
   ::rocksdb::WriteBatch            _writeBuffer;
   boost::signals2::connection      _pre_apply_connection;
   /// Helper member to be able to detect another incomming tx and increment tx-counter.
   transaction_id_type              _lastTx;
   size_t                           _txNo = 0;
   size_t                           _totalOps = 0;
   /// Number of data-chunks for ops being stored inside _writeBuffer. To decide when to flush.
   unsigned int                     _collectedOps = 0;
};


bool rocksdb_plugin::impl::find_account_history_data(const account_name_type& name, account_history_object* data) const
{
   std::unique_ptr<::rocksdb::Iterator> it(_storage->NewIterator(ReadOptions(), _columnHandles[3]));
   account_name_storage_id_pair nameIdPair(name.data, 0);
   PrimitiveTypeSlice<account_name_storage_id_pair> nameIdPairSlice(nameIdPair);

   //for(it->Seek(key); it->Valid() && it->key().starts_with(blockNumSlice); it->Next())
   
   return false;
}

bool rocksdb_plugin::impl::find_operation_object(size_t opId, operation_object* op) const
{
   std::string data;
   PrimitiveTypeSlice<size_t> idSlice(opId);
   ::rocksdb::Status s = _storage->Get(ReadOptions(), _columnHandles[1], idSlice, &data);

   if(s.ok())
   {
      load(*op, data.data(), data.size());
      return true;
   }
   
   FC_ASSERT(s.IsNotFound());

   return false;
}

void rocksdb_plugin::impl::find_operations_by_block(size_t blockNum,
   std::function<void(const operation_object&)> processor) const
{
   std::allocator<operation_object> a;
   std::unique_ptr<::rocksdb::Iterator> it(_storage->NewIterator(ReadOptions(), _columnHandles[2]));
   PrimitiveTypeSlice<uint32_t> blockNumSlice(blockNum);
   PrimitiveTypeSlice<location_id_pair> key(location_id_pair(blockNum, 0));

   for(it->Seek(key); it->Valid() && it->key().starts_with(blockNumSlice); it->Next())
   {
      auto valueSlice = it->value();
      const auto& opId = PrimitiveTypeSlice<size_t>::unpackSlice(valueSlice);

      operation_object op([](operation_object&){}, a);
      bool found = find_operation_object(opId, &op);
      FC_ASSERT(found);

      processor(op);
   }
}

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

   columnDefs.emplace_back("account_history_object_by_name_id", ColumnFamilyOptions());
   auto& byAccountNameColumn = columnDefs.back();
   byAccountNameColumn.options.comparator = by_account_name_storage_id_pair_Comparator();

   return columnDefs;
}

bool rocksdb_plugin::impl::createDbSchema(const bfs::path& path)
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
      return false; /// DB does not need data import.
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

      return true; /// DB needs data import
   }
   else
   {
      elog("RocksDB can not create storage at location: `${p}'.\nReturned error: ${e}",
         ("p", strPath)("e", s.ToString()));

      return false;
   }
}

void rocksdb_plugin::impl::importOperation(uint32_t blockNum, const transaction_id_type& txId,
   uint32_t txInBlock, const operation& op, uint16_t opInTx, size_t opSeqNo)
{
   if(_lastTx != txId)
   {
      ++_txNo;
      _lastTx = txId;
   }

   if(blockNum % 10000 == 0 && txInBlock == 0 && opInTx == 0)
   {
      ilog( "RocksDb data import processed blocks: ${n}, containing: ${tx} transactions and ${op} operations.",
         ("n", blockNum)
         ("tx", _txNo)
         ("op", _totalOps));
   }

   std::allocator<operation_object> a;
   operation_object obj([](operation_object&){}, a);
   obj.id._id       = opSeqNo;
   obj.trx_id       = txId;
   obj.block        = blockNum;
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

   flat_set<account_name_type> impacted;
   steem::app::operation_get_impacted_accounts(op, impacted);

   for(const auto& name : impacted)
   {
      account_name_storage_id_pair nameIdPair(name.data, obj.id._id);
      PrimitiveTypeSlice<account_name_storage_id_pair> nameIdPairSlice(nameIdPair);
      s = _writeBuffer.Put(_columnHandles[3], nameIdPairSlice, idSlice);
      FC_ASSERT(s.ok(), "Data write failed");
   }

   if(++this->_collectedOps >= WRITE_BUFFER_FLUSH_LIMIT)
      flushWriteBuffer();

   ++_totalOps;
}

void rocksdb_plugin::impl::on_operation(const operation_notification& n)
{
   //importOperation(n.block, n.trx_id, n.trx_in_block, n.op, n.op_in_trx, _totalOps);
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
      ("rocksdb-stop-import-at-block", bpo::value<uint32_t>()->default_value(0),
         "Allows to specify block number, the data import process should stop at.")
         
   ;
}

void rocksdb_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   if(options.count("rocksdb-path"))
      _dbPath = options.at("rocksdb-path").as<bfs::path>();

   if(options.count("rocksdb-stop-import-at-block"))
      _blockLimit = options.at("rocksdb-stop-import-at-block").as<uint32_t>();
}

void rocksdb_plugin::plugin_startup()
{
   ilog("Starting up rocksdb_plugin...");

   _my = std::make_unique<impl>(*this);
   if(_dbPath.is_absolute())
   {
      _my->openDb(_dbPath, _blockLimit);
   }
   else
   {
      auto basePath = appbase::app().get_plugin<steem::plugins::chain::chain_plugin>().state_storage_dir();
      auto actualPath = basePath / _dbPath;
      _my->openDb(actualPath, _blockLimit);
   }
}

void rocksdb_plugin::plugin_shutdown()
{
   ilog("Shutting down rocksdb_plugin...");
   _my->shutdownDb();
}

bool rocksdb_plugin::find_account_history_data(const account_name_type& name, account_history_object* data) const
{
   return _my->find_account_history_data(name, data);
}

bool rocksdb_plugin::find_operation_object(size_t opId, operation_object* data) const
{
   return _my->find_operation_object(opId, data);
}

void rocksdb_plugin::find_operations_by_block(size_t blockNum,
   std::function<void(const operation_object&)> processor) const
{
   _my->find_operations_by_block(blockNum, processor);
}

} } }
