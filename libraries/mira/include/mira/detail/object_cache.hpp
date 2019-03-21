#pragma once

#include <boost/core/ignore_unused.hpp>
#include <boost/any.hpp>
#include <fc/log/logger.hpp>
#include <list>
#include <map>
#include <memory>
#include <iostream>
#include <mutex>

namespace mira { namespace multi_index { namespace detail {

class abstract_multi_index_cache_manager
{
public:
   abstract_multi_index_cache_manager() = default;
   virtual ~abstract_multi_index_cache_manager() = default;

   virtual bool purgeable( boost::any v ) = 0;
   virtual void purge( boost::any v ) = 0;
};

class lru_cache_manager
{
public:
   typedef std::list<
      std::pair<
         boost::any,
         std::shared_ptr< abstract_multi_index_cache_manager >
      >
   > list_type;

   typedef list_type::iterator        iterator_type;
   typedef list_type::const_iterator  const_iterator_type;

private:
   list_type    _lru;
   size_t       _obj_threshold = 5;
   std::mutex   _lock;

public:
   iterator_type insert( boost::any v, std::shared_ptr< abstract_multi_index_cache_manager >&& m )
   {
      std::lock_guard< std::mutex > lock( _lock );
      return _lru.insert( _lru.begin(), std::make_pair( v, m ) );
   }

   void update( const_iterator_type iter )
   {
      std::lock_guard< std::mutex > lock( _lock );
      _lru.splice( _lru.begin(), _lru, iter );
   }

   void remove( iterator_type iter )
   {
      std::lock_guard< std::mutex > lock( _lock );
      _lru.erase( iter );
   }

   void set_object_threshold( size_t capacity )
   {
      _obj_threshold = capacity;
   }

   void adjust_capacity()
   {
      adjust_capacity( _obj_threshold );
   }

   void adjust_capacity( size_t cap )
   {
      static size_t attempts = 0;
      if ( _lru.size() > cap )
      {
         attempts = 0;
         size_t max_attempts = _lru.size() - cap;
         auto it = _lru.end();
         do
         {
            // Prevents an infinite loop in the case
            // where everything in the cache is considered
            // non-purgeable
            if ( attempts++ == max_attempts )
               break;

            --it;

            // If we can purge this item, we do,
            // otherwise we move it to the front
            // so we don't repeatedly ask if it's
            // purgeable
            if ( it->second->purgeable( it->first ) )
               it->second->purge( it->first );
            else
               update( it );

            it = _lru.end();
         } while ( _lru.size() > cap );
      }
   }
};

struct cache_manager
{
   static std::shared_ptr< lru_cache_manager >& get( bool reset = false )
   {
      static std::shared_ptr< lru_cache_manager > cache_ptr;

      if( !cache_ptr || reset )
         cache_ptr = std::make_shared< lru_cache_manager >();

      return cache_ptr;
   }

   static void reset()
   {
      get( true );
   }
};

typedef const void* cache_key_type;

template< typename Value >
struct cache_factory;

template < typename Value >
class multi_index_cache_manager;

template < typename Value >
class abstract_index_cache
{
public:
   abstract_index_cache() = default;
   virtual ~abstract_index_cache() = default;

   friend class multi_index_cache_manager< Value >;
   typedef typename std::shared_ptr< Value > ptr_type;
   typedef std::pair< ptr_type, lru_cache_manager::iterator_type > cache_bundle_type;

   virtual ptr_type get( cache_key_type key ) = 0;
   virtual void update( cache_key_type key, Value&& v ) = 0;
   virtual void update( cache_key_type key, Value&& v, const std::vector< size_t >& modified_indices ) = 0;
   virtual bool contains( cache_key_type key ) = 0;
   virtual std::mutex& get_lock() = 0;

protected:
   std::shared_ptr< multi_index_cache_manager< Value > > _multi_index_cache_manager;

private:
   virtual void set_multi_index_cache_manager( std::shared_ptr< multi_index_cache_manager< Value > >&& m )
   {
      _multi_index_cache_manager = m;
   }

