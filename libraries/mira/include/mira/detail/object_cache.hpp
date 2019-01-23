#pragma once

#include <fc/log/logger.hpp>

#include <deque>
#include <map>
#include <memory>

#include <iostream>

namespace mira { namespace multi_index { namespace detail {

// Forward declaration
template < typename Value >
class multi_index_cache_manager;

template < typename Value >
struct abstract_index_cache
{
   abstract_index_cache() = default;
   virtual ~abstract_index_cache() = default;

   typedef typename std::shared_ptr< Value > ptr_type;

   virtual void set_cache_manager( std::shared_ptr< multi_index_cache_manager< Value > > m )
   {
      cache_manager = m;
   }

   virtual ptr_type get( const void* key ) = 0;
   virtual ptr_type cache( Value&& v ) = 0;
   virtual void update( const void* key, Value&& v ) = 0;
   virtual void update( Value&& v ) = 0;
   virtual void invalidate( const void* key ) = 0;
   virtual void invalidate( const Value& v ) = 0;
   virtual void clear() = 0;

protected:
   std::shared_ptr< multi_index_cache_manager< Value > > cache_manager;
};

template < typename Value >
class multi_index_cache_manager : public std::enable_shared_from_this< multi_index_cache_manager< Value > >
{
public:
   multi_index_cache_manager() = default;
   ~multi_index_cache_manager() = default;
   typedef typename std::unique_ptr< abstract_index_cache< Value > > index_cache_type;
   typedef typename std::shared_ptr< Value > ptr_type;

private:
   std::deque< index_cache_type > _index_caches;

public:
   void add_index_cache( index_cache_type&& index_cache )
   {
      index_cache->set_cache_manager( this->shared_from_this() );
      _index_caches.push_front( std::move( index_cache ) );
   }

   index_cache_type& get_index_cache( size_t index )
   {
      assert( index <= _index_caches.size() );
      return _index_caches[ index ];
   }

   ptr_type cache( Value&& v )
   {
      for ( auto& c : _index_caches )
         c->cache( v );
   }

   void update( Value&& v )
   {
      for ( auto& c : _index_caches )
         c->update( v );
   }

   void invalidate( const Value& v )
   {
      for ( auto& c : _index_caches )
         c->invalidate( v );
   }

   void clear()
   {
      for ( auto& c : _index_caches )
         c->clear();
   }
};

template< typename Value, typename Key, typename KeyFromValue >
struct object_cache_factory; // forward declaration

template< typename Value, typename Key, typename KeyFromValue >
struct object_cache : abstract_index_cache< Value >
{
public:
   typedef typename std::shared_ptr< Value > ptr_type;
   typedef object_cache_factory<
      Value,
      Key,
      KeyFromValue >                         factory_type;

private:
   KeyFromValue                              _get_key;
   std::map< Key, ptr_type >                 _cache;

   const Key key( const void* k )
   {
      return *( ( Key* )k );
   }

public:
   object_cache() = default;
   virtual ~object_cache() = default;

   ptr_type cache( Value&& v )
   {
      auto key = _get_key( v );
      auto value = std::make_shared< Value >( std::move( v ) );

      _cache.insert( std::make_pair( key, value ) );

      return value;
   }

   void invalidate( const Value& v )
   {
      invalidate( (void*)&_get_key( v ) );
   }

   void invalidate( const void* k )
   {
      _cache.erase( key( k ) );
   }

   void update( const void* k, Value&&v )
   {
      *( _cache[ key( k ) ] ) = std::move( v );
   }

   void update( Value&& v )
   {
      auto k = (void*)&_get_key( v );
      update( k, std::move( v ) );
   }

   ptr_type get( const void* k )
   {
      auto itr = _cache.find( key( k ) );
      return itr != _cache.end() ? itr->second : ptr_type();
   }

   void clear()
   {
      _cache.clear();
   }
};

template< typename Value, typename Key, typename KeyFromValue >
struct object_cache_factory
{
   static std::shared_ptr< object_cache< Value, Key, KeyFromValue > > get_shared_cache( bool reset = false )
   {
      static std::shared_ptr< object_cache< Value, Key, KeyFromValue > > cache_ptr;

      if( !cache_ptr || reset )
         cache_ptr = std::make_shared< object_cache< Value, Key, KeyFromValue > >();

      return cache_ptr;
   }

   static void reset()
   {
      get_shared_cache( true );
   }
};

} } } // mira::multi_index::detail
