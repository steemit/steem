#include <steem/plugins/rocksdb/rocksdb_plugin.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/history_object.hpp>
#include <steem/chain/util/impacted.hpp>

#include <steem/plugins/chain/chain_plugin.hpp>

#include <steem/utilities/benchmark_dumper.hpp>
#include <steem/utilities/plugin_utilities.hpp>

#include <appbase/application.hpp>

#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/utilities/write_batch_with_index.h>

#include <boost/type.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/container/flat_set.hpp>

#include <limits>
#include <string>
#include <typeindex>
#include <typeinfo>

namespace bpo = boost::program_options;

#define STEEM_NAMESPACE_PREFIX "steem::protocol::"
#define OPEN_FILE_LIMIT 750

#define DIAGNOSTIC(s)
//#define DIAGNOSTIC(s) s

namespace steem { namespace plugins { namespace rocksdb {

using steem::protocol::account_name_type;
using steem::protocol::block_id_type;
using steem::protocol::operation;
using steem::protocol::signed_block;
using steem::protocol::signed_block_header;
using steem::protocol::signed_transaction;

using steem::chain::operation_notification;
using steem::chain::transaction_id_type;

using steem::utilities::benchmark_dumper;

using ::rocksdb::DB;
using ::rocksdb::DBOptions;
using ::rocksdb::Options;
using ::rocksdb::PinnableSlice;
using ::rocksdb::ReadOptions;
using ::rocksdb::Slice;
using ::rocksdb::Comparator;
using ::rocksdb::ColumnFamilyDescriptor;
using ::rocksdb::ColumnFamilyOptions;
using ::rocksdb::ColumnFamilyHandle;
using ::rocksdb::WriteBatch;

/** Represents an AH entry in mapped to account name.
 *  Holds additional informations, which are needed to simplify pruning process.
 *  All operations specific to given account, are next mapped to ID of given object.
 */
class account_history_info
{
public:
   uint32_t       id = 0;
   uint32_t       oldestEntryId = 0;
   uint32_t       newestEntryId = 0;
   /// Timestamp of oldest operation, just to quickly decide if start detail prune checking at all.
   time_point_sec oldestEntryTimestamp;

   uint32_t getAssociatedOpCount() const
   {
      return newestEntryId - oldestEntryId + 1;
   }
};

namespace
{
   template <class T>
   serialize_buffer_t dump(const T& obj)
   {
      serialize_buffer_t serializedObj;
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
   void load(T& obj, const serialize_buffer_t& source)
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

   static const T& unpackSlice(const std::string& s)
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
      if(a.size() != sizeof(T) || b.size() != sizeof(T))
         return a.compare(b);

      const T& id1 = retrieveKey(a);
      const T& id2 = retrieveKey(b);

      if(id1 < id2)
         return -1;

      if(id1 > id2)
         return 1;

      return 0;
  }

  virtual bool Equal(const Slice& a, const Slice& b) const override
  {
      if(a.size() != sizeof(T) || b.size() != sizeof(T))
         return a == b;

      const auto& id1 = retrieveKey(a);
      const auto& id2 = retrieveKey(b);

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

typedef PrimitiveTypeComparatorImpl<uint32_t> by_id_ComparatorImpl;

typedef PrimitiveTypeComparatorImpl<account_name_type::Storage> by_account_name_ComparatorImpl;

/** Location index is nonunique. Since RocksDB supports only unique indexes, it must be extended
 *  by some unique part (ie ID).
 *
 */
typedef std::pair<uint32_t, uint32_t> location_id_pair;
typedef PrimitiveTypeComparatorImpl<location_id_pair> by_location_ComparatorImpl;

/// Compares account_history_info::id and rocksdb_operation_object::id pair
typedef PrimitiveTypeComparatorImpl<std::pair<uint32_t, uint32_t>> by_ah_info_operation_ComparatorImpl;

const Comparator* by_id_Comparator()
{
   static by_id_ComparatorImpl c;
   return &c;
}

const Comparator* by_location_Comparator()
{
   static by_location_ComparatorImpl c;
   return &c;
}

const Comparator* by_account_name_Comparator()
{
   static by_account_name_ComparatorImpl c;
   return &c;
}

const Comparator* by_ah_info_operation_Comparator()
{
   static by_ah_info_operation_ComparatorImpl c;
   return &c;
}

#define checkStatus(s) FC_ASSERT((s).ok(), "Data access failed: ${m}", ("m", (s).ToString()))

class operation_name_provider
{
public:
   typedef void result_type;

   static std::string getName(const operation& op)
   {
      operation_name_provider provider;
      op.visit(provider);
      return provider._name;
   }

   template<typename Op>
   void operator()( const Op& ) const
   {
      _name = fc::get_typename<Op>::name();
   }

private:
   operation_name_provider() = default;

/// Class attributes:
private:
   mutable std::string _name;
};

class CachableWriteBatch : public WriteBatch
{
public:
   CachableWriteBatch(const std::unique_ptr<DB>& storage, const std::vector<ColumnFamilyHandle*>& columnHandles) :
      _storage(storage), _columnHandles(columnHandles) {}

   bool getAHInfo(const account_name_type& name, account_history_info* ahInfo) const
   {
      auto fi = _ahInfoCache.find(name);
      if(fi != _ahInfoCache.end())
      {
         *ahInfo = fi->second;
         return true;
      }

      PrimitiveTypeSlice<account_name_type::Storage> key(name.data);
      PinnableSlice buffer;
      auto s = _storage->Get(ReadOptions(), _columnHandles[3], key, &buffer);
      if(s.ok())
      {
         load(*ahInfo, buffer.data(), buffer.size());
         return true;
      }

      FC_ASSERT(s.IsNotFound());
      return false;
   }

   void putAHInfo(const account_name_type& name, const account_history_info& ahInfo)
   {
      _ahInfoCache[name] = ahInfo;
      auto serializeBuf = dump(ahInfo);
      PrimitiveTypeSlice<account_name_type::Storage> nameSlice(name.data);
      auto s = Put(_columnHandles[3], nameSlice, Slice(serializeBuf.data(), serializeBuf.size()));
      checkStatus(s);
   }

   void Clear()
   {
      _ahInfoCache.clear();
      WriteBatch::Clear();
   }

private:
   const std::unique_ptr<DB>&                        _storage;
   const std::vector<ColumnFamilyHandle*>&           _columnHandles;
   std::map<account_name_type, account_history_info> _ahInfoCache;
};


} /// anonymous

class rocksdb_plugin::impl final
{
public:
   impl( const bpo::variables_map& options, const bfs::path& storagePath) :
      _mainDb(appbase::app().get_plugin<steem::plugins::chain::chain_plugin>().db()),
      _storagePath(storagePath),
      _writeBuffer(_storage, _columnHandles)
      {
      collectOptions(options);

      _mainDb.on_reindex_start_connect([&]() -> void
         {
            onReindexStart();
         });

      _mainDb.on_reindex_done_connect([&](bool success, uint32_t finalBlock) -> void
         {
            onReindexStop(finalBlock);
         });
      }