   virtual void cache( cache_bundle_type bundle ) = 0;
   virtual void invalidate( const Value& v ) = 0;
   virtual void invalidate( const Value& v, bool update_cache_manager ) = 0;
   virtual void clear() = 0;
   virtual void clear( bool update_cache_manager ) = 0;
   virtual size_t usage() const = 0;
   virtual size_t size() const = 0;
};

template < typename Value >
class multi_index_cache_manager :
   public abstract_multi_index_cache_manager,
   public std::enable_shared_from_this< multi_index_cache_manager< Value > >
{
public:
   multi_index_cache_manager() = default;
   ~multi_index_cache_manager() = default;
   typedef std::unique_ptr< abstract_index_cache< Value > >              index_cache_type;
   typedef std::shared_ptr< Value >                                      ptr_type;
   typedef std::weak_ptr< Value >                                        manager_ptr_type;
   typedef cache_factory< Value >                                        factory_type;
   typedef std::pair< ptr_type, lru_cache_manager::iterator_type >       cache_bundle_type;

private:
   std::map< size_t, index_cache_type > _index_caches;
   std::mutex                           _lock;

public:
   void set_index_cache( size_t index, index_cache_type&& index_cache )
   {
      index_cache->set_multi_index_cache_manager( this->shared_from_this() );
      _index_caches[ index ] = std::move( index_cache );
   }

   virtual bool purgeable( boost::any v )
   {
      manager_ptr_type value = boost::any_cast< manager_ptr_type >( v );
      assert( !value.expired() );

      if ( (size_t)value.use_count() > _index_caches.size() )
         return false;

      return true;
   }

   virtual void purge( boost::any v )
   {
      manager_ptr_type value = boost::any_cast< manager_ptr_type >( v );
      assert( !value.expired() );
      invalidate( *value.lock() );
   }

   const index_cache_type& get_index_cache( size_t index )
   {
      assert( index >= 1 );
      assert( index <= _index_caches.size() );
      return _index_caches[ index ];
   }

   ptr_type cache( const Value& value )
   {
      Value v = value;
      return cache( std::move( v ) );
   }

   ptr_type cache( Value&& value )
   {
      ptr_type p = std::make_shared< Value >( std::move( value ) );
      return cache( p );
   }

   ptr_type cache( ptr_type value )
   {
      manager_ptr_type lru_value = value;
      auto it = cache_manager::get()->insert( lru_value, this->shared_from_this() );

      cache_bundle_type bundle = std::make_pair( value, it );

      for ( auto& c : _index_caches )
         c.second->cache( bundle );

      return value;
   }

   void update( cache_bundle_type bundle, Value&& new_value )
   {
      std::vector< size_t > modified_indices;
      for ( auto& c : _index_caches )
         modified_indices.push_back( c.first );

      update( bundle, std::move( new_value ), modified_indices );
   }

   void update( cache_bundle_type bundle, Value&& new_value, const std::vector< size_t >& modified_indices )
   {
      // Invalidate the keys based on our old value
      for ( auto i : modified_indices )
         _index_caches[ i ]->invalidate( *( bundle.first ) );

      // Replace the value without changing our pointers
      *( bundle.first ) = std::move( new_value );

      // Generate new keys for each index based on the new value
      for ( auto i : modified_indices )
         _index_caches[ i ]->cache( bundle );

      cache_manager::get()->update( bundle.second );
   }

   void invalidate( const Value& v )
   {
      assert( _index_caches.begin() != _index_caches.end() );

      const auto& last_key = _index_caches.rbegin()->first;

      for ( auto& c : _index_caches )
      {
         if ( c.first == last_key )
            c.second->invalidate( v, true );
         else
            c.second->invalidate( v );
      }
   }

   void clear()
   {
      assert( _index_caches.begin() != _index_caches.end() );

      const auto& last_key = _index_caches.rbegin()->first;

      for ( auto& c : _index_caches )
      {
         if ( c.first == last_key )
            c.second->clear( true );
         else
            c.second->clear();
      }
   }

   size_t usage() const
   {
      // This assumes index 1 is the root index
      return _index_caches.find( 1 )->second->usage();
   }

   size_t size() const
   {
      return _index_caches.find( 1 )->second->size();
   }

   std::mutex& get_lock()
   {
      return _lock;
   }
};

template< typename Value, typename Key, typename KeyFromValue >
class index_cache : public abstract_index_cache< Value >
{
public:
   typedef typename std::shared_ptr< Value >                                ptr_type;
   typedef typename std::pair< ptr_type, lru_cache_manager::iterator_type > cache_bundle_type;

private:
   KeyFromValue                        _get_key;
   std::map< Key, cache_bundle_type >  _cache;

