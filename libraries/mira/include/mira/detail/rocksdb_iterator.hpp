#pragma once

#include <boost/operators.hpp>

#include <mira/multi_index_container_fwd.hpp>
#include <mira/detail/object_cache.hpp>

#include <rocksdb/db.h>

#include <iostream>

namespace mira { namespace multi_index { namespace detail {

template< typename Value, typename Key, typename KeyFromValue,
          typename KeyCompare, typename ID, typename IDFromValue >
struct rocksdb_iterator :
   public boost::bidirectional_iterator_helper<
      rocksdb_iterator< Value, Key, KeyFromValue, KeyCompare, ID, IDFromValue >,
      Value,
      std::size_t,
      const Value*,
      const Value& >
{
   typedef Key                                     key_type;
   typedef Value                                   value_type;
   typedef typename std::shared_ptr< value_type >  value_ptr;
   typedef multi_index_cache_manager< value_type > cache_type;

private:
   const column_handles&                           _handles;
   const size_t                                    _index = 0;

   std::unique_ptr< ::rocksdb::Iterator >          _iter;
   std::shared_ptr< ::rocksdb::ManagedSnapshot >   _snapshot;
   ::rocksdb::ReadOptions                          _opts;
   db_ptr                                          _db;

   cache_type&                                     _cache;
   IDFromValue                                     _get_id;

   KeyCompare                                      _compare;

   // Cached iterators
   key_type                                        _cache_key;
   bool                                            _cached_itr = false;

public:

   rocksdb_iterator( const column_handles& handles, size_t index, db_ptr db, cache_type& cache ) :
      _handles( handles ),
      _index( index ),
      _db( db ),
      _cache( cache )
   {
      // Not sure the implicit move constuctor for ManageSnapshot isn't going to release the snapshot...
      //_snapshot = std::make_shared< ::rocksdb::ManagedSnapshot >( &(*_db) );
      //_opts.snapshot = _snapshot->snapshot();
      //_iter.reset( _db->NewIterator( _opts, _handles[ _index ] ) );
   }
   rocksdb_iterator( const key_type& cache_key, const column_handles& handles, size_t index, db_ptr db, cache_type& cache ) :
      _handles( handles ),
      _index( index ),
      _db( db ),
      _cache( cache ),
      _cache_key( cache_key ),
      _cached_itr( true )
   {
   }

   rocksdb_iterator( const key_type& cache_key, const column_handles& handles, size_t index, db_ptr db, cache_type& cache, std::unique_ptr< ::rocksdb::Iterator > iter ) :
      _handles( handles ),
      _index( index ),
      _iter( std::move( iter ) ),
      _db( db ),
      _cache( cache ),
      _cache_key( cache_key ),
      _cached_itr( true )
   {
   }

   rocksdb_iterator( const rocksdb_iterator& other ) :
      _handles( other._handles ),
      _index( other._index ),
      _snapshot( other._snapshot ),
      _db( other._db ),
      _cache( other._cache ),
      _cache_key( other._cache_key ),
      _cached_itr( other._cached_itr )
   {
      if ( other._iter )
      {
         _iter.reset( _db->NewIterator( _opts, _handles[ _index] ) );

         if( other._iter->Valid() )
            _iter->Seek( other._iter->key() );
      }
   }

   rocksdb_iterator( const column_handles& handles, size_t index, db_ptr db, cache_type& cache, const Key& k ) :
      _handles( handles ),
      _index( index ),
      _db( db ),
      _cache( cache )
   {
      key_type* id = (key_type*)&k;
      if ( cache.get_index_cache( index )->contains( (void*)id ) )
      {
         _cache_key = k;
         _cached_itr = true;
      }
      else
      {
         _iter.reset( _db->NewIterator( _opts, _handles[ _index ] ) );

         PinnableSlice key_slice;
         pack_to_slice( key_slice, k );

         _iter->Seek( key_slice );

         if( !_iter->status().ok() )
         {
            std::cout << std::string( _iter->status().getState() ) << std::endl;
         }

         assert( _iter->status().ok() && _iter->Valid() );
      }
   }

   rocksdb_iterator( const column_handles& handles, size_t index, db_ptr db, cache_type& cache, const ::rocksdb::Slice& s  ) :
      _handles( handles ),
      _index( index ),
      _db( db ),
      _cache( cache )
   {
      Key k;
      unpack_from_slice( s, k );
      key_type* id = (key_type*)&k;
      if ( cache.get_index_cache( index )->contains( (void*)id ) )
      {
         _cache_key = k;
         _cached_itr = true;
      }
      else
      {
         //_snapshot = std::make_shared< ::rocksdb::ManagedSnapshot >( &(*_db) );
         //_opts.snapshot = _snapshot->snapshot();

         _iter.reset( _db->NewIterator( _opts, _handles[ _index ] ) );
         _iter->Seek( s );

         assert( _iter->status().ok() && _iter->Valid() );
      }
   }

   rocksdb_iterator( rocksdb_iterator&& other ) :
      _handles( other._handles ),
      _index( other._index ),
      _iter( std::move( other._iter ) ),
      _snapshot( other._snapshot ),
      _db( other._db ),
      _cache( other._cache ),
      _cache_key( other._cache_key ),
      _cached_itr( other._cached_itr )
   {
      //_opts.snapshot = _snapshot->snapshot();
      other._snapshot.reset();
      other._db.reset();
   }

   const value_type& operator*()
   {
      BOOST_ASSERT( valid() );
      value_ptr ptr;

      if ( _cached_itr )
      {
         ptr = _cache.get_index_cache( _index )->get( (void*)&_cache_key );
      }
      else
      {
         key_type key;

         unpack_from_slice( _iter->key(), key );
         ptr = _cache.get_index_cache( _index )->get( (void*)&key );

         if ( !ptr )
         {
            if ( _index == ID_INDEX )
            {
               // We are iterating on the primary key, so there is no indirection
               ::rocksdb::Slice value_slice = _iter->value();
               ptr = std::make_shared< value_type >();
               unpack_from_slice( value_slice, *ptr );
               ptr = _cache.cache( std::move( *ptr ) );
            }
            else
            {
               ::rocksdb::PinnableSlice value_slice;
               auto s = _db->Get( _opts, _handles[ ID_INDEX ], _iter->value(), &value_slice );
               assert( s.ok() );

               ptr = std::make_shared< value_type >();
               unpack_from_slice( value_slice, *ptr );
               ptr = _cache.cache( std::move( *ptr ) );
            }
         }

         _cached_itr = true;
         _cache_key = key;
      }

      return (*ptr);
   }

   const value_type* operator->()
   {
      return &(**this);
   }

   rocksdb_iterator& operator++()
   {
      //BOOST_ASSERT( valid() );
      if( !valid() ) _iter.reset( _db->NewIterator( _opts, _handles[ _index ] ) );

      if ( _cached_itr )
      {
         _cached_itr = false;

         if ( _iter == nullptr )
         {
            ::rocksdb::PinnableSlice slice;
            pack_to_slice( slice, _cache_key );

            _iter.reset( _db->NewIterator( _opts, _handles[ _index ] ) );
            _iter->Seek( slice );

            if( _iter->Valid() )
            {
               ::rocksdb::Slice found_key = _iter->key();
               if( memcmp( slice.data(), found_key.data(), std::min( slice.size(), found_key.size() ) ) != 0 )
               {
                  _iter.reset( _db->NewIterator( _opts, _handles[ _index ] ) );
                  return *this;
               }
            }
            else
            {
               _iter.reset( _db->NewIterator( _opts, _handles[ _index ] ) );
               return *this;
            }
         }
      }

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
      if( !valid() )
      {
         _iter.reset( _db->NewIterator( _opts, _handles[ _index ] ) );
         _iter->SeekToLast();
      }
      else
      {
         if ( _cached_itr )
         {
            _cached_itr = false;

            if ( _iter == nullptr )
            {
               ::rocksdb::PinnableSlice slice;
               pack_to_slice( slice, _cache_key );

               _iter.reset( _db->NewIterator( _opts, _handles[ _index ] ) );
               _iter->Seek( slice );

               if( _iter->Valid() )
               {
                  ::rocksdb::Slice found_key = _iter->key();
                  if( memcmp( slice.data(), found_key.data(), std::min( slice.size(), found_key.size() ) ) != 0 )
                  {
                     _iter.reset( _db->NewIterator( _opts, _handles[ _index ] ) );
                     return *this;
                  }
               }
               else
               {
                  _iter.reset( _db->NewIterator( _opts, _handles[ _index ] ) );
                  return *this;
               }
            }
         }
         _iter->Prev();
      }

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
      return _cached_itr || ( _iter && _iter->Valid() );
   }

   bool unchecked()const { return false; }

   bool equals( const rocksdb_iterator& other )const
   {
      if( valid() && other.valid() )
      {
         Key this_key, other_key;

         if ( _cached_itr )
            this_key = _cache_key;
         else
            unpack_from_slice( _iter->key(), this_key );

         if ( other._cached_itr )
            other_key = other._cache_key;
         else
            unpack_from_slice( other._iter->key(), other_key );

         return _compare( this_key, other_key ) == _compare( other_key, this_key );
      }

      return valid() == other.valid();
   }

   rocksdb_iterator& operator=( rocksdb_iterator&& other )
   {
      _iter = std::move( other._iter );
      _snapshot = other._snapshot;
      _db = other._db;
      _cached_itr = other._cached_itr;
      _cache_key = other._cache_key;

      return *this;
   }


   static rocksdb_iterator begin(
      const column_handles& handles,
      size_t index,
      db_ptr db,
      cache_type& cache )
   {
      rocksdb_iterator itr( handles, index, db, cache );
      itr._iter.reset( db->NewIterator( itr._opts, handles[ index ] ) );
      itr._iter->SeekToFirst();
      return itr;
   }

   static rocksdb_iterator end(
      const column_handles& handles,
      size_t index,
      db_ptr db,
      cache_type& cache )
   {
      return rocksdb_iterator( handles, index, db, cache );
   }

   template< typename CompatibleKey >
   static rocksdb_iterator find(
      const column_handles& handles,
      size_t index,
      db_ptr db,
      cache_type& cache,
      const CompatibleKey& k )
   {
      static KeyCompare compare = KeyCompare();
      rocksdb_iterator itr( handles, index, db, cache );
      itr._iter.reset( db->NewIterator( itr._opts, handles[ index ] ) );
      auto key = Key( k );

      PinnableSlice key_slice;
      pack_to_slice( key_slice, key );

      itr._iter->Seek( key_slice );

      if( itr.valid() )
      {
         Key found_key;
         unpack_from_slice( itr._iter->key(), found_key );

         if( compare( k, found_key ) != compare( found_key, k ) )
         {
            itr._iter.reset( itr._db->NewIterator( itr._opts, itr._handles[ itr._index ] ) );
         }
         else
         {
            key_type* key = (key_type*)&found_key;

            if ( cache.get_index_cache( index )->contains( (void*)key ) )
            {
               return rocksdb_iterator( *key, handles, index, db, cache, std::move( itr._iter ) );
            }
         }
      }
      else
      {
         itr._iter.reset( itr._db->NewIterator( itr._opts, itr._handles[ itr._index ] ) );
      }

      return itr;
   }

   static rocksdb_iterator find(
      const column_handles& handles,
      size_t index,
      db_ptr db,
      cache_type& cache,
      const Key& k )
   {
      static KeyCompare compare = KeyCompare();

      key_type* id = (key_type*)&k;
      if ( cache.get_index_cache( index )->contains( (void*)id ) )
      {
         return rocksdb_iterator( *id, handles, index, db, cache );
      }

      rocksdb_iterator itr( handles, index, db, cache );
      itr._iter.reset( db->NewIterator( itr._opts, handles[ index ] ) );

      PinnableSlice key_slice;
      pack_to_slice( key_slice, k );
      itr._iter->Seek( key_slice );

      if( itr.valid() )
      {
         Key found_key;
         unpack_from_slice( itr._iter->key(), found_key );

         if( compare( k, found_key ) != compare( found_key, k ) )
         {
            itr._iter.reset( itr._db->NewIterator( itr._opts, itr._handles[ itr._index ] ) );
         }
      }
      else
      {
         itr._iter.reset( itr._db->NewIterator( itr._opts, itr._handles[ itr._index ] ) );
      }

      return itr;
   }

   static rocksdb_iterator lower_bound(
      const column_handles& handles,
      size_t index,
      db_ptr db,
      cache_type& cache,
      const Key& k )
   {
      key_type* id = (key_type*)&k;
      if ( cache.get_index_cache( index )->contains( (void*)id ) )
      {
         return rocksdb_iterator( *id, handles, index, db, cache );
      }

      rocksdb_iterator itr( handles, index, db, cache );
      itr._iter.reset( db->NewIterator( itr._opts, handles[ index ] ) );

      PinnableSlice key_slice;
      pack_to_slice( key_slice, k );
      itr._iter->Seek( key_slice );

      return itr;
   }

   template< typename CompatibleKey >
   static rocksdb_iterator lower_bound(
      const column_handles& handles,
      size_t index,
      db_ptr db,
      cache_type& cache,
      const CompatibleKey& k )
   {
      return lower_bound( handles, index, db, cache, Key( k ) );
   }
/*
   static rocksdb_iterator upper_bound(
      const column_handles& handles,
      size_t index,
      db_ptr db,
      cache_type& cache,
      const Key& k )
   {
      rocksdb_iterator itr( handles, index, db, cache );
      itr._iter.reset( db->NewIterator( itr._opts, handles[ index ] ) );

      PinnableSlice key_slice;
      pack_to_slice( key_slice, k );

      itr._iter->SeekForPrev( key_slice );

      if( itr.valid() )
      {
         itr._iter->Next();
      }

      return itr;
   }
*/
   template< typename CompatibleKey >
   static rocksdb_iterator upper_bound(
      const column_handles& handles,
      size_t index,
      db_ptr db,
      cache_type& cache,
      const CompatibleKey& k )
   {
      static KeyCompare compare = KeyCompare();
      rocksdb_iterator itr( handles, index, db, cache );
      itr._iter.reset( db->NewIterator( itr._opts, handles[ index ] ) );

      auto key = Key( k );
      PinnableSlice key_slice;
      pack_to_slice( key_slice, key );

      itr._iter->Seek( key_slice );

      if( itr.valid() )
      {
         Key itr_key;
         unpack_from_slice( itr._iter->key(), itr_key );

         while( !compare( k, itr_key ) )
         {
            ++itr;
            if( !itr.valid() ) return itr;

            unpack_from_slice( itr._iter->key(), itr_key );
         }
      }

      return itr;
   }

   template< typename LowerBoundType, typename UpperBoundType >
   static std::pair< rocksdb_iterator, rocksdb_iterator > range(
      const column_handles& handles,
      size_t index,
      db_ptr db,
      cache_type& cache,
      const LowerBoundType& lower,
      const UpperBoundType& upper )
   {
      return std::make_pair< rocksdb_iterator, rocksdb_iterator >(
         lower_bound( handles, index, db, cache, lower ),
         upper_bound( handles, index, db, cache, upper )
      );
   }

   template< typename CompatibleKey >
   static std::pair< rocksdb_iterator, rocksdb_iterator > equal_range(
      const column_handles& handles,
      size_t index,
      db_ptr db,
      cache_type& cache,
      const CompatibleKey& k )
   {
      return std::make_pair< rocksdb_iterator, rocksdb_iterator >(
         lower_bound( handles, index, db, cache, k ),
         upper_bound( handles, index, db, cache, k )
      );
   }
};

template< typename Value, typename Key, typename KeyFromValue,
          typename KeyCompare, typename ID, typename IDFromValue >
bool operator==(
   const rocksdb_iterator< Value, Key, KeyFromValue, KeyCompare, ID, IDFromValue >& x,
   const rocksdb_iterator< Value, Key, KeyFromValue, KeyCompare, ID, IDFromValue >& y)
{
   return x.equals( y );
}

template< typename Value, typename Key, typename KeyFromValue,
          typename KeyCompare, typename ID, typename IDFromValue >
bool operator!=(
   const rocksdb_iterator< Value, Key, KeyFromValue, KeyCompare, ID, IDFromValue >& x,
   const rocksdb_iterator< Value, Key, KeyFromValue, KeyCompare, ID, IDFromValue >& y)
{
   return !( x == y );
}


} } } // mira::multi_index::detail