   ~impl()
   {
      shutdownDb();
   }

   void openDb()
   {
      createDbSchema(_storagePath);

      auto columnDefs = prepareColumnDefinitions(true);

      DB* storageDb = nullptr;
      auto strPath = _storagePath.string();
      Options options;
      /// Optimize RocksDB. This is the easiest way to get RocksDB to perform well
      options.IncreaseParallelism();
      options.OptimizeLevelStyleCompaction();

      DBOptions dbOptions(options);
      options.max_open_files = OPEN_FILE_LIMIT;

      auto status = DB::Open(dbOptions, strPath, columnDefs, &_columnHandles, &storageDb);

      if(status.ok())
      {
         ilog("RocksDB opened successfully storage at location: `${p}'.", ("p", strPath));
         verifyStoreVersion(storageDb);
         loadSeqIdentifiers(storageDb);
         _storage.reset(storageDb);

         _pre_apply_connection = _mainDb.pre_apply_operation_proxy(
            [&]( const operation_notification& note )
            {
               on_operation(note);
            },
            appbase::app().get_plugin< rocksdb_plugin >() );
      }
      else
      {
         elog("RocksDB cannot open database at location: `${p}'.\nReturned error: ${e}",
            ("p", strPath)("e", status.ToString()));
      }
   }

   void printReport(uint32_t blockNo, const char* detailText) const;
   void onReindexStart();
   void onReindexStop(uint32_t finalBlock);

   /// Allows to start immediate data import (outside replay process).
   void importData(unsigned int blockLimit);

   void find_account_history_data(const account_name_type& name, uint64_t start, uint32_t limit,
      std::function<void(unsigned int, const rocksdb_operation_object&)> processor) const;
   bool find_operation_object(size_t opId, rocksdb_operation_object* op) const;
   /// Allows to look for all operations present in given block and call `processor` for them.
   void find_operations_by_block(size_t blockNum,
      std::function<void(const rocksdb_operation_object&)> processor) const;
   /// Allows to enumerate all operations registered in given block range.
   uint32_t enumVirtualOperationsFromBlockRange(uint32_t blockRangeBegin,
      uint32_t blockRangeEnd, std::function<void(const rocksdb_operation_object&)> processor) const;

