
#include <steem/chain/steem_fwd.hpp>

#include <steem/plugins/account_history_rocksdb/account_history_rocksdb_plugin.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/history_object.hpp>
#include <steem/chain/index.hpp>
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

#define CURRENT_LIB 1
#define OPERATION_BY_ID 2
#define OPERATION_BY_BLOCK 3
#define AH_INFO_BY_NAME 4
#define AH_OPERATION_BY_ID 5

#define WRITE_BUFFER_FLUSH_LIMIT     10
#define ACCOUNT_HISTORY_LENGTH_LIMIT 30
#define ACCOUNT_HISTORY_TIME_LIMIT   30
#define VIRTUAL_OP_FLAG              0x8000000000000000

/** Because localtion_id_pair stores block_number paired with (VIRTUAL_OP_FLAG|operation_id),
 *  max allowed operation-id is max_int (instead of max_uint).
 */
#define MAX_OPERATION_ID             std::numeric_limits<int64_t>::max()

#define STORE_MAJOR_VERSION          1
#define STORE_MINOR_VERSION          0

namespace steem { namespace plugins { namespace account_history_rocksdb {

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
   int64_t        id = 0;
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

typedef PrimitiveTypeSlice< uint32_t > lib_id_slice_t;
typedef PrimitiveTypeSlice< uint32_t > lib_slice_t;

#define LIB_ID lib_id_slice_t( 0 )

/** Location index is nonunique. Since RocksDB supports only unique indexes, it must be extended
 *  by some unique part (ie ID).
 *
 */
typedef std::pair< uint32_t, uint64_t > block_op_id_pair;
typedef PrimitiveTypeComparatorImpl< block_op_id_pair > op_by_block_num_ComparatorImpl;

/// Compares account_history_info::id and rocksdb_operation_object::id pair
typedef std::pair< int64_t, uint32_t > ah_op_id_pair;
typedef PrimitiveTypeComparatorImpl< ah_op_id_pair > ah_op_by_id_ComparatorImpl;

typedef PrimitiveTypeSlice< int64_t > id_slice_t;
typedef PrimitiveTypeSlice< block_op_id_pair > op_by_block_num_slice_t;
typedef PrimitiveTypeSlice< uint32_t > by_block_slice_t;
typedef PrimitiveTypeSlice< account_name_type::Storage > ah_info_by_name_slice_t;
typedef PrimitiveTypeSlice< ah_op_id_pair > ah_op_by_id_slice_t;

const Comparator* by_id_Comparator()
{
   static by_id_ComparatorImpl c;
   return &c;
}

const Comparator* op_by_block_num_Comparator()
{
   static op_by_block_num_ComparatorImpl c;
   return &c;
}

const Comparator* by_account_name_Comparator()
{
   static by_account_name_ComparatorImpl c;
   return &c;
}

const Comparator* ah_op_by_id_Comparator()
{
   static ah_op_by_id_ComparatorImpl c;
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

      ah_info_by_name_slice_t key(name.data);
      PinnableSlice buffer;
      auto s = _storage->Get(ReadOptions(), _columnHandles[AH_INFO_BY_NAME], key, &buffer);
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
      ah_info_by_name_slice_t nameSlice(name.data);
      auto s = Put(_columnHandles[AH_INFO_BY_NAME], nameSlice, Slice(serializeBuf.data(), serializeBuf.size()));
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

class account_history_rocksdb_plugin::impl final
{
public:
   impl( account_history_rocksdb_plugin& self, const bpo::variables_map& options, const bfs::path& storagePath) :
      _self(self),
      _mainDb(appbase::app().get_plugin<steem::plugins::chain::chain_plugin>().db()),
      _storagePath(storagePath),
      _writeBuffer(_storage, _columnHandles)
      {
      collectOptions(options);

      _mainDb.add_pre_reindex_handler([&]( const steem::chain::reindex_notification& note ) -> void
         {
            on_pre_reindex( note );
         }, _self, 0);

      _mainDb.add_post_reindex_handler([&]( const steem::chain::reindex_notification& note ) -> void
         {
            on_post_reindex( note );
         }, _self, 0);

      add_plugin_index< volatile_operation_index >( _mainDb );
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

         const auto& rocksdb_plugin = appbase::app().get_plugin< account_history_rocksdb_plugin >();

         // I do not like using exceptions for control paths, but column definitions are set multiple times
         // opening the db, so that is not a good place to write the initial lib.
         try
         {
            get_lib();
         }
         catch( fc::assert_exception& )
         {
            update_lib( 0 );
         }

         _on_post_apply_operation_con = _mainDb.add_post_apply_operation_handler(
            [&]( const operation_notification& note )
            {
               on_post_apply_operation(note);
            },
            rocksdb_plugin
         );

         _on_irreversible_block_conn = _mainDb.add_irreversible_block_handler(
            [&]( uint32_t block_num )
            {
               on_irreversible_block( block_num );
            },
            rocksdb_plugin
         );
      }
      else
      {
         elog("RocksDB cannot open database at location: `${p}'.\nReturned error: ${e}",
            ("p", strPath)("e", status.ToString()));
      }
   }

   void printReport(uint32_t blockNo, const char* detailText) const;
   void on_pre_reindex( const steem::chain::reindex_notification& note );
   void on_post_reindex( const steem::chain::reindex_notification& note );

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
      chain::util::disconnect_signal(_on_post_apply_operation_con);
      chain::util::disconnect_signal(_on_irreversible_block_conn);
      flushStorage();
      cleanupColumnHandles();
      _storage.reset();
   }

private:
   uint32_t get_lib();
   void update_lib( uint32_t );

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