   const Key key( cache_key_type k )
   {
      return *( ( Key* )k );
   }

   virtual void cache( cache_bundle_type bundle ) override final
   {
      auto key = _get_key( *( bundle.first ) );
      auto r = _cache.insert( std::make_pair( key, bundle ) );
      assert( r.second == true );
      boost::ignore_unused( r );
   }

   virtual void invalidate( const Value& v ) override final
   {
      auto k = _get_key( v );
      auto n = _cache.erase( k );
      assert( n == 1 );
      boost::ignore_unused( n );
   }

   virtual void invalidate( const Value& v, bool update_cache_manager ) override final
   {
      auto k = _get_key( v );
      auto it = _cache.find( k );
      assert( it != _cache.end() );

      if ( update_cache_manager )
         cache_manager::get()->remove( it->second.second );

      _cache.erase( it );
   }

   virtual void clear() override final
   {
      _cache.clear();
   }

   virtual void clear( bool update_cache_manager ) override final
   {
      if ( update_cache_manager )
      {
         for ( auto& item : _cache )
            cache_manager::get()->remove( item.second.second );
      }

      clear();
   }

   virtual size_t usage() const override final
   {
      size_t cache_size = 0;
      for( const auto& entry : _cache )
      {
         cache_size += fc::raw::pack_size( *(entry.second.first) );
      }

      return cache_size;
   }

   virtual size_t size() const override final
   {
      return _cache.size();
   }


public:
   index_cache() = default;
   virtual ~index_cache() = default;

   virtual void update( cache_key_type k, Value&&v ) override final
   {
      assert( abstract_index_cache< Value >::_multi_index_cache_manager != nullptr );
      abstract_index_cache< Value >::_multi_index_cache_manager->update( _cache[ key( k ) ], std::move( v ) );
   }

   virtual void update( cache_key_type k, Value&& v, const std::vector< size_t >& modified_indices ) override final
   {
      assert( abstract_index_cache< Value >::_multi_index_cache_manager != nullptr );
      abstract_index_cache< Value >::_multi_index_cache_manager->update( _cache[ key( k ) ], std::move( v ), modified_indices );
   }

   virtual ptr_type get( cache_key_type k ) override final
   {
      auto itr = _cache.find( key( k ) );
      if ( itr != _cache.end() )
      {
         cache_manager::get()->update( itr->second.second );
         return itr->second.first;
      }
      return ptr_type();
   }

   virtual bool contains( cache_key_type k ) override final
   {
      return _cache.find( key( k ) ) != _cache.end();
   }

   virtual std::mutex& get_lock() override final
   {
      return abstract_index_cache< Value >::_multi_index_cache_manager->get_lock();
   }
};

template< typename Value >
struct cache_factory
{
   static std::shared_ptr< multi_index_cache_manager< Value > > get_shared_cache( bool reset = false )
   {
      static std::shared_ptr< multi_index_cache_manager< Value > > cache_ptr;

      if( !cache_ptr || reset )
         cache_ptr = std::make_shared< multi_index_cache_manager< Value > >();

      return cache_ptr;
   }

   static void reset()
   {
      get_shared_cache( true );
   }
};

} } } // mira::multi_index::detail