   void shutdownDb()
   {
      chain::util::disconnect_signal(_pre_apply_connection);
      flushStorage();
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

   void importOperation(uint32_t blockNum, const fc::time_point_sec& blockTime, const transaction_id_type& txId,
      uint32_t txInBlock, const operation& op, uint16_t opInTx);
   void buildAccountHistoryRecord(const account_name_type& name, const rocksdb_operation_object& obj, const operation& op,
      const fc::time_point_sec& blockTime);
   void prunePotentiallyTooOldItems(account_history_info* ahInfo, const account_name_type& name,
      const fc::time_point_sec& now);

   void saveStoreVersion()
   {
      PrimitiveTypeSlice<uint32_t> majorVSlice(STORE_MAJOR_VERSION);
      PrimitiveTypeSlice<uint32_t> minorVSlice(STORE_MINOR_VERSION);

      auto s = _writeBuffer.Put(Slice("STORE_MAJOR_VERSION"), majorVSlice);
      checkStatus(s);
      s = _writeBuffer.Put(Slice("STORE_MINOR_VERSION"), minorVSlice);
      checkStatus(s);
   }

   void verifyStoreVersion(DB* storageDb)
   {
      ReadOptions rOptions;

      std::string buffer;
      auto s = storageDb->Get(rOptions, "STORE_MAJOR_VERSION", &buffer);
      checkStatus(s);
      const auto major = PrimitiveTypeSlice<uint32_t>::unpackSlice(buffer);

      FC_ASSERT(major == STORE_MAJOR_VERSION, "Store major version mismatch");

      s = storageDb->Get(rOptions, "STORE_MINOR_VERSION", &buffer);
      checkStatus(s);
      const auto minor = PrimitiveTypeSlice<uint32_t>::unpackSlice(buffer);

      FC_ASSERT(minor == STORE_MINOR_VERSION, "Store minor version mismatch");
   }

   void storeSequenceIds()
   {
      Slice opSeqIdName("OPERATION_SEQ_ID");
      Slice ahSeqIdName("AH_SEQ_ID");

      PrimitiveTypeSlice<size_t> opId(_operationSeqId);
      PrimitiveTypeSlice<size_t> ahId(_accountHistorySeqId);

      auto s = _writeBuffer.Put(opSeqIdName, opId);
      checkStatus(s);
      s = _writeBuffer.Put(ahSeqIdName, ahId);
      checkStatus(s);
   }

   void loadSeqIdentifiers(DB* storageDb)
   {
      Slice opSeqIdName("OPERATION_SEQ_ID");
      Slice ahSeqIdName("AH_SEQ_ID");

      ReadOptions rOptions;

      std::string buffer;
      auto s = storageDb->Get(rOptions, opSeqIdName, &buffer);
      checkStatus(s);
      _operationSeqId = PrimitiveTypeSlice<size_t>::unpackSlice(buffer);

      s = storageDb->Get(rOptions, ahSeqIdName, &buffer);
      checkStatus(s);
      _accountHistorySeqId = PrimitiveTypeSlice<size_t>::unpackSlice(buffer);

      ilog("Loaded OperationObject seqId: ${o}, AccountHistoryObject seqId: ${ah}.",
         ("o", _operationSeqId)("ah", _accountHistorySeqId));
   }

   void flushWriteBuffer(DB* storage = nullptr)
   {
      storeSequenceIds();

      if(storage == nullptr)
         storage = _storage.get();

      ::rocksdb::WriteOptions wOptions;
      auto s = storage->Write(wOptions, _writeBuffer.GetWriteBatch());
      checkStatus(s);
      _writeBuffer.Clear();
      _collectedOps = 0;
   }

   void flushStorage()
   {
      if(_storage == nullptr)
         return;

      /// If there are still not yet saved changes let's do it now.
      if(_collectedOps != 0)
         flushWriteBuffer();

      ::rocksdb::FlushOptions fOptions;
      for(const auto& cf : _columnHandles)
      {
         auto s = _storage->Flush(fOptions, cf);
         checkStatus(s);
      }
   }

   void on_operation(const operation_notification& opNote);

   void collectOptions(const bpo::variables_map& options);

   /** Returns true if given account is tracked.
    *  Depends on `"account-history-whitelist-ops"`, `account-history-blacklist-ops` option usage.
    *  Only some accounts can be chosen for tracking operation history.
    */
   bool isTrackedAccount(const account_name_type& name) const;

   /** Returns a collection of the accounts being impacted by given `op` and
    *  also tracked, because of passed options.
    *
    *  \see isTrackedAccount.
    */
   std::vector<account_name_type> getImpactedAccounts(const operation& op) const;

   /** Returns true if given operation should be collected.
    *  Depends on `account-history-blacklist-ops`, `account-history-whitelist-ops`.
    */
   bool isTrackedOperation(const operation& op) const;

   void storeOpFilteringParameters(const std::vector<std::string>& opList,
      flat_set<std::string>* storage) const;

   enum
   {
      WRITE_BUFFER_FLUSH_LIMIT     = 100,
      ACCOUNT_HISTORY_LENGTH_LIMIT = 30,
      ACCOUNT_HISTORY_TIME_LIMIT   = 30,

      VIRTUAL_OP_FLAG              = 0x80000000,
      /** Because localtion_id_pair stores block_number paired with (VIRTUAL_OP_FLAG|operation_id),
       *  max allowed operation-id is max_int (instead of max_uint).
       */
      MAX_OPERATION_ID             = std::numeric_limits<int>::max(),

      STORE_MAJOR_VERSION          = 1,
      STORE_MINOR_VERSION          = 0,
   };

/// Class attributes:
private:
   typedef flat_map< account_name_type, account_name_type > account_name_range_index;

   chain::database&                 _mainDb;
   bfs::path                        _storagePath;
   std::unique_ptr<DB>              _storage;
   std::vector<ColumnFamilyHandle*> _columnHandles;
   CachableWriteBatch               _writeBuffer;
   boost::signals2::connection      _pre_apply_connection;
   /// Helper member to be able to detect another incomming tx and increment tx-counter.
   transaction_id_type              _lastTx;
   size_t                           _txNo = 0;
   /// Total processed ops in this session (counts every operation, even excluded by filtering).
   size_t                           _totalOps = 0;
   /// Total number of ops being skipped by filtering options.
   size_t                           _excludedOps = 0;
   /// Total number of accounts (impacted by ops) excluded from processing because of filtering.
   mutable size_t                   _excludedAccountCount = 0;
   /// IDs to be assigned to object.id field.
   size_t                           _operationSeqId = 0;
   size_t                           _accountHistorySeqId = 0;

   /// Number of data-chunks for ops being stored inside _writeBuffer. To decide when to flush.
   unsigned int                     _collectedOps = 0;
   /** Limit which value depends on block data source:
    *    - if blocks come from network, there is no need for delaying write, becasue they appear quite rare (limit == 1)
    *    - if reindex process or direct import has been spawned, this massive operation can need reduction of direct
           writes (limit == WRITE_BUFFER_FLUSH_LIMIT).
    */
   unsigned int                     _collectedOpsWriteLimit = 1;

   account_name_range_index         _tracked_accounts;
   flat_set<std::string>            _op_list;
   flat_set<std::string>            _blacklisted_op_list;

   bool                             _prune = false;
};

void rocksdb_plugin::impl::collectOptions(const boost::program_options::variables_map& options)
{
   typedef std::pair< account_name_type, account_name_type > pairstring;
   STEEM_LOAD_VALUE_SET(options, "account-history-track-account-range", _tracked_accounts, pairstring);

   if(options.count("track-account-range"))
   {
      wlog( "track-account-range is deprecated in favor of account-history-track-account-range" );
      STEEM_LOAD_VALUE_SET( options, "track-account-range", _tracked_accounts, pairstring );
   }

   if(options.count("account-history-whitelist-ops"))
   {
      const auto& args = options.at("account-history-whitelist-ops").as<std::vector<std::string>>();
      storeOpFilteringParameters(args, &_op_list);
   }

   if( options.count( "history-whitelist-ops" ) )
   {
      wlog( "history-whitelist-ops is deprecated in favor of account-history-whitelist-ops." );

      const auto& args = options.at("history-whitelist-ops").as<std::vector<std::string>>();
      storeOpFilteringParameters(args, &_op_list);
   }

   if(_op_list.empty() == false)
      ilog( "Account History: whitelisting ops ${o}", ("o", _op_list) );

   if(options.count("account-history-blacklist-ops"))
   {
      const auto& args = options.at("account-history-blacklist-ops").as<std::vector<std::string>>();
      storeOpFilteringParameters(args, &_blacklisted_op_list);
   }

   if( options.count( "history-blacklist-ops" ) )
   {
      wlog( "history-blacklist-ops is deprecated in favor of account-history-blacklist-ops." );

      const auto& args = options.at("history-blacklist-ops").as<std::vector<std::string>>();
      storeOpFilteringParameters(args, &_blacklisted_op_list);
   }

   if(_blacklisted_op_list.empty() == false)
      ilog( "Account History: blacklisting ops ${o}", ("o", _blacklisted_op_list) );

   if(options.count( "history-disable-pruning"))
      _prune = !options["history-disable-pruning"].as<bool>();

   if(_prune)
   {
      wlog("****************************************************************************************************");
      wlog("*                       History pruning is deprecated. Please don't enable it.                     *");
      wlog("****************************************************************************************************");
   }
}

inline bool rocksdb_plugin::impl::isTrackedAccount(const account_name_type& name) const
{
   if(_tracked_accounts.empty())
      return true;

   /// Code below is based on original contents of account_history_plugin_impl::on_operation
   auto itr = _tracked_accounts.lower_bound(name);

   /*
      * The map containing the ranges uses the key as the lower bound and the value as the upper bound.
      * Because of this, if a value exists with the range (key, value], then calling lower_bound on
      * the map will return the key of the next pair. Under normal circumstances of those ranges not
      * intersecting, the value we are looking for will not be present in range that is returned via
      * lower_bound.
      *
      * Consider the following example using ranges ["a","c"], ["g","i"]
      * If we are looking for "bob", it should be tracked because it is in the lower bound.
      * However, lower_bound( "bob" ) returns an iterator to ["g","i"]. So we need to decrement the iterator
      * to get the correct range.
      *
      * If we are looking for "g", lower_bound( "g" ) will return ["g","i"], so we need to make sure we don't
      * decrement.
      *
      * If the iterator points to the end, we should check the previous (equivalent to rbegin)
      *
      * And finally if the iterator is at the beginning, we should not decrement it for obvious reasons
      */
   if(itr != _tracked_accounts.begin() &&
      ((itr != _tracked_accounts.end() && itr->first != name ) || itr == _tracked_accounts.end()))
   {
      --itr;
   }

   bool inRange = itr != _tracked_accounts.end() && itr->first <= name && name <= itr->second;

   _excludedAccountCount += inRange;

   DIAGNOSTIC(
      if(inRange)
         ilog("Account: ${a} matched to defined account range: [${b},${e}]", ("a", name)("b", itr->first)("e", itr->second) );
      else
         ilog("Account: ${a} ignored due to defined tracking filters.", ("a", name));
   );

   return inRange;
}

std::vector<account_name_type> rocksdb_plugin::impl::getImpactedAccounts(const operation& op) const
{
   flat_set<account_name_type> impacted;
   steem::app::operation_get_impacted_accounts(op, impacted);
   std::vector<account_name_type> retVal;

   if(impacted.empty())
      return retVal;

   if(_tracked_accounts.empty())
   {
      retVal.insert(retVal.end(), impacted.begin(), impacted.end());
      return retVal;
   }

   retVal.reserve(impacted.size());

   for(const auto& name : impacted)
   {
      if(isTrackedAccount(name))
         retVal.push_back(name);
   }

   return retVal;
}

inline bool rocksdb_plugin::impl::isTrackedOperation(const operation& op) const
{
   if(_op_list.empty() && _blacklisted_op_list.empty())
      return true;

   auto opName = operation_name_provider::getName(op);

   if(_blacklisted_op_list.find(opName) != _blacklisted_op_list.end())
      return false;

   bool isTracked = (_op_list.empty() || (_op_list.find(opName) != _op_list.end()));
   return isTracked;
}

void rocksdb_plugin::impl::storeOpFilteringParameters(const std::vector<std::string>& opList,
   flat_set<std::string>* storage) const
   {
      for(const auto& arg : opList)
      {
         std::vector<std::string> ops;
         boost::split(ops, arg, boost::is_any_of(" \t,"));

         for(const string& op : ops)
         {
            if( op.empty() == false )
               storage->insert( STEEM_NAMESPACE_PREFIX + op );
         }
      }
   }

void rocksdb_plugin::impl::find_account_history_data(const account_name_type& name, uint64_t start,
   uint32_t limit, std::function<void(unsigned int, const rocksdb_operation_object&)> processor) const
{
   ReadOptions rOptions;

   PrimitiveTypeSlice<account_name_type::Storage> nameSlice(name.data);
   PinnableSlice buffer;
   auto s = _storage->Get(rOptions, _columnHandles[3], nameSlice, &buffer);

   if(s.IsNotFound())
      return;

   checkStatus(s);

   account_history_info ahInfo;
   load(ahInfo, buffer.data(), buffer.size());

   PrimitiveTypeSlice<std::pair<uint32_t, uint32_t>> lowerBoundSlice(std::make_pair(ahInfo.id, ahInfo.oldestEntryId));
   PrimitiveTypeSlice<std::pair<uint32_t, uint32_t>> upperBoundSlice(std::make_pair(ahInfo.id, ahInfo.newestEntryId+1));

   rOptions.iterate_lower_bound = &lowerBoundSlice;
   rOptions.iterate_upper_bound = &upperBoundSlice;

   PrimitiveTypeSlice<std::pair<uint32_t, uint32_t>> key(std::make_pair(ahInfo.id, start));
   PrimitiveTypeSlice<uint32_t> ahIdSlice(ahInfo.id);

   std::unique_ptr<::rocksdb::Iterator> it(_storage->NewIterator(rOptions, _columnHandles[4]));

   it->SeekForPrev(key);

   if(it->Valid() == false)
      return;

   auto keySlice = it->key();
   auto keyValue = PrimitiveTypeSlice<std::pair<uint32_t, uint32_t>>::unpackSlice(keySlice);

   auto lowerBound = keyValue.second > limit ? keyValue.second - limit : 0;

   for(; it->Valid(); it->Prev())
   {
      auto keySlice = it->key();
      if(keySlice.starts_with(ahIdSlice) == false)
         break;

      keyValue = PrimitiveTypeSlice<std::pair<uint32_t, uint32_t>>::unpackSlice(keySlice);

      auto valueSlice = it->value();
      const auto& opId = PrimitiveTypeSlice<uint32_t>::unpackSlice(valueSlice);
      rocksdb_operation_object oObj;
      bool found = find_operation_object(opId, &oObj);
      FC_ASSERT(found, "Missing operation?");

//      ilog("AH-info-id: ${a}, Entry: ${e}, OperationId: ${oid}", ("a", keyValue.first)("e", keyValue.second)("oid", oObj.id));

      processor(keyValue.second, oObj);

      if(keyValue.second <= lowerBound)
        break;
   }
}

bool rocksdb_plugin::impl::find_operation_object(size_t opId, rocksdb_operation_object* op) const
{
   std::string data;
   PrimitiveTypeSlice<uint32_t> idSlice(opId);
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
   std::function<void(const rocksdb_operation_object&)> processor) const
{
   std::unique_ptr<::rocksdb::Iterator> it(_storage->NewIterator(ReadOptions(), _columnHandles[2]));
   PrimitiveTypeSlice<uint32_t> blockNumSlice(blockNum);
   PrimitiveTypeSlice<location_id_pair> key(location_id_pair(blockNum, 0));

   for(it->Seek(key); it->Valid() && it->key().starts_with(blockNumSlice); it->Next())
   {
      auto valueSlice = it->value();
      const auto& opId = PrimitiveTypeSlice<size_t>::unpackSlice(valueSlice);

      rocksdb_operation_object op;
      bool found = find_operation_object(opId, &op);
      FC_ASSERT(found);

      processor(op);
   }
}

uint32_t rocksdb_plugin::impl::enumVirtualOperationsFromBlockRange(uint32_t blockRangeBegin,
   uint32_t blockRangeEnd, std::function<void(const rocksdb_operation_object&)> processor) const
{
   FC_ASSERT(blockRangeEnd > blockRangeBegin, "Block range must be upward");

   PrimitiveTypeSlice<location_id_pair> upperBoundSlice(location_id_pair(blockRangeEnd, 0));

   PrimitiveTypeSlice<location_id_pair> rangeBeginSlice(location_id_pair(blockRangeBegin, 0));

   ReadOptions rOptions;
   rOptions.iterate_upper_bound = &upperBoundSlice;

   std::unique_ptr<::rocksdb::Iterator> it(_storage->NewIterator(rOptions, _columnHandles[2]));

   uint32_t lastFoundBlock = 0;

   for(it->Seek(rangeBeginSlice); it->Valid(); it->Next())
   {
      auto keySlice = it->key();
      const auto& key = PrimitiveTypeSlice<location_id_pair>::unpackSlice(keySlice);

      /// Accept only virtual operations
      if(key.second & VIRTUAL_OP_FLAG)
      {
         auto valueSlice = it->value();
         const auto& opId = PrimitiveTypeSlice<size_t>::unpackSlice(valueSlice);

         rocksdb_operation_object op;
         bool found = find_operation_object(opId, &op);
         FC_ASSERT(found);

         processor(op);
         lastFoundBlock = op.block;
      }
   }

   PrimitiveTypeSlice<location_id_pair> lowerBoundSlice(location_id_pair(lastFoundBlock, 0));
   rOptions = ReadOptions();
   rOptions.iterate_lower_bound = &lowerBoundSlice;
   it.reset(_storage->NewIterator(rOptions, _columnHandles[2]));

   PrimitiveTypeSlice<location_id_pair> nextRangeBeginSlice(location_id_pair(lastFoundBlock + 1, 0));
   for(it->Seek(nextRangeBeginSlice); it->Valid(); it->Next())
   {
      auto keySlice = it->key();
      const auto& key = PrimitiveTypeSlice<location_id_pair>::unpackSlice(keySlice);

      if(key.second & VIRTUAL_OP_FLAG)
         return key.first;
   }

   return 0;
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

   columnDefs.emplace_back("account_history_info_by_name", ColumnFamilyOptions());
   auto& byAccountNameColumn = columnDefs.back();
   byAccountNameColumn.options.comparator = by_account_name_Comparator();

   columnDefs.emplace_back("ah_info_operation_by_ids", ColumnFamilyOptions());
   auto& byAHInfoColumn = columnDefs.back();
   byAHInfoColumn.options.comparator = by_ah_info_operation_Comparator();

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
         saveStoreVersion();
         /// Store initial values of Seq-IDs for held objects.
         flushWriteBuffer(db);
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

void rocksdb_plugin::impl::importOperation(uint32_t blockNum, const fc::time_point_sec& blockTime,
   const transaction_id_type& txId, uint32_t txInBlock, const operation& op, uint16_t opInTx)
{
   if(_lastTx != txId)
   {
      ++_txNo;
      _lastTx = txId;
   }

   if(blockNum % 10000 == 0 && txInBlock == 0 && opInTx == 0)
   {
      ilog("RocksDb data import processed blocks: ${n}, containing: ${tx} transactions and ${op} operations.\n"
           " ${ep} operations have been filtered out due to configured options.\n"
           " ${ea} accounts have been filtered out due to configured options.",
         ("n", blockNum)
         ("tx", _txNo)
         ("op", _totalOps)
         ("ep", _excludedOps)
         ("ea", _excludedAccountCount)
         );
   }

   auto impacted = getImpactedAccounts(op);

   if(impacted.empty())
      return; /// Ignore operations not impacting any account (according to original implementation)

   FC_ASSERT(_operationSeqId < MAX_OPERATION_ID, "Operation id limit exceeded");

   rocksdb_operation_object obj;
   obj.id           = _operationSeqId++;
   obj.trx_id       = txId;
   obj.block        = blockNum;
   obj.trx_in_block = txInBlock;
   obj.timestamp    = blockTime;
   auto size = fc::raw::pack_size(op);
   obj.serialized_op.resize(size);
   {
      fc::datastream<char*> ds(obj.serialized_op.data(), size);
      fc::raw::pack(ds, op);
   }

   serialize_buffer_t serializedObj;
   size = fc::raw::pack_size(obj);
   serializedObj.resize(size);
   {
      fc::datastream<char*> ds(serializedObj.data(), size);
      fc::raw::pack(ds, obj);
   }

   PrimitiveTypeSlice<uint32_t> idSlice(obj.id);
   auto s = _writeBuffer.Put(_columnHandles[1], idSlice, Slice(serializedObj.data(), serializedObj.size()));
   checkStatus(s);

   uint32_t encodedId = obj.id;
   if(is_virtual_operation(op))
      encodedId |= VIRTUAL_OP_FLAG;

   PrimitiveTypeSlice<location_id_pair> blockNoIdSlice(location_id_pair(blockNum, encodedId));
   s = _writeBuffer.Put(_columnHandles[2], blockNoIdSlice, idSlice);
   checkStatus(s);

   if(isTrackedOperation(op))
   {
      for(const auto& name : impacted)
         buildAccountHistoryRecord(name, obj, op, blockTime);
   }
   else
   {
      ++_excludedOps;
   }

   if(++_collectedOps >= _collectedOpsWriteLimit)
      flushWriteBuffer();

   ++_totalOps;
}

void rocksdb_plugin::impl::buildAccountHistoryRecord(const account_name_type& name, const rocksdb_operation_object& obj,
   const operation& op, const fc::time_point_sec& blockTime)
{
   std::string strName = name;

   ReadOptions rOptions;
   //rOptions.tailing = true;

   PrimitiveTypeSlice<account_name_type::Storage> nameSlice(name.data);

   account_history_info ahInfo;
   bool found = _writeBuffer.getAHInfo(name, &ahInfo);

   if(found)
   {
      auto count = ahInfo.getAssociatedOpCount();

      if(_prune && count > ACCOUNT_HISTORY_LENGTH_LIMIT &&
         ((blockTime - ahInfo.oldestEntryTimestamp) > fc::days(ACCOUNT_HISTORY_TIME_LIMIT)))
         {
            prunePotentiallyTooOldItems(&ahInfo, name, blockTime);
         }

      auto nextEntryId = ++ahInfo.newestEntryId;
       _writeBuffer.putAHInfo(name, ahInfo);

      PrimitiveTypeSlice<std::pair<uint32_t, uint32_t>> ahInfoOpSlice(std::make_pair(ahInfo.id, nextEntryId));
      PrimitiveTypeSlice<uint32_t> valueSlice(obj.id);
      auto s = _writeBuffer.Put(_columnHandles[4], ahInfoOpSlice, valueSlice);
      checkStatus(s);

   // if(strName == "blocktrades")
   // {
   //    ilog("Block: ${b}: Storing another AH entry ${e} for account: `${a}' (${ahID}). Operation block: ${ob}, Operation id: ${oid}",
   //       ("b", _mainDb.head_block_num())
   //       ("e", nextEntryId)
   //       ("a", strName)
   //       ("ahID", ahInfo.id)
   //       ("ob", obj.block)
   //       ("oid", obj.id)
   //    );
   // }

   }
   else
   {
      /// New entry must be created - there is first operation recorded.
      ahInfo.id = _accountHistorySeqId++;
      ahInfo.newestEntryId = ahInfo.oldestEntryId = 0;
      ahInfo.oldestEntryTimestamp = obj.timestamp;

      _writeBuffer.putAHInfo(name, ahInfo);

      PrimitiveTypeSlice<std::pair<unsigned int, unsigned int>> ahInfoOpSlice(std::make_pair(ahInfo.id, 0));
      PrimitiveTypeSlice<unsigned int> valueSlice(obj.id);
      auto s = _writeBuffer.Put(_columnHandles[4], ahInfoOpSlice, valueSlice);
      checkStatus(s);

   // if(strName == "blocktrades")
   // {
   //    blocktrades_id = ahInfo.id;
   //    ilog("Block: ${b}: Storing FIRST AH entry ${e} for account: `${a}' (${ahID}). Operation block: ${ob}, OPeration id: ${oid}",
   //       ("b", _mainDb.head_block_num())
   //       ("e", 0)
   //       ("a", strName)
   //       ("ahID", ahInfo.id)
   //       ("ob", obj.block)
   //       ("oid", obj.id)
   //    );
   // }

   }
}

void rocksdb_plugin::impl::prunePotentiallyTooOldItems(account_history_info* ahInfo, const account_name_type& name,
   const fc::time_point_sec& now)
{
   std::string strName = name;

   auto ageLimit =  fc::days(ACCOUNT_HISTORY_TIME_LIMIT);

   PrimitiveTypeSlice<std::pair<uint32_t, uint32_t>> oldestEntrySlice(
      std::make_pair(ahInfo->id, ahInfo->oldestEntryId));
   auto lookupUpperBound = std::make_pair(ahInfo->id + 1,
      ahInfo->newestEntryId - ACCOUNT_HISTORY_LENGTH_LIMIT + 1);

   PrimitiveTypeSlice<std::pair<uint32_t, uint32_t>> newestEntrySlice(lookupUpperBound);

   ReadOptions rOptions;
   //rOptions.tailing = true;
   rOptions.iterate_lower_bound = &oldestEntrySlice;
   rOptions.iterate_upper_bound = &newestEntrySlice;

   auto s = _writeBuffer.SingleDelete(_columnHandles[4], oldestEntrySlice);
   checkStatus(s);

   std::unique_ptr<::rocksdb::Iterator> dataItr(_storage->NewIterator(rOptions, _columnHandles[4]));

   /** To clean outdated records we have to iterate over all AH records having subsequent number greater than limit
    *  and additionally verify date of operation, to clean up only these exceeding a date limit.
    *  So just operations having a list position > ACCOUNT_HISTORY_LENGTH_LIMIT and older that ACCOUNT_HISTORY_TIME_LIMIT
    *  shall be removed.
    */
   dataItr->Seek(oldestEntrySlice);

   /// Boundaries of keys to be removed
   //uint32_t leftBoundary = ahInfo->oldestEntryId;
   uint32_t rightBoundary = ahInfo->oldestEntryId;

   for(; dataItr->Valid(); dataItr->Next())
   {
      auto key = dataItr->key();
      auto foundEntry = PrimitiveTypeSlice<std::pair<uint32_t, uint32_t>>::unpackSlice(key);

      if(foundEntry.first != ahInfo->id || foundEntry.second >= lookupUpperBound.second)
         break;

      auto value = dataItr->value();

      auto pointedOpId = PrimitiveTypeSlice<uint32_t>::unpackSlice(value);
      rocksdb_operation_object op;
      find_operation_object(pointedOpId, &op);

      auto age = now - op.timestamp;

      if(age > ageLimit)
      {
         rightBoundary = foundEntry.second;
         PrimitiveTypeSlice<std::pair<uint32_t, uint32_t>> rightBoundarySlice(
            std::make_pair(ahInfo->id, rightBoundary));
         s = _writeBuffer.SingleDelete(_columnHandles[4], rightBoundarySlice);
         checkStatus(s);
      }
      else
      {
         ahInfo->oldestEntryId = foundEntry.second;
         ahInfo->oldestEntryTimestamp = op.timestamp;
         FC_ASSERT(ahInfo->oldestEntryId <= ahInfo->newestEntryId);

         break;
      }
   }
}

void rocksdb_plugin::impl::onReindexStart()
{
   ilog("Received onReindexStart request, attempting to clean database storage.");

   shutdownDb();
   std::string strPath = _storagePath.string();

   auto s = ::rocksdb::DestroyDB(strPath, ::rocksdb::Options());
   checkStatus(s);

   openDb();

   ilog("Setting write limit to massive level");

   _collectedOpsWriteLimit = WRITE_BUFFER_FLUSH_LIMIT;

   _lastTx = transaction_id_type();
   _txNo = 0;
   _totalOps = 0;
   _excludedOps = 0;

   ilog("onReindexStart request completed successfully.");
}

void rocksdb_plugin::impl::onReindexStop(uint32_t finalBlock)
{
   ilog("Reindex completed up to block: ${b}. Setting back write limit to non-massive level.",
      ("b", finalBlock));

   flushStorage();
   _collectedOpsWriteLimit = 1;

   printReport(finalBlock, "RocksDB data reindex finished. ");
}

void rocksdb_plugin::impl::printReport(uint32_t blockNo, const char* detailText) const
{
   ilog("${t}Processed blocks: ${n}, containing: ${tx} transactions and ${op} operations.\n"
        "${ep} operations have been filtered out due to configured options.\n"
        "${ea} accounts have been filtered out due to configured options.",
      ("t", detailText)
      ("n", blockNo)
      ("tx", _txNo)
      ("op", _totalOps)
      ("ep", _excludedOps)
      ("ea", _excludedAccountCount)
      );
}

void rocksdb_plugin::impl::importData(unsigned int blockLimit)
{
   if(_storage == nullptr)
   {
      ilog("RocksDB has no opened storage. Skipping data import...");
      return;
   }

   ilog("Starting data import...");

   block_id_type lastBlock;
   size_t blockNo = 0;

   _lastTx = transaction_id_type();
   _txNo = 0;
   _totalOps = 0;
   _excludedOps = 0;

   benchmark_dumper dumper;
   dumper.initialize([](benchmark_dumper::database_object_sizeof_cntr_t&){}, "rocksdb_data_import.json");

   _mainDb.foreach_operation([blockLimit, &blockNo, &lastBlock, this](
      const signed_block_header& prevBlockHeader, const signed_block& block, const signed_transaction& tx,
      uint32_t txInBlock, const operation& op, uint16_t opInTx) -> bool
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

      importOperation(blockNo, prevBlockHeader.timestamp, tx.id(), txInBlock, op, opInTx);

      return true;
   }
   );

   if(_collectedOps != 0)
      flushWriteBuffer();

   const auto& measure = dumper.measure(blockNo, [](benchmark_dumper::index_memory_details_cntr_t&, bool){});
   ilog( "RocksDb data import - Performance report at block ${n}. Elapsed time: ${rt} ms (real), ${ct} ms (cpu). Memory usage: ${cm} (current), ${pm} (peak) kilobytes.",
      ("n", blockNo)
      ("rt", measure.real_ms)
      ("ct", measure.cpu_ms)
      ("cm", measure.current_mem)
      ("pm", measure.peak_mem) );

   printReport(blockNo, "RocksDB data import finished. ");
}

void rocksdb_plugin::impl::on_operation(const operation_notification& n)
{
   if(_storage != nullptr)
   {
      auto blockTime = _mainDb.head_block_time();
      importOperation(n.block, blockTime, n.trx_id, n.trx_in_block, n.op, n.op_in_trx);
   }
}

rocksdb_plugin::rocksdb_plugin()
{
}

rocksdb_plugin::~rocksdb_plugin()
{

}

void rocksdb_plugin::set_program_options(
   boost::program_options::options_description &command_line_options,
   boost::program_options::options_description &cfg)
{
   cfg.add_options()
      ("rocksdb-path", bpo::value<bfs::path>()->default_value("rocksdb_storage"),
         "Allows to specify path where rocksdb store will be located. If path is relative, actual directory is made as `shared-file-dir` subdirectory.")
      ("account-history-track-account-range", boost::program_options::value< std::vector<std::string> >()->composing()->multitoken(), "Defines a range of accounts to track as a json pair [\"from\",\"to\"] [from,to] Can be specified multiple times.")
      ("track-account-range", boost::program_options::value< std::vector<std::string> >()->composing()->multitoken(), "Defines a range of accounts to track as a json pair [\"from\",\"to\"] [from,to] Can be specified multiple times. Deprecated in favor of account-history-track-account-range.")
      ("account-history-whitelist-ops", boost::program_options::value< std::vector<std::string> >()->composing(), "Defines a list of operations which will be explicitly logged.")
      ("history-whitelist-ops", boost::program_options::value< std::vector<std::string> >()->composing(), "Defines a list of operations which will be explicitly logged. Deprecated in favor of account-history-whitelist-ops.")
      ("account-history-blacklist-ops", boost::program_options::value< std::vector<std::string> >()->composing(), "Defines a list of operations which will be explicitly ignored.")
      ("history-blacklist-ops", boost::program_options::value< std::vector<std::string> >()->composing(), "Defines a list of operations which will be explicitly ignored. Deprecated in favor of account-history-blacklist-ops.")
      ("history-disable-pruning", boost::program_options::value< bool >()->default_value( false ), "Disables automatic account history trimming" )

   ;
   command_line_options.add_options()
      ("rocksdb-immediate-import", bpo::bool_switch()->default_value(false),
         "Allows to force immediate data import at plugin startup. By default storage is supplied during reindex process.")
      ("rocksdb-stop-import-at-block", bpo::value<uint32_t>()->default_value(0),
         "Allows to specify block number, the data import process should stop at.")
   ;
}

void rocksdb_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   if(options.count("rocksdb-stop-import-at-block"))
      _blockLimit = options.at("rocksdb-stop-import-at-block").as<uint32_t>();