   template< typename T >
   void importOperation( rocksdb_operation_object& obj, const T& impacted )
   {
      if(_lastTx != obj.trx_id)
      {
         ++_txNo;
         _lastTx = obj.trx_id;
      }

      obj.id = _operationSeqId++;

      serialize_buffer_t serializedObj;
      auto size = fc::raw::pack_size(obj);
      serializedObj.resize(size);
      {
         fc::datastream<char*> ds(serializedObj.data(), size);
         fc::raw::pack(ds, obj);
      }

      id_slice_t idSlice(obj.id);
      auto s = _writeBuffer.Put(_columnHandles[OPERATION_BY_ID], idSlice, Slice(serializedObj.data(), serializedObj.size()));
      checkStatus(s);

      // uint64_t location = ( (uint64_t) obj.trx_in_block << 32 ) | ( (uint64_t) obj.op_in_trx << 16 ) | ( obj.virtual_op );

      uint64_t encoded_id = (uint64_t) obj.id;
      if( obj.virtual_op > 0 )
      {
         encoded_id |= VIRTUAL_OP_FLAG;
      }

      op_by_block_num_slice_t blockLocSlice( block_op_id_pair( obj.block, encoded_id ) );
      s = _writeBuffer.Put(_columnHandles[OPERATION_BY_BLOCK], blockLocSlice, idSlice);
      checkStatus(s);

      for(const auto& name : impacted)
         buildAccountHistoryRecord( name, obj );

      if(++_collectedOps >= _collectedOpsWriteLimit)
         flushWriteBuffer();

      ++_totalOps;
}

   void buildAccountHistoryRecord( const account_name_type& name, const rocksdb_operation_object& obj );
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

      id_slice_t opId(_operationSeqId);
      id_slice_t ahId(_accountHistorySeqId);

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
      _operationSeqId = id_slice_t::unpackSlice(buffer);

      s = storageDb->Get(rOptions, ahSeqIdName, &buffer);
      checkStatus(s);
      _accountHistorySeqId = id_slice_t::unpackSlice(buffer);

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

   void on_post_apply_operation(const operation_notification& opNote);

