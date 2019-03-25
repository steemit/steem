/* Multiply indexed container.
 *
 * Copyright 2003-2018 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#pragma once


#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <memory>
#include <boost/core/addressof.hpp>
#include <boost/detail/no_exceptions_support.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/filesystem.hpp>
#include <boost/move/core.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/find_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/deref.hpp>
#include <mira/multi_index_container_fwd.hpp>
#include <mira/detail/base_type.hpp>
#include <mira/detail/has_tag.hpp>
#include <mira/detail/no_duplicate_tags.hpp>
#include <mira/detail/object_cache.hpp>
#include <mira/slice_pack.hpp>
#include <mira/configuration.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/base_from_member.hpp>

#include <fc/io/raw.hpp>
#include <fc/log/logger.hpp>

#include <rocksdb/filter_policy.h>
#include <rocksdb/table.h>
#include <rocksdb/statistics.h>
#include <rocksdb/rate_limiter.h>
#include <rocksdb/convenience.h>

#include <iostream>

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
#include <initializer_list>
#endif

#define BOOST_MULTI_INDEX_CHECK_INVARIANT_OF(x)
#define BOOST_MULTI_INDEX_CHECK_INVARIANT

#define DEFAULT_COLUMN 0
#define MIRA_MAX_OPEN_FILES_PER_DB 64

#define ENTRY_COUNT_KEY "ENTRY_COUNT"
#define REVISION_KEY "REV"

namespace mira{

namespace multi_index{

#if BOOST_WORKAROUND(BOOST_MSVC,BOOST_TESTED_AT(1500))
#pragma warning(push)
#pragma warning(disable:4522) /* spurious warning on multiple operator=()'s */
#endif

