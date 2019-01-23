#pragma once

#include <fc/log/logger.hpp>

#include <map>
#include <memory>

#include <iostream>

#define _unused(x) ((void)(x))
#define CACHE_KEY(key) ((void*)&key)

namespace mira { namespace multi_index { namespace detail {

typedef const void* cache_key_type;

template< typename Value >
struct cache_factory;

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

   virtual ptr_type get( cache_key_type key ) = 0;
   virtual void cache( ptr_type v ) = 0;
   virtual void update( cache_key_type key, Value&& v ) = 0;
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
   typedef typename std::shared_ptr< Value >                         ptr_type;
   typedef cache_factory< Value >                                    factory_type;

private:
   std::vector< index_cache_type > _index_caches;

public:
   void add_index_cache( index_cache_type&& index_cache )
   {
      index_cache->set_cache_manager( this->shared_from_this() );
      _index_caches.push_back( std::move( index_cache ) );
   }

   index_cache_type& get_index_cache( size_t index )
   {
      assert( index >= 1 );
      assert( index <= _index_caches.size() );
      return _index_caches[ index - 1 ];
   }

   ptr_type cache( Value&& value )
   {
      ptr_type p = std::make_shared< Value >( std::move( value ) );
      return cache( p );
   }

   ptr_type cache( ptr_type value )
   {
      for ( auto& c : _index_caches )
         c->cache( value );

      return value;
   }

   void update( ptr_type old_value, Value&& new_value )
   {
      // Invalidate the keys based on our old value
      for ( auto& c : _index_caches )
         c->invalidate( *old_value );

      // Replace the value without changing our pointers
      *old_value = std::move( new_value );

      // Generate new keys for each index based on the new value
      for ( auto& c : _index_caches )
         c->cache( old_value );
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
struct index_cache : abstract_index_cache< Value >
{
public:
   typedef typename std::shared_ptr< Value > ptr_type;

private:
   KeyFromValue               _get_key;
   std::map< Key, ptr_type >  _cache;

   const Key key( cache_key_type k )
   {
      return *( ( Key* )k );
   }

public:
   index_cache() = default;
   virtual ~index_cache() = default;

   void cache( ptr_type value )
   {
      auto key = _get_key( *value );
      auto r = _cache.insert( std::make_pair( key, value ) );
      assert( r.second == true );
      _unused( r );
   }

   void invalidate( const Value& v )
   {
      auto k = _get_key( v );
      auto n = _cache.erase( k );
      assert( n == 1 );
      _unused( n );
   }

   void update( cache_key_type k, Value&&v )
   {
      abstract_index_cache< Value >::cache_manager->update( _cache[ key( k ) ], std::move( v ) );
   }

   ptr_type get( cache_key_type k )
   {
      auto itr = _cache.find( key( k ) );
      return itr != _cache.end() ? itr->second : ptr_type();
   }

   void clear()
   {
      _cache.clear();
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
