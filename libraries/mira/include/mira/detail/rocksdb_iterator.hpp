#pragma once

#include <mira/multi_index_container_fwd.hpp>

#include <rocksdb/db.h>

#include <fc/io/raw.hpp>

namespace mira { namespace multi_index { namespace detail {

#define ID_INDEX 1

template< typename Value, typename Key >
struct rocksdb_iterator
{
private:
   const column_handles&                           _handles;
   const size_t                                    _index = 0;

   std::unique_ptr< ::rocksdb::Iterator >          _iter;
   std::shared_ptr< ::rocksdb::ManagedSnapshot >   _snapshot;
   ::rocksdb::ReadOptions                          _opts;
   db_ptr                                          _db;

   rocksdb_iterator( const column_handles& handles, size_t index, db_ptr db ) :
      _handles( handles ),
      _index( index ),
      _db( db )
   {
      // Not sure the implicit move constuctor for ManageSnapshot isn't going to release the snapshot...
      _snapshot = std::make_shared< ::rocksdb::ManagedSnapshot >( &(*_db) );
      _opts.snapshot = _snapshot->snapshot();
      _iter.reset( _db->NewIterator( _opts, _handles[ _index ] ) );
   }

   rocksdb_iterator( const rocksdb_iterator& other ) :
      _handles( other._handles ),
      _index( other._index ),
      _snapshot( other._snapshot ),
      _db( other._db )
   {
      _iter.reset( _db->NewIterator( _opts, _handles[ _index] ) );

      if( other._iter->Valid() )
         _iter->Seek( other._iter->key() );
   }

public:

   rocksdb_iterator( const column_handles& handles, size_t index, db_ptr db, const Key& k ) :
      _handles( handles ),
      _index( index ),
      _db( db )
   {
      _snapshot = std::make_shared< ::rocksdb::ManagedSnapshot >( &(*_db) );
      _opts.snapshot = _snapshot->snapshot();

      std::vector< char > ser_key = fc::raw::pack_to_vector( k );

      _iter.reset( _db->NewIterator( _opts, _handles[ _index ] ) );
      _iter->Seek( ::rocksdb::Slice( ser_key.data(), ser_key.size() ) );

      assert( _iter->status().ok() && _iter->Valid() );
   }

   rocksdb_iterator( const column_handles& handles, size_t index, db_ptr db, const ::rocksdb::Slice& s ) :
      _handles( handles ),
      _index( index ),
      _db( db )
   {
      _snapshot = std::make_shared< ::rocksdb::ManagedSnapshot >( &(*_db) );
      _opts.snapshot = _snapshot->snapshot();

      _iter.reset( _db->NewIterator( _opts, _handles[ _index ] ) );
      _iter->Seek( s );

      assert( _iter->status().ok() && _iter->Valid() );
   }

   rocksdb_iterator( rocksdb_iterator&& other ) :
      _handles( other._handles ),
      _index( other._index ),
      _iter( std::move( other._iter ) ),
      _snapshot( other._snapshot ),
      _db( other._db )
   {
      _opts.snapshot = _snapshot->snapshot();
      other._snapshot.reset();
      other._db.reset();
   }

   Value operator*()const
   {
      BOOST_ASSERT( valid() );
      ::rocksdb::Slice key_slice = _iter->value();

      if( _index == ID_INDEX )
      {
         // We are iterating on the primary key, so there is no indirection
         return fc::raw::unpack_from_char_array< Value >( key_slice.data(), key_slice.size() );
      }

      ::rocksdb::PinnableSlice value_slice;
      auto s = _db->Get( _opts, _handles[ ID_INDEX ], key_slice, &value_slice );
      assert( s.ok() );

      return fc::raw::unpack_from_char_array< Value >( value_slice.data(), value_slice.size() );
   }

   rocksdb_iterator& operator++()
   {
      //BOOST_ASSERT( valid() );
      _iter->Next();
      assert( _iter->status().ok() );
      return *this;
   }

   rocksdb_iterator operator++(int)const
   {
      //BOOST_ASSERT( valid() );
      rocksdb_iterator new_itr( *this );
      return ++new_itr;
   }