template<typename Value,typename IndexSpecifierList,typename Allocator>
class multi_index_container:
  public detail::multi_index_base_type<
    Value,IndexSpecifierList,Allocator>::type
{

private:
  BOOST_COPYABLE_AND_MOVABLE(multi_index_container)

  template <typename,typename,typename> friend class  detail::index_base;

  typedef typename detail::multi_index_base_type<
      Value,IndexSpecifierList,Allocator>::type   super;

   int64_t                                         _revision = -1;

   std::string                                     _name;
   std::shared_ptr< ::rocksdb::Statistics >        _stats;
   ::rocksdb::WriteOptions                         _wopts;

   rocksdb::ReadOptions                            _ropts;

public:
  /* All types are inherited from super, a few are explicitly
   * brought forward here to save us some typename's.
   */

  typedef typename super::ctor_args_list           ctor_args_list;
  typedef IndexSpecifierList                       index_specifier_type_list;

  typedef typename super::index_type_list          index_type_list;

  typedef typename super::iterator_type_list       iterator_type_list;
  typedef typename super::const_iterator_type_list const_iterator_type_list;
  typedef typename super::value_type               value_type;
  typedef typename super::final_allocator_type     allocator_type;
  typedef typename super::iterator                 iterator;
  typedef typename super::const_iterator           const_iterator;

  typedef typename super::primary_index_type       primary_index_type;

  typedef typename primary_index_type::iterator    primary_iterator;

  static const size_t                              node_size = sizeof(value_type);

  BOOST_STATIC_ASSERT(
    detail::no_duplicate_tags_in_index_list<index_type_list>::value);

  /* global project() needs to see this publicly */

  /* construct/copy/destroy */

  multi_index_container( const allocator_type& al ):
    super(ctor_args_list()),
    entry_count(0)
   {
      std::vector< std::string > split_v;
      auto type = boost::core::demangle( typeid( Value ).name() );
      boost::split( split_v, type, boost::is_any_of( ":" ) );
      _wopts.disableWAL = true;

      _name = "rocksdb_" + *(split_v.rbegin());
   }

  explicit multi_index_container( const allocator_type& al, const boost::filesystem::path& p ):
    super(ctor_args_list()),
    entry_count(0)
   {
      std::vector< std::string > split_v;
      auto type = boost::core::demangle( typeid( Value ).name() );
      boost::split( split_v, type, boost::is_any_of( ":" ) );

      _name = "rocksdb_" + *(split_v.rbegin());
      _wopts.disableWAL = true;

      open( p );

      BOOST_MULTI_INDEX_CHECK_INVARIANT;
   }

   bool open( const boost::filesystem::path& p, const boost::any& cfg = nullptr )
   {
      assert( p.is_absolute() );

      std::string str_path = ( p / _name ).string();

      maybe_create_schema( str_path );

      // TODO: Move out of constructor becasuse throwing exceptions in a constuctor is sad...
      column_definitions column_defs;
      populate_column_definitions_( column_defs );

      ::rocksdb::Options opts;

      try
      {
         if ( configuration::gather_statistics( cfg ) )
            _stats = ::rocksdb::CreateDBStatistics();

         detail::cache_manager::get()->set_object_threshold( configuration::get_object_count( cfg ) );

         opts = configuration::get_options( cfg, boost::core::demangle( typeid( Value ).name() ) );
      }
      catch ( ... )
      {
         elog( "Failure while applying configuration for database: ${db}",
            ("db", boost::core::demangle( typeid( Value ).name())) );
         throw;
      }

      ::rocksdb::DB* db = nullptr;
      ::rocksdb::Status s = ::rocksdb::DB::Open( opts, str_path, column_defs, &(super::_handles), &db );

      if( s.ok() )
      {
         // Verify DB Schema

         super::_db.reset( db );

         ::rocksdb::ReadOptions read_opts;
         ::rocksdb::PinnableSlice value_slice;

         auto ser_count_key = fc::raw::pack_to_vector( ENTRY_COUNT_KEY );

         s = super::_db->Get(
            read_opts,
            super::_handles[0],
            ::rocksdb::Slice( ser_count_key.data(), ser_count_key.size() ),
            &value_slice );

         if( s.ok() )
         {
            entry_count = fc::raw::unpack_from_char_array< uint64_t >( value_slice.data(), value_slice.size() );
         }

         auto ser_rev_key = fc::raw::pack_to_vector( REVISION_KEY );
         value_slice.Reset();

         s = super::_db->Get(
            read_opts,
            super::_handles[0],
            ::rocksdb::Slice( ser_rev_key.data(), ser_rev_key.size() ),
            &value_slice );

         if( s.ok() )
         {
            _revision = fc::raw::unpack_from_char_array< int64_t >( value_slice.data(), value_slice.size() );
         }
      }
      else
      {
         std::cout << std::string( s.getState() ) << std::endl;
         return false;
      }

      super::cache_first_key();

      super::object_cache_factory_type::reset();
      return true;
   }

   void close()
   {
      if( super::_db )
      {
         auto ser_count_key = fc::raw::pack_to_vector( ENTRY_COUNT_KEY );
         auto ser_count_val = fc::raw::pack_to_vector( entry_count );

         super::_db->Put(
            _wopts,
            super::_handles[ DEFAULT_COLUMN ],
            ::rocksdb::Slice( ser_count_key.data(), ser_count_key.size() ),
            ::rocksdb::Slice( ser_count_val.data(), ser_count_val.size() ) );

         super::_cache->clear();
         assert( super::_db.unique() );
         rocksdb::CancelAllBackgroundWork( &(*super::_db), true );
         super::cleanup_column_handles();
         super::_db.reset();
      }
   }

   void wipe( const boost::filesystem::path& p )
   {
      assert( !(super::_db) );

      column_definitions column_defs;
      populate_column_definitions_( column_defs );

      auto s = rocksdb::DestroyDB( ( p / _name ).string(), rocksdb::Options(), column_defs );

      if( !s.ok() ) std::cout << std::string( s.getState() ) << std::endl;

      super::_cache->clear();
   }

   ~multi_index_container()
   {
      close();
   }

   void flush()
   {
      if( super::_db )
      {
         super::flush();
      }
   }

   void trim_cache()
   {
      detail::cache_manager::get()->adjust_capacity();
   }

  allocator_type get_allocator()const BOOST_NOEXCEPT
  {
    return allocator_type();
  }

  /* retrieval of indices by number */

  template<int N>
  struct nth_index
  {
    BOOST_STATIC_ASSERT(N>=0&&N<boost::mpl::size<index_type_list>::type::value);
    typedef typename boost::mpl::at_c<index_type_list,N>::type type;
  };

  template<int N>
  typename nth_index<N>::type& get()BOOST_NOEXCEPT
  {
    BOOST_STATIC_ASSERT(N>=0&&N<boost::mpl::size<index_type_list>::type::value);
    return *this;
  }

  template<int N>
  const typename nth_index<N>::type& get()const BOOST_NOEXCEPT
  {
    BOOST_STATIC_ASSERT(N>=0&&N<boost::mpl::size<index_type_list>::type::value);
    return *this;
  }

  /* retrieval of indices by tag */

  template<typename Tag>
  struct index
  {
    typedef typename boost::mpl::find_if<
      index_type_list,
      detail::has_tag<Tag>
    >::type                                    iter;

    BOOST_STATIC_CONSTANT(
      bool,index_found=!(boost::is_same<iter,typename boost::mpl::end<index_type_list>::type >::value));
    BOOST_STATIC_ASSERT(index_found);

    typedef typename boost::mpl::deref<iter>::type    type;
  };

  template<typename Tag>
  typename index<Tag>::type& get()BOOST_NOEXCEPT
  {
    return *this;
  }

  template<typename Tag>
  const typename index<Tag>::type& get()const BOOST_NOEXCEPT
  {
    return *this;
  }

  /* projection of iterators by number */
#if !defined(BOOST_NO_MEMBER_TEMPLATES)
  template<int N>
  struct nth_index_iterator
  {
    typedef typename nth_index<N>::type::iterator type;
  };

  template<int N>
  struct nth_index_const_iterator
  {
    typedef typename nth_index<N>::type::const_iterator type;
  };
#endif

  /* projection of iterators by tag */

#if !defined(BOOST_NO_MEMBER_TEMPLATES)
  template<typename Tag>
  struct index_iterator
  {
    typedef typename index<Tag>::type::iterator type;
  };

  template<typename Tag>
  struct index_const_iterator
  {
    typedef typename index<Tag>::type::const_iterator type;
  };
#endif

int64_t revision() { return _revision; }

int64_t set_revision( int64_t rev )
{
   const static auto ser_rev_key = fc::raw::pack_to_vector( REVISION_KEY );
   const static ::rocksdb::Slice rev_slice( ser_rev_key.data(), ser_rev_key.size() );
   auto ser_rev_val = fc::raw::pack_to_vector( rev );

   auto s = super::_db->Put(
      _wopts, rev_slice,
      ::rocksdb::Slice( ser_rev_val.data(), ser_rev_val.size() ) );

   if( s.ok() ) _revision = rev;

   return _revision;
}

void print_stats() const
{
   if( _stats )
   {
      std::cout << _name << " stats:\n";
      std::cout << _stats->ToString() << "\n";
   }
}

size_t get_cache_usage() const
{
   return super::_cache->usage();
}

size_t get_cache_size() const
{
   return super::_cache->size();
}

void dump_lb_call_counts()
{
   ilog( "Object ${s}:", ("s",_name) );
   super::dump_lb_call_counts();
   ilog( "" );
}

primary_iterator iterator_to( const value_type& x )
{
   return primary_index_type::iterator_to( x );
}

template< typename CompatibleKey >
primary_iterator find( const CompatibleKey& x )const
{
   return primary_index_type::find( x );
}

template< typename CompatibleKey >
primary_iterator lower_bound( const CompatibleKey& x )const
{
   return primary_index_type::lower_bound( x );
}

primary_iterator upper_bound( const typename primary_index_type::key_type& x )const
{
   return primary_index_type::upper_bound( x );
}

template< typename CompatibleKey >
primary_iterator upper_bound( const CompatibleKey& x )const
{
   return primary_index_type::upper_bound( x );
}

template< typename LowerBounder >
std::pair< primary_iterator, primary_iterator >
range( LowerBounder& lower, const typename primary_index_type::key_type upper )const
{
   return primary_index_type::range( lower, upper );
}

typename primary_index_type::iterator begin() BOOST_NOEXCEPT
   { return primary_index_type::begin(); }

typename primary_index_type::const_iterator begin()const BOOST_NOEXCEPT
   { return primary_index_type::begin(); }

typename primary_index_type::iterator end() BOOST_NOEXCEPT
   { return primary_index_type::end(); }

typename primary_index_type::const_iterator end()const BOOST_NOEXCEPT
   { return primary_index_type::end(); }

typename primary_index_type::reverse_iterator rbegin() BOOST_NOEXCEPT
   { return primary_index_type::rbegin(); }

typename primary_index_type::const_reverse_iterator rbegin() const BOOST_NOEXCEPT
   { return primary_index_type::rbegin(); }

typename primary_index_type::reverse_iterator rend() BOOST_NOEXCEPT
   { return primary_index_type::rend(); }

typename primary_index_type::const_reverse_iterator rend() const BOOST_NOEXCEPT
   { return primary_index_type::rend(); }

typename primary_index_type::const_iterator cbegin()const BOOST_NOEXCEPT
   { return primary_index_type::cbegin(); }

typename primary_index_type::const_iterator cend()const BOOST_NOEXCEPT
   { return primary_index_type::cend(); }

typename primary_index_type::const_reverse_iterator crbegin()const BOOST_NOEXCEPT
   { return primary_index_type::crbegin(); }

typename primary_index_type::const_reverse_iterator crend()const BOOST_NOEXCEPT
   { return primary_index_type::crend(); }

template< typename Modifier >
bool modify( primary_iterator position, Modifier mod )
{
   return primary_index_type::modify( position, mod );
}

primary_iterator erase( primary_iterator position )
{
   return primary_index_type::erase( position );
}

  bool empty_()const
  {
    return entry_count==0;
  }

  uint64_t size_()const
  {
    return entry_count;
  }

  uint64_t max_size_()const
  {
    return static_cast<uint64_t >(-1);
  }

   template< typename... Args >
   bool emplace_rocksdb_( Args&&... args )
   {
      Value v( std::forward< Args >(args)... );
      bool status = false;
      if( super::insert_rocksdb_( v ) )
      {
         auto retval = super::_db->Write( _wopts, super::_write_buffer.GetWriteBatch() );
         status = retval.ok();
         if( status )
         {
            ++entry_count;
            super::commit_first_key_update();
         }
         else
         {
            elog( "${e}", ("e", retval.ToString()) );
            super::reset_first_key_update();
         }
      }
      else
      {
         super::reset_first_key_update();
      }
      super::_write_buffer.Clear();

      return status;
   }

   void erase_( value_type& v )
   {
      super::erase_( v );
      auto retval = super::_db->Write( _wopts, super::_write_buffer.GetWriteBatch() );
      bool status = retval.ok();
      if( status )
      {
         --entry_count;
         std::lock_guard< std::mutex > lock( super::_cache->get_lock() );
         super::_cache->invalidate( v );
         super::commit_first_key_update();
      }
      else
      {
         elog( "${e}", ("e", retval.ToString()) );
         super::reset_first_key_update();
      }

      super::_write_buffer.Clear();
   }

  void clear_()
  {
    super::clear_();
    super::_cache->clear();
    entry_count=0;
  }

   template< typename Modifier >
   bool modify_( Modifier& mod, value_type& v )
   {
      bool status = false;
      std::vector< size_t > modified_indices;
      if( super::modify_( mod, v, modified_indices ) )
      {
         auto retval = super::_db->Write( _wopts, super::_write_buffer.GetWriteBatch() );
         status = retval.ok();

         if( status )
         {
            /* This gets a little weird because the reference passed in
            most likely belongs to the shared ptr in the cache, so updating
            the value has already updated the cache, but in case something
            doesn't line up here, we update by moving the value to itself... */
            std::lock_guard< std::mutex > lock( super::_cache->get_lock() );
            super::_cache->get_index_cache( ID_INDEX )->update( (void*)&super::id( v ), std::move( v ), modified_indices );
            super::commit_first_key_update();
         }
         else
         {
            elog( "${e}", ("e", retval.ToString()) );
            super::reset_first_key_update();
         }
      }
      else
      {
         super::reset_first_key_update();
      }

      super::_write_buffer.Clear();

      return status;
   }

   template< typename MetaKey, typename MetaValue >
   bool get_metadata( const MetaKey& k, MetaValue& v )
   {
      if( !super::_db ) return false;

      rocksdb::PinnableSlice key_slice, value_slice;

      pack_to_slice( key_slice, k );

      auto status = super::_db->Get(
         _ropts,
         super::_handles[ 0 ],
         key_slice,
         &value_slice );

      if( status.ok() )
      {
         unpack_from_slice( value_slice, v );
         //ilog( "Retrieved metdata for ${type}: ${key},${value}", ("type",boost::core::demangle(typeid(Value).name()))("key",k)("value",v) );
      }
      else
      {
         //ilog( "Failed to retrieve metadata for ${type}: ${key}", ("type",boost::core::demangle(typeid(Value).name()))("key",k) );
      }

      return status.ok();
   }

   template< typename MetaKey, typename MetaValue >
   bool put_metadata( const MetaKey& k, const MetaValue& v )
   {
      if( !super::_db ) return false;

      rocksdb::PinnableSlice key_slice, value_slice;
      pack_to_slice( key_slice, k );
      pack_to_slice( value_slice, v );

      auto status = super::_db->Put(
         _wopts,
         super::_handles[0],
         key_slice,
         value_slice );

      if( status.ok() )
      {
         //ilog( "Stored metdata for ${type}: ${key},${value}", ("type",boost::core::demangle(typeid(Value).name()))("key",k)("value",v) );
      }
      else
      {
         //ilog( "Failed to store metadata for ${type}: ${key},${value}", ("type",boost::core::demangle(typeid(Value).name()))("key",k)("value",v) );
      }

      return status.ok();
   }

private:
   uint64_t entry_count;

   size_t get_column_size() const { return super::COLUMN_INDEX; }

   void populate_column_definitions_( column_definitions& defs ) const
   {
      super::populate_column_definitions_( defs );
   }

   bool maybe_create_schema( const std::string& str_path )
   {
      ::rocksdb::DB* db = nullptr;

      column_definitions column_defs;
      populate_column_definitions_( column_defs );

      ::rocksdb::Options opts;
      //opts.IncreaseParallelism();
      //opts.OptimizeLevelStyleCompaction();
      opts.max_open_files = MIRA_MAX_OPEN_FILES_PER_DB;

      ::rocksdb::Status s = ::rocksdb::DB::OpenForReadOnly( opts, str_path, column_defs, &(super::_handles), &db );

      if( s.ok() )
      {
         super::cleanup_column_handles();
         delete db;
         return true;
      }

      opts.create_if_missing = true;

      s = ::rocksdb::DB::Open( opts, str_path, &db );

      if( s.ok() )
      {
         column_defs.clear();
         populate_column_definitions_( column_defs );
         column_defs.erase( column_defs.begin() );

         s = db->CreateColumnFamilies( column_defs, &(super::_handles) );

         if( s.ok() )
         {
            // Create default column keys

            auto ser_count_key = fc::raw::pack_to_vector( ENTRY_COUNT_KEY );
            auto ser_count_val = fc::raw::pack_to_vector( uint64_t(0) );

            s = db->Put(
               _wopts,
               db->DefaultColumnFamily(),
               ::rocksdb::Slice( ser_count_key.data(), ser_count_key.size() ),
               ::rocksdb::Slice( ser_count_val.data(), ser_count_val.size() ) );

            if( !s.ok() ) return false;

            auto ser_rev_key = fc::raw::pack_to_vector( REVISION_KEY );
            auto ser_rev_val = fc::raw::pack_to_vector( int64_t(0) );

            db->Put(
               _wopts,
               db->DefaultColumnFamily(),
               ::rocksdb::Slice( ser_rev_key.data(), ser_rev_key.size() ),
               ::rocksdb::Slice( ser_rev_val.data(), ser_rev_val.size() ) );

            if( !s.ok() ) return false;

            // Save schema info

            super::cleanup_column_handles();
         }
         else
         {
            std::cout << std::string( s.getState() ) << std::endl;
         }

         delete db;

         return true;
      }
      else
      {
         std::cout << std::string( s.getState() ) << std::endl;
      }

      return false;
   }

#if defined(BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING)&&\
    BOOST_WORKAROUND(__MWERKS__,<=0x3003)
#pragma parse_mfunc_templ reset
#endif
};