   void on_irreversible_block( uint32_t block_num );

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

/// Class attributes:
private:
   typedef flat_map< account_name_type, account_name_type > account_name_range_index;

   account_history_rocksdb_plugin&  _self;
   chain::database&                 _mainDb;
   bfs::path                        _storagePath;
   std::unique_ptr<DB>              _storage;
   std::vector<ColumnFamilyHandle*> _columnHandles;
   CachableWriteBatch               _writeBuffer;

   boost::signals2::connection      _on_post_apply_operation_con;
   boost::signals2::connection      _on_irreversible_block_conn;

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
   uint64_t                         _operationSeqId = 0;
   uint64_t                         _accountHistorySeqId = 0;

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

   bool                             _reindexing = false;

   bool                             _prune = false;
};

void account_history_rocksdb_plugin::impl::collectOptions(const boost::program_options::variables_map& options)
{
    fc::mutable_variant_object state_opts;

   typedef std::pair< account_name_type, account_name_type > pairstring;
   STEEM_LOAD_VALUE_SET(options, "account-history-rocksdb-track-account-range", _tracked_accounts, pairstring);

   state_opts[ "account-history-rocksdb-track-account-range" ] = _tracked_accounts;

   if(options.count("account-history-rocksdb-whitelist-ops"))
   {
      const auto& args = options.at("account-history-rocksdb-whitelist-ops").as<std::vector<std::string>>();
      storeOpFilteringParameters(args, &_op_list);

      if( _op_list.size() )
      {
         state_opts["account-history-rocksdb-whitelist-ops"] = _op_list;
      }
   }

   if(_op_list.empty() == false)
      ilog( "Account History: whitelisting ops ${o}", ("o", _op_list) );

   if(options.count("account-history-rocksdb-blacklist-ops"))
   {
      const auto& args = options.at("account-history-rocksdb-blacklist-ops").as<std::vector<std::string>>();
      storeOpFilteringParameters(args, &_blacklisted_op_list);

      if( _blacklisted_op_list.size() )
      {
         state_opts["account-history-rocksdb-blacklist-ops"] = _blacklisted_op_list;
      }
   }

   if(_blacklisted_op_list.empty() == false)
      ilog( "Account History: blacklisting ops ${o}", ("o", _blacklisted_op_list) );

   appbase::app().get_plugin< chain::chain_plugin >().report_state_options( _self.name(), state_opts );
}

inline bool account_history_rocksdb_plugin::impl::isTrackedAccount(const account_name_type& name) const
{
   if(_tracked_accounts.empty())
      return true;

   /// Code below is based on original contents of account_history_plugin_impl::on_post_apply_operation
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

std::vector<account_name_type> account_history_rocksdb_plugin::impl::getImpactedAccounts(const operation& op) const
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

inline bool account_history_rocksdb_plugin::impl::isTrackedOperation(const operation& op) const
{
   if(_op_list.empty() && _blacklisted_op_list.empty())
      return true;

   auto opName = operation_name_provider::getName(op);

   if(_blacklisted_op_list.find(opName) != _blacklisted_op_list.end())
      return false;

   bool isTracked = (_op_list.empty() || (_op_list.find(opName) != _op_list.end()));
   return isTracked;
}

void account_history_rocksdb_plugin::impl::storeOpFilteringParameters(const std::vector<std::string>& opList,
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

void account_history_rocksdb_plugin::impl::find_account_history_data(const account_name_type& name, uint64_t start,
   uint32_t limit, std::function<void(unsigned int, const rocksdb_operation_object&)> processor) const
{
   ReadOptions rOptions;

   ah_info_by_name_slice_t nameSlice(name.data);
   PinnableSlice buffer;
   auto s = _storage->Get(rOptions, _columnHandles[AH_INFO_BY_NAME], nameSlice, &buffer);

   if(s.IsNotFound())
      return;

   checkStatus(s);

   account_history_info ahInfo;
   load(ahInfo, buffer.data(), buffer.size());

   ah_op_by_id_slice_t lowerBoundSlice(std::make_pair(ahInfo.id, ahInfo.oldestEntryId));
   ah_op_by_id_slice_t upperBoundSlice(std::make_pair(ahInfo.id, ahInfo.newestEntryId+1));

   rOptions.iterate_lower_bound = &lowerBoundSlice;
   rOptions.iterate_upper_bound = &upperBoundSlice;

   ah_op_by_id_slice_t key(std::make_pair(ahInfo.id, start));
   id_slice_t ahIdSlice(ahInfo.id);

   std::unique_ptr<::rocksdb::Iterator> it(_storage->NewIterator(rOptions, _columnHandles[AH_OPERATION_BY_ID]));

   it->SeekForPrev(key);

   if(it->Valid() == false)
      return;

   auto keySlice = it->key();
   auto keyValue = ah_op_by_id_slice_t::unpackSlice(keySlice);

   auto lowerBound = keyValue.second > limit ? keyValue.second - limit : 0;

   for(; it->Valid(); it->Prev())
   {
      auto keySlice = it->key();
      if(keySlice.starts_with(ahIdSlice) == false)
         break;

      keyValue = ah_op_by_id_slice_t::unpackSlice(keySlice);

      auto valueSlice = it->value();
      const auto& opId = id_slice_t::unpackSlice(valueSlice);
      rocksdb_operation_object oObj;
      bool found = find_operation_object(opId, &oObj);
      FC_ASSERT(found, "Missing operation?");

      processor(keyValue.second, oObj);

      if(keyValue.second <= lowerBound)
        break;
   }
}

bool account_history_rocksdb_plugin::impl::find_operation_object(size_t opId, rocksdb_operation_object* op) const
{
   std::string data;
   id_slice_t idSlice(opId);
   ::rocksdb::Status s = _storage->Get(ReadOptions(), _columnHandles[OPERATION_BY_ID], idSlice, &data);

   if(s.ok())
   {
      load(*op, data.data(), data.size());
      return true;
   }

   FC_ASSERT(s.IsNotFound());

   return false;
}

void account_history_rocksdb_plugin::impl::find_operations_by_block(size_t blockNum,
   std::function<void(const rocksdb_operation_object&)> processor) const
{
   std::unique_ptr<::rocksdb::Iterator> it(_storage->NewIterator(ReadOptions(), _columnHandles[OPERATION_BY_BLOCK]));
   by_block_slice_t blockNumSlice(blockNum);
   op_by_block_num_slice_t key(block_op_id_pair(blockNum, 0));

   for(it->Seek(key); it->Valid() && it->key().starts_with(blockNumSlice); it->Next())
   {
      auto valueSlice = it->value();
      const auto& opId = id_slice_t::unpackSlice(valueSlice);

      rocksdb_operation_object op;
      bool found = find_operation_object(opId, &op);
      FC_ASSERT(found);

      processor(op);
   }
}

uint32_t account_history_rocksdb_plugin::impl::enumVirtualOperationsFromBlockRange(uint32_t blockRangeBegin,
   uint32_t blockRangeEnd, std::function<void(const rocksdb_operation_object&)> processor) const
{
   FC_ASSERT(blockRangeEnd > blockRangeBegin, "Block range must be upward");

   op_by_block_num_slice_t upperBoundSlice(block_op_id_pair(blockRangeEnd, 0));

   op_by_block_num_slice_t rangeBeginSlice(block_op_id_pair(blockRangeBegin, 0));

   ReadOptions rOptions;
   rOptions.iterate_upper_bound = &upperBoundSlice;

   std::unique_ptr<::rocksdb::Iterator> it(_storage->NewIterator(rOptions, _columnHandles[OPERATION_BY_BLOCK]));

   uint32_t lastFoundBlock = 0;

   for(it->Seek(rangeBeginSlice); it->Valid(); it->Next())
   {
      auto keySlice = it->key();
      const auto& key = op_by_block_num_slice_t::unpackSlice(keySlice);

      /// Accept only virtual operations
      if(key.second & VIRTUAL_OP_FLAG)
      {
         auto valueSlice = it->value();
         const auto& opId = id_slice_t::unpackSlice(valueSlice);

         rocksdb_operation_object op;
         bool found = find_operation_object(opId, &op);
         FC_ASSERT(found);

         processor(op);
         lastFoundBlock = op.block;
      }
   }

   op_by_block_num_slice_t lowerBoundSlice(block_op_id_pair(lastFoundBlock, 0));
   rOptions = ReadOptions();
   rOptions.iterate_lower_bound = &lowerBoundSlice;
   it.reset(_storage->NewIterator(rOptions, _columnHandles[OPERATION_BY_BLOCK]));

   op_by_block_num_slice_t nextRangeBeginSlice(block_op_id_pair(lastFoundBlock + 1, 0));
   for(it->Seek(nextRangeBeginSlice); it->Valid(); it->Next())
   {
      auto keySlice = it->key();
      const auto& key = op_by_block_num_slice_t::unpackSlice(keySlice);

      if(key.second & VIRTUAL_OP_FLAG)
         return key.first;
   }

   return 0;
}

uint32_t account_history_rocksdb_plugin::impl::get_lib()
{
   std::string data;
   auto s = _storage->Get(ReadOptions(), _columnHandles[CURRENT_LIB], LIB_ID, &data );

   FC_ASSERT( s.ok(), "Could not find last irreversible block." );

   uint32_t lib = 0;
   load( lib, data.data(), data.size() );
   return lib;
}

void account_history_rocksdb_plugin::impl::update_lib( uint32_t lib )
{
   auto s = _writeBuffer.Put( _columnHandles[ CURRENT_LIB ], LIB_ID, lib_slice_t( lib ) );
   checkStatus( s );
}

account_history_rocksdb_plugin::impl::ColumnDefinitions account_history_rocksdb_plugin::impl::prepareColumnDefinitions(bool addDefaultColumn)
{
   ColumnDefinitions columnDefs;
   if(addDefaultColumn)
      columnDefs.emplace_back(::rocksdb::kDefaultColumnFamilyName, ColumnFamilyOptions());

   columnDefs.emplace_back("current_lib", ColumnFamilyOptions());

   columnDefs.emplace_back("operation_by_id", ColumnFamilyOptions());
   auto& byIdColumn = columnDefs.back();
   byIdColumn.options.comparator = by_id_Comparator();

   columnDefs.emplace_back("operation_by_block", ColumnFamilyOptions());
   auto& byLocationColumn = columnDefs.back();
   byLocationColumn.options.comparator = op_by_block_num_Comparator();

   columnDefs.emplace_back("account_history_info_by_name", ColumnFamilyOptions());
   auto& byAccountNameColumn = columnDefs.back();
   byAccountNameColumn.options.comparator = by_account_name_Comparator();

   columnDefs.emplace_back("ah_operation_by_id", ColumnFamilyOptions());
   auto& byAHInfoColumn = columnDefs.back();
   byAHInfoColumn.options.comparator = ah_op_by_id_Comparator();

   return columnDefs;
}

bool account_history_rocksdb_plugin::impl::createDbSchema(const bfs::path& path)
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

void account_history_rocksdb_plugin::impl::buildAccountHistoryRecord( const account_name_type& name, const rocksdb_operation_object& obj )
{
   std::string strName = name;

   ReadOptions rOptions;
   //rOptions.tailing = true;

   ah_info_by_name_slice_t nameSlice(name.data);

   account_history_info ahInfo;
   bool found = _writeBuffer.getAHInfo(name, &ahInfo);

   if(found)
   {
      auto count = ahInfo.getAssociatedOpCount();

      if(_prune && count > ACCOUNT_HISTORY_LENGTH_LIMIT &&
         ((obj.timestamp - ahInfo.oldestEntryTimestamp) > fc::days(ACCOUNT_HISTORY_TIME_LIMIT)))
         {
            prunePotentiallyTooOldItems(&ahInfo, name, obj.timestamp);
         }

      auto nextEntryId = ++ahInfo.newestEntryId;
       _writeBuffer.putAHInfo(name, ahInfo);

      ah_op_by_id_slice_t ahInfoOpSlice(std::make_pair(ahInfo.id, nextEntryId));
      id_slice_t valueSlice(obj.id);
      auto s = _writeBuffer.Put(_columnHandles[AH_OPERATION_BY_ID], ahInfoOpSlice, valueSlice);
      checkStatus(s);
   }
   else
   {
      /// New entry must be created - there is first operation recorded.
      ahInfo.id = _accountHistorySeqId++;
      ahInfo.newestEntryId = ahInfo.oldestEntryId = 0;
      ahInfo.oldestEntryTimestamp = obj.timestamp;

      _writeBuffer.putAHInfo(name, ahInfo);

      ah_op_by_id_slice_t ahInfoOpSlice(std::make_pair(ahInfo.id, 0));
      id_slice_t valueSlice(obj.id);
      auto s = _writeBuffer.Put(_columnHandles[AH_OPERATION_BY_ID], ahInfoOpSlice, valueSlice);
      checkStatus(s);
   }
}

void account_history_rocksdb_plugin::impl::prunePotentiallyTooOldItems(account_history_info* ahInfo, const account_name_type& name,
   const fc::time_point_sec& now)
{
   std::string strName = name;

   auto ageLimit =  fc::days(ACCOUNT_HISTORY_TIME_LIMIT);

   ah_op_by_id_slice_t oldestEntrySlice(
      std::make_pair(ahInfo->id, ahInfo->oldestEntryId));
   auto lookupUpperBound = std::make_pair(ahInfo->id + 1,
      ahInfo->newestEntryId - ACCOUNT_HISTORY_LENGTH_LIMIT + 1);

   ah_op_by_id_slice_t newestEntrySlice(lookupUpperBound);

   ReadOptions rOptions;
   //rOptions.tailing = true;
   rOptions.iterate_lower_bound = &oldestEntrySlice;
   rOptions.iterate_upper_bound = &newestEntrySlice;

   auto s = _writeBuffer.SingleDelete(_columnHandles[AH_OPERATION_BY_ID], oldestEntrySlice);
   checkStatus(s);

   std::unique_ptr<::rocksdb::Iterator> dataItr(_storage->NewIterator(rOptions, _columnHandles[AH_OPERATION_BY_ID]));

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
      auto foundEntry = ah_op_by_id_slice_t::unpackSlice(key);

      if(foundEntry.first != ahInfo->id || foundEntry.second >= lookupUpperBound.second)
         break;

      auto value = dataItr->value();

      auto pointedOpId = id_slice_t::unpackSlice(value);
      rocksdb_operation_object op;
      find_operation_object(pointedOpId, &op);

      auto age = now - op.timestamp;

      if(age > ageLimit)
      {
         rightBoundary = foundEntry.second;
         ah_op_by_id_slice_t rightBoundarySlice(
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

void account_history_rocksdb_plugin::impl::on_pre_reindex(const steem::chain::reindex_notification& note)
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
   _reindexing = true;

   ilog("onReindexStart request completed successfully.");
}

void account_history_rocksdb_plugin::impl::on_post_reindex(const steem::chain::reindex_notification& note)
{
   ilog("Reindex completed up to block: ${b}. Setting back write limit to non-massive level.",
      ("b", note.last_block_number));

   flushStorage();
   _collectedOpsWriteLimit = 1;
   _reindexing = false;
   update_lib( note.last_block_number ); // We always reindex irreversible blocks.

   printReport( note.last_block_number, "RocksDB data reindex finished." );
}

void account_history_rocksdb_plugin::impl::printReport(uint32_t blockNo, const char* detailText) const
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

void account_history_rocksdb_plugin::impl::importData(unsigned int blockLimit)
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

      auto impacted = getImpactedAccounts( op );

      if( impacted.empty() )
         return true;

      rocksdb_operation_object obj;
      obj.trx_id = tx.id();
      obj.block = blockNo;
      obj.trx_in_block = txInBlock;
      obj.op_in_trx = opInTx;
      obj.timestamp = _mainDb.head_block_time();
      auto size = fc::raw::pack_size( op );
      obj.serialized_op.resize( size );
      fc::datastream< char* > ds( obj.serialized_op.data(), size );
      fc::raw::pack( ds, op );

      importOperation( obj, impacted );

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

void account_history_rocksdb_plugin::impl::on_post_apply_operation(const operation_notification& n)
{
   if( n.block % 10000 == 0 && n.trx_in_block == 0 && n.op_in_trx == 0 && n.virtual_op == 0 )
   {
      ilog("RocksDb data import processed blocks: ${n}, containing: ${tx} transactions and ${op} operations.\n"
           " ${ep} operations have been filtered out due to configured options.\n"
           " ${ea} accounts have been filtered out due to configured options.",
         ("n", n.block)
         ("tx", _txNo)
         ("op", _totalOps)
         ("ep", _excludedOps)
         ("ea", _excludedAccountCount)
         );
   }

   if( !isTrackedOperation(n.op) )
   {
      ++_excludedOps;
      return;
   }

   auto impacted = getImpactedAccounts(n.op);

   if( impacted.empty() )
      return; // Ignore operations not impacting any account (according to original implementation)

   if( _reindexing )
   {
      rocksdb_operation_object obj;
      obj.trx_id = n.trx_id;
      obj.block = n.block;
      obj.trx_in_block = n.trx_in_block;
      obj.op_in_trx = n.op_in_trx;
      obj.virtual_op = n.virtual_op;
      obj.timestamp = _mainDb.head_block_time();
      auto size = fc::raw::pack_size( n.op );
      obj.serialized_op.resize( size );
      fc::datastream< char* > ds( obj.serialized_op.data(), size );
      fc::raw::pack( ds, n.op );

      importOperation( obj, impacted );
   }
   else
   {
      _mainDb.create< volatile_operation_object >( [&]( volatile_operation_object& o )
      {
         o.trx_id = n.trx_id;
         o.block = n.block;
         o.trx_in_block = n.trx_in_block;
         o.op_in_trx = n.op_in_trx;
         o.virtual_op = n.virtual_op;
         o.timestamp = _mainDb.head_block_time();
         auto size = fc::raw::pack_size( n.op );
         o.serialized_op.resize( size );
         fc::datastream< char* > ds( o.serialized_op.data(), size );
         fc::raw::pack( ds, n.op );
         o.impacted.insert( o.impacted.end(), impacted.begin(), impacted.end() );
      });
   }
}

void account_history_rocksdb_plugin::impl::on_irreversible_block( uint32_t block_num )
{
   if( _reindexing ) return;

   if( block_num <= get_lib() ) return;

   const auto& volatile_idx = _mainDb.get_index< volatile_operation_index, by_block >();
   auto itr = volatile_idx.begin();

   vector< const volatile_operation_object* > to_delete;

   while( itr != volatile_idx.end() && itr->block <= block_num )
   {
      rocksdb_operation_object obj( *itr );
      importOperation( obj , itr->impacted );
      to_delete.push_back( &(*itr) );
      ++itr;
   }

   for( const volatile_operation_object* o : to_delete )
   {
      _mainDb.remove( *o );
   }

   update_lib( block_num );
}

account_history_rocksdb_plugin::account_history_rocksdb_plugin()
{
}

account_history_rocksdb_plugin::~account_history_rocksdb_plugin()
{

}

void account_history_rocksdb_plugin::set_program_options(
   boost::program_options::options_description &command_line_options,
   boost::program_options::options_description &cfg)
{
   cfg.add_options()
      ("account-history-rocksdb-path", bpo::value<bfs::path>()->default_value("blockchain/account-history-rocksdb-storage"),
         "The location of the rocksdb database for account history. By default it is $DATA_DIR/blockchain/account-history-rocksdb-storage")
      ("account-history-rocksdb-track-account-range", boost::program_options::value< std::vector<std::string> >()->composing()->multitoken(), "Defines a range of accounts to track as a json pair [\"from\",\"to\"] [from,to] Can be specified multiple times.")
      ("account-history-rocksdb-whitelist-ops", boost::program_options::value< std::vector<std::string> >()->composing(), "Defines a list of operations which will be explicitly logged.")
      ("account-history-rocksdb-blacklist-ops", boost::program_options::value< std::vector<std::string> >()->composing(), "Defines a list of operations which will be explicitly ignored.")

   ;
   command_line_options.add_options()
      ("account-history-rocksdb-immediate-import", bpo::bool_switch()->default_value(false),
         "Allows to force immediate data import at plugin startup. By default storage is supplied during reindex process.")
      ("account-history-rocksdb-stop-import-at-block", bpo::value<uint32_t>()->default_value(0),
         "Allows to specify block number, the data import process should stop at.")
   ;
}

void account_history_rocksdb_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   if(options.count("account-history-rocksdb-stop-import-at-block"))
      _blockLimit = options.at("account-history-rocksdb-stop-import-at-block").as<uint32_t>();

   _doImmediateImport = options.at("account-history-rocksdb-immediate-import").as<bool>();

   bfs::path dbPath;

   if(options.count("account-history-rocksdb-path"))
      dbPath = options.at("account-history-rocksdb-path").as<bfs::path>();

   if(dbPath.is_absolute() == false)
   {
      auto basePath = appbase::app().data_dir();
      auto actualPath = basePath / dbPath;
      dbPath = actualPath;
   }

   _my = std::make_unique<impl>( *this, options, dbPath );

   _my->openDb();
}

void account_history_rocksdb_plugin::plugin_startup()
{
   ilog("Starting up account_history_rocksdb_plugin...");

   if(_doImmediateImport)
      _my->importData(_blockLimit);
}

void account_history_rocksdb_plugin::plugin_shutdown()
{
   ilog("Shutting down account_history_rocksdb_plugin...");
   _my->shutdownDb();
}

void account_history_rocksdb_plugin::find_account_history_data(const account_name_type& name, uint64_t start, uint32_t limit,
   std::function<void(unsigned int, const rocksdb_operation_object&)> processor) const
{
   _my->find_account_history_data(name, start, limit, processor);
}

bool account_history_rocksdb_plugin::find_operation_object(size_t opId, rocksdb_operation_object* op) const
{
   return _my->find_operation_object(opId, op);
}

void account_history_rocksdb_plugin::find_operations_by_block(size_t blockNum,
   std::function<void(const rocksdb_operation_object&)> processor) const
{
   _my->find_operations_by_block(blockNum, processor);
}

uint32_t account_history_rocksdb_plugin::enum_operations_from_block_range(uint32_t blockRangeBegin, uint32_t blockRangeEnd,
   std::function<void(const rocksdb_operation_object&)> processor) const
{
   return _my->enumVirtualOperationsFromBlockRange(blockRangeBegin, blockRangeEnd, processor);
}

} } }

FC_REFLECT( steem::plugins::account_history_rocksdb::account_history_info,
   (id)(oldestEntryId)(newestEntryId)(oldestEntryTimestamp) )