   _doImmediateImport = options.at("rocksdb-immediate-import").as<bool>();

   bfs::path dbPath;

   if(options.count("rocksdb-path"))
      dbPath = options.at("rocksdb-path").as<bfs::path>();

   if(dbPath.is_absolute() == false)
   {
      auto basePath = appbase::app().get_plugin<steem::plugins::chain::chain_plugin>().state_storage_dir();
      auto actualPath = basePath / dbPath;
      dbPath = actualPath;
   }

   _my = std::make_unique<impl>( options, dbPath );

   _my->openDb();
}

void rocksdb_plugin::plugin_startup()
{
   ilog("Starting up rocksdb_plugin...");

   if(_doImmediateImport)
      _my->importData(_blockLimit);
}

void rocksdb_plugin::plugin_shutdown()
{
   ilog("Shutting down rocksdb_plugin...");
   _my->shutdownDb();
}

void rocksdb_plugin::find_account_history_data(const account_name_type& name, uint64_t start, uint32_t limit,
   std::function<void(unsigned int, const rocksdb_operation_object&)> processor) const
{
   _my->find_account_history_data(name, start, limit, processor);
}

bool rocksdb_plugin::find_operation_object(size_t opId, rocksdb_operation_object* op) const
{
   return _my->find_operation_object(opId, op);
}

void rocksdb_plugin::find_operations_by_block(size_t blockNum,
   std::function<void(const rocksdb_operation_object&)> processor) const
{
   _my->find_operations_by_block(blockNum, processor);
}

uint32_t rocksdb_plugin::enum_operations_from_block_range(uint32_t blockRangeBegin, uint32_t blockRangeEnd,
   std::function<void(const rocksdb_operation_object&)> processor) const
{
   return _my->enumVirtualOperationsFromBlockRange(blockRangeBegin, blockRangeEnd, processor);
}

} } }

FC_REFLECT( steem::plugins::rocksdb::account_history_info,
   (id)(oldestEntryId)(newestEntryId)(oldestEntryTimestamp) )