#if BOOST_WORKAROUND(BOOST_MSVC,BOOST_TESTED_AT(1500))
#pragma warning(pop) /* C4522 */
#endif

/* retrieval of indices by number */

template<typename MultiIndexContainer,int N>
struct nth_index
{
  BOOST_STATIC_CONSTANT(
    int,
    M=boost::mpl::size<typename MultiIndexContainer::index_type_list>::type::value);
  BOOST_STATIC_ASSERT(N>=0&&N<M);
  typedef typename boost::mpl::at_c<
    typename MultiIndexContainer::index_type_list,N>::type type;
};

/* retrieval of indices by tag */

template<typename MultiIndexContainer,typename Tag>
struct index
{
  typedef typename MultiIndexContainer::index_type_list index_type_list;

  typedef typename boost::mpl::find_if<
    index_type_list,
    detail::has_tag<Tag>
  >::type                                      iter;

  BOOST_STATIC_CONSTANT(
    bool,index_found=!(boost::is_same<iter,typename boost::mpl::end<index_type_list>::type >::value));
  BOOST_STATIC_ASSERT(index_found);

  typedef typename boost::mpl::deref<iter>::type       type;
};

/* projection of iterators by number */

template<typename MultiIndexContainer,int N>
struct nth_index_iterator
{
  typedef typename nth_index<MultiIndexContainer,N>::type::iterator type;
};

template<typename MultiIndexContainer,int N>
struct nth_index_const_iterator
{
  typedef typename nth_index<MultiIndexContainer,N>::type::const_iterator type;
};

/* projection of iterators by tag */

template<typename MultiIndexContainer,typename Tag>
struct index_iterator
{
  typedef typename ::mira::multi_index::index<
    MultiIndexContainer,Tag>::type::iterator    type;
};

template<typename MultiIndexContainer,typename Tag>
struct index_const_iterator
{
  typedef typename ::mira::multi_index::index<
    MultiIndexContainer,Tag>::type::const_iterator type;
};

} /* namespace multi_index */

/* Associated global functions are promoted to namespace boost, except
 * comparison operators and swap, which are meant to be Koenig looked-up.
 */

} /* namespace mira */

#undef BOOST_MULTI_INDEX_CHECK_INVARIANT
#undef BOOST_MULTI_INDEX_CHECK_INVARIANT_OF