   rocksdb_iterator& operator--()
   {
      _iter->Prev();
      assert( _iter->status().ok() );
      return *this;
   }

   rocksdb_iterator operator--(int)const
   {
      rocksdb_iterator new_itr( *this );
      return --new_itr;
   }

   bool valid()const
   {
      return _iter->Valid();
   }

   bool unchecked()const { return false; }

   bool equals( const rocksdb_iterator& other )const
   {
      if( _iter->Valid() && other._iter->Valid() )
      {
         ::rocksdb::Slice this_key = _iter->key();
         ::rocksdb::Slice other_key = other._iter->key();
         assert( this_key.size() == other_key.size() );
         return memcmp( this_key.data(), other_key.data(), this_key.size() ) == 0;
      }

      return _iter->Valid() == other._iter->Valid();
   }


   static rocksdb_iterator begin(
      const column_handles& handles,
      size_t index,
      db_ptr db )
   {
      rocksdb_iterator itr( handles, index, db );
      itr._iter->SeekToFirst();
      return itr;
   }

   static rocksdb_iterator end(
      const column_handles& handles,
      size_t index,
      db_ptr db )
   {
      return rocksdb_iterator( handles, index, db );
   }

   template< typename CompatibleKey >
   static rocksdb_iterator find(
      const column_handles& handles,
      size_t index,
      db_ptr db,
      const CompatibleKey& k )
   {
      rocksdb_iterator itr( handles, index, db );

      std::vector< char > ser_key = fc::raw::pack_to_vector( k );
      itr._iter->Seek( ::rocksdb::Slice( ser_key.data(), ser_key.size() ) );

      ::rocksdb::Slice found_key = itr._iter->key();
      if( memcmp( ser_key.data(), found_key.data(), std::min( ser_key.size(), found_key.size() ) ) != 0 )
      {
         itr._iter.reset( itr._db->NewIterator( itr._opts, itr._handles[ itr._index ] ) );
      }

      return itr;
   }

   template< typename CompatibleKey >
   static rocksdb_iterator lower_bound(
      const column_handles& handles,
      size_t index,
      db_ptr db,
      const CompatibleKey& k )
   {
      rocksdb_iterator itr( handles, index, db );

      std::vector< char > ser_key = fc::raw::pack_to_vector( k );
      itr._iter->Seek( ::rocksdb::Slice( ser_key.data(), ser_key.size() ) );

      return itr;
   }

   template< typename CompatibleKey >
   static rocksdb_iterator upper_bound(
      const column_handles& handles,
      size_t index,
      db_ptr db,
      const CompatibleKey& k )
   {
      rocksdb_iterator itr( handles, index, db );

      std::vector< char > ser_key = fc::raw::pack_to_char_array( k );
      itr._iter->SeekForPrev( ::rocksdb::Slice( ser_key.data(), ser_key.size() ) );

      ::rocksdb::Slice found_key = itr._iter->key();
      if( memcmp( ser_key.data(), found_key.data(), std::min( ser_key.size(), found_key.size() ) ) == 0 )
      {
         itr._iter->Next();
      }

      return itr;
   }

   template< typename LowerBoundType, typename UpperBoundType >
   static std::pair< rocksdb_iterator, rocksdb_iterator > range(
      const column_handles& handles,
      size_t index,
      db_ptr db,
      const LowerBoundType& lower,
      const UpperBoundType& upper )
   {
      return std::make_pair< rocksdb_iterator, rocksdb_iterator >(
         lower_bound( handles, index, db, lower ),
         upper_bound( handles, index, db, upper )
      );
   }
};

template< typename Value, typename Key >
bool operator==(
   const rocksdb_iterator< Value, Key >& x,
   const rocksdb_iterator< Value, Key >& y)
{
   return x.equals( y );
}

template< typename Value, typename Key >
bool operator!=(
   const rocksdb_iterator< Value, Key >& x,
   const rocksdb_iterator< Value, Key >& y)
{
   return !( x == y );
}


} } } // mira::multi_index::detail
