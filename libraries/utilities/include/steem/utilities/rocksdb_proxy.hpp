#pragma once

#include <steem/protocol/types.hpp>

#include <rocksdb/db.h>

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

namespace steem { namespace utilities {

namespace bfs = boost::filesystem;

using protocol::account_name_type;

using ::rocksdb::DB;
using ::rocksdb::ColumnFamilyHandle;
using ::rocksdb::WriteBatch;
using ::rocksdb::Slice;
using ::rocksdb::PinnableSlice;
using ::rocksdb::ReadOptions;
using ::rocksdb::Comparator;
using ::rocksdb::Env;
using ::rocksdb::Status;
using ::rocksdb::DBOptions;
using ::rocksdb::Options;
using ::rocksdb::ColumnFamilyDescriptor;
using ::rocksdb::ColumnFamilyOptions;

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

struct any_info
{
   virtual uint32_t getAssociatedOpCount() const{ return 0; };
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

/** Helper class to simplify construction of Slice objects holding primitive type values.
 *
 */
template <typename T>
class primitive_type_slice final : public Slice
{
public:
   explicit primitive_type_slice(T value) : _value(value)
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

template< typename ITEM >
class cachable_write_batch : public WriteBatch
{
public:
   cachable_write_batch(const std::unique_ptr<DB>& storage, const std::vector<ColumnFamilyHandle*>& columnHandles) :
      _storage(storage), _columnHandles(columnHandles) {}

   bool getInfo(const account_name_type& name, ITEM* info) const
   {
      auto fi = _InfoCache.find(name);
      if(fi != _InfoCache.end())
      {
         *info = fi->second;
         return true;
      }

      primitive_type_slice<account_name_type::Storage> key(name.data);
      PinnableSlice buffer;
      auto s = _storage->Get(ReadOptions(), _columnHandles[3], key, &buffer);
      if(s.ok())
      {
         load(*info, buffer.data(), buffer.size());
         return true;
      }

      FC_ASSERT(s.IsNotFound());
      return false;
   }

   void putInfo(const account_name_type& name, const ITEM& info)
   {
      _InfoCache[name] = info;
      auto serializeBuf = dump( info );
      primitive_type_slice<account_name_type::Storage> nameSlice(name.data);
      auto s = Put(_columnHandles[3], nameSlice, Slice(serializeBuf.data(), serializeBuf.size()));
      checkStatus(s);
   }

   void Clear()
   {
      _InfoCache.clear();
      WriteBatch::Clear();
   }

private:

   const std::unique_ptr<DB>&                        _storage;
   const std::vector<ColumnFamilyHandle*>&           _columnHandles;

   std::map< account_name_type, ITEM > _InfoCache;
};

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

namespace rocksdb_types
{
   using ColumnFamilyOptions = ::rocksdb::ColumnFamilyOptions;

   using ColumnDefinitions = std::vector<ColumnFamilyDescriptor>;
   using column_definitions_preparer = std::function< void( bool, ColumnDefinitions& ) >;
   
   using slice_info_pair = std::pair< std::string, size_t >;
   using slice_info_items = std::list< slice_info_pair >;

   const Comparator* by_id_Comparator();
   const Comparator* by_location_Comparator();
   const Comparator* by_account_name_Comparator();
   const Comparator* by_ah_info_operation_Comparator();
}

} } // steem::utilities
