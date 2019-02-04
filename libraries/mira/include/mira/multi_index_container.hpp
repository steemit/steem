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
#include <boost/detail/allocator_utilities.hpp>
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
#include <mira/detail/access_specifier.hpp>
#include <mira/detail/adl_swap.hpp>
#include <mira/detail/base_type.hpp>
#include <mira/detail/do_not_copy_elements_tag.hpp>
#include <mira/detail/converter.hpp>
#include <mira/detail/header_holder.hpp>
#include <mira/detail/has_tag.hpp>
#include <mira/detail/no_duplicate_tags.hpp>
#include <mira/detail/object_cache.hpp>
#include <mira/detail/safe_mode.hpp>
#include <mira/detail/scope_guard.hpp>
#include <mira/slice_pack.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/multi_index/detail/vartempl_support.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/base_from_member.hpp>

#include <fc/io/raw.hpp>

#include <rocksdb/filter_policy.h>
#include <rocksdb/table.h>
#include <rocksdb/statistics.h>

#include <iostream>

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
#include <initializer_list>
#endif

#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
#include <mira/detail/archive_constructed.hpp>
#include <mira/detail/serialization_version.hpp>
#include <boost/serialization/collection_size_type.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
#include <boost/throw_exception.hpp>
#endif

#if defined(BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING)
#include <boost/multi_index/detail/invariant_assert.hpp>
#define BOOST_MULTI_INDEX_CHECK_INVARIANT_OF(x)                              \
  detail::scope_guard BOOST_JOIN(check_invariant_,__LINE__)=                 \
    detail::make_obj_guard(x,&multi_index_container::check_invariant_);      \
  BOOST_JOIN(check_invariant_,__LINE__).touch();
#define BOOST_MULTI_INDEX_CHECK_INVARIANT                                    \
  BOOST_MULTI_INDEX_CHECK_INVARIANT_OF(*this)
#else
#define BOOST_MULTI_INDEX_CHECK_INVARIANT_OF(x)
#define BOOST_MULTI_INDEX_CHECK_INVARIANT
#endif

#define DEFAULT_COLUMN 0
#define MIRA_MAX_OPEN_FILES_PER_DB 64
#define MIRA_SHARED_CACHE_SIZE (1ull * 1024 * 1024 * 1024 ) /* 4G */

#define ENTRY_COUNT_KEY "ENTRY_COUNT"
#define REVISION_KEY "REV"

namespace mira{

namespace multi_index{

#if BOOST_WORKAROUND(BOOST_MSVC,BOOST_TESTED_AT(1500))
#pragma warning(push)
#pragma warning(disable:4522) /* spurious warning on multiple operator=()'s */
#endif

struct rocksdb_options_factory
{
   static std::shared_ptr< rocksdb::Cache > get_shared_cache()
   {
      static std::shared_ptr< rocksdb::Cache > cache = rocksdb::NewLRUCache( MIRA_SHARED_CACHE_SIZE, 4 );
      return cache;
   }

   static std::shared_ptr< rocksdb::Statistics > get_shared_stats()
   {
      static std::shared_ptr< rocksdb::Statistics > cache = ::rocksdb::CreateDBStatistics();;
      return cache;
   }
};

template<typename Value,typename IndexSpecifierList,typename Allocator>
class multi_index_container:
  private boost::base_from_member<
    typename boost::detail::allocator::rebind_to<
      Allocator,
      typename detail::multi_index_node_type<
        Value,IndexSpecifierList,Allocator>::type
    >::type>,
  BOOST_MULTI_INDEX_PRIVATE_IF_MEMBER_TEMPLATE_FRIENDS detail::header_holder<
    typename std::allocator_traits<
      typename boost::detail::allocator::rebind_to<
        Allocator,
        typename detail::multi_index_node_type<
          Value,IndexSpecifierList,Allocator>::type
      >::type
    >::pointer,
    multi_index_container<Value,IndexSpecifierList,Allocator> >,
  public detail::multi_index_base_type<
    Value,IndexSpecifierList,Allocator>::type
{

private:
  BOOST_COPYABLE_AND_MOVABLE(multi_index_container)

  template <typename,typename,typename> friend class  detail::index_base;
  template <typename,typename>          friend struct detail::header_holder;
  template <typename,typename>          friend struct detail::converter;

  typedef typename detail::multi_index_base_type<
      Value,IndexSpecifierList,Allocator>::type   super;
  typedef typename
  boost::detail::allocator::rebind_to<
    Allocator,
    typename super::node_type
  >::type                                         node_allocator;
#ifdef BOOST_NO_CXX11_ALLOCATOR
  typedef typename node_allocator::pointer        node_pointer;
#else
  typedef std::allocator_traits<node_allocator>   node_allocator_traits;
  typedef typename node_allocator_traits::pointer node_pointer;
#endif
  typedef boost::base_from_member<
    node_allocator>                               bfm_allocator;
  typedef detail::header_holder<
    node_pointer,
    multi_index_container>                        bfm_header;

   int64_t                                         _revision = -1;

   std::string                                     _name;
   std::shared_ptr< ::rocksdb::Statistics >        _stats;
   ::rocksdb::WriteOptions                         _wopts;

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

  BOOST_STATIC_ASSERT(
    detail::no_duplicate_tags_in_index_list<index_type_list>::value);

  /* global project() needs to see this publicly */

  typedef typename super::node_type node_type;

  /* construct/copy/destroy */

  multi_index_container( const allocator_type& al ):
    bfm_allocator(al),
    super(ctor_args_list(),bfm_allocator::member),
    entry_count(0)
   {
      std::vector< std::string > split_v;
      auto type = boost::core::demangle( typeid( Value ).name() );
      boost::split( split_v, type, boost::is_any_of( ":" ) );
      _wopts.disableWAL = true;

      _name = "rocksdb_" + *(split_v.rbegin());
   }

  explicit multi_index_container( const allocator_type& al, const boost::filesystem::path& p ):
    bfm_allocator(al),
    super(ctor_args_list(),bfm_allocator::member),
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

   bool open( const boost::filesystem::path& p )
   {
      assert( p.is_absolute() );

      std::string str_path = ( p / _name ).string();

      maybe_create_schema( str_path );

      // TODO: Move out of constructor becasuse throwing exceptions in a constuctor is sad...
      column_definitions column_defs;
      populate_column_definitions_( column_defs );

//      _stats = rocksdb_options_factory::get_shared_stats();

      ::rocksdb::Options opts;
//
      opts.OptimizeLevelStyleCompaction();
//      opts.OptimizeUniversalStyleCompaction( 4 << 20 );
      opts.IncreaseParallelism();
//      opts.max_open_files = MIRA_MAX_OPEN_FILES_PER_DB;
      //opts.compression = rocksdb::CompressionType::kNoCompression;

//      opts.statistics = _stats;
//      opts.stats_dump_period_sec = 20;
//*
//      //opts.block_size = 8 << 10; //8K
//      //opts.cache_size = 4 << 30; // 4G
//      opts.write_buffer_size = 4 << 10; // 64K
//      opts.max_write_buffer_number = 1;
//      //opts.min_write_buffer_number_to_merge = 4;
//      opts.max_bytes_for_level_base = 2 << 30; // 3G
//      opts.max_bytes_for_level_multiplier = 5;
//      opts.target_file_size_base = 128 << 20; // 20M
//      opts.target_file_size_multiplier = 1;
//      opts.max_background_jobs = 32;
//      opts.max_background_flushes = 1;
//      opts.max_background_compactions = 8;
//      opts.level0_file_num_compaction_trigger = 2;
//      opts.level0_slowdown_writes_trigger = 24;
//      opts.level0_stop_writes_trigger = 56;
//      //opts.cache_numshardbits = 6;
//      opts.table_cache_numshardbits = 0;
//      opts.allow_mmap_reads = 1;
//      opts.allow_mmap_writes = 0;
//      opts.use_fsync = false;
//      opts.use_adaptive_mutex = false;
//      opts.bytes_per_sync = 2 << 20; // 2M
//      //opts.source_compaction_factor = 1;
//      //opts.max_grandparent_overlap_factor = 5;
//*/

      ::rocksdb::BlockBasedTableOptions table_options;
      table_options.block_size = 8 << 10; // 8K
      table_options.block_cache = rocksdb_options_factory::get_shared_cache();
      table_options.filter_policy.reset( rocksdb::NewBloomFilterPolicy( 10, false ) );
      opts.table_factory.reset( ::rocksdb::NewBlockBasedTableFactory( table_options ) );

      opts.allow_mmap_reads = true;

      opts.write_buffer_size = 2048 * 1024;              // 128k
      opts.max_bytes_for_level_base = 5 * 1024 * 1024;  // 1MB
      opts.target_file_size_base = 100 * 1024;          // 100k
      opts.max_write_buffer_number = 16;
      opts.max_background_compactions = 16;
      opts.max_background_flushes = 16;
      opts.min_write_buffer_number_to_merge = 8;

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

      super::object_cache_factory_type::reset();
      return true;
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

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
  /* As per http://www.boost.org/doc/html/move/emulation_limitations.html
   * #move.emulation_limitations.assignment_operator
   */

  multi_index_container<Value,IndexSpecifierList,Allocator>& operator=(
    const multi_index_container<Value,IndexSpecifierList,Allocator>& x)
  {
    multi_index_container y(x);
    this->swap(y);
    return *this;
  }
#endif

  multi_index_container<Value,IndexSpecifierList,Allocator>& operator=(
    BOOST_COPY_ASSIGN_REF(multi_index_container) x)
  {
    multi_index_container y(x);
    this->swap(y);
    return *this;
  }

  multi_index_container<Value,IndexSpecifierList,Allocator>& operator=(
    BOOST_RV_REF(multi_index_container) x)
  {
    this->swap(x);
    return *this;
  }

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
/*
  multi_index_container<Value,IndexSpecifierList,Allocator>& operator=(
    std::initializer_list<Value> list)
  {
    BOOST_MULTI_INDEX_CHECK_INVARIANT;
    typedef const Value* init_iterator;

    multi_index_container x(*this,detail::do_not_copy_elements_tag());
    iterator hint=x.end();
    for(init_iterator first=list.begin(),last=list.end();
        first!=last;++first){
      hint=x.make_iterator(x.insert_(*first,hint.get_node()).first);
      ++hint;
    }
    x.swap_elements_(*this);
    return*this;
  }
*/
#endif

  allocator_type get_allocator()const BOOST_NOEXCEPT
  {
    return allocator_type(bfm_allocator::member);
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

  template<int N,typename IteratorType>
  typename nth_index_iterator<N>::type project(IteratorType it)
  {
    typedef typename nth_index<N>::type index_type;

#if !defined(__SUNPRO_CC)||!(__SUNPRO_CC<0x580) /* fails in Sun C++ 5.7 */
    BOOST_STATIC_ASSERT(
      (boost::mpl::contains<iterator_type_list,IteratorType>::value));
#endif

    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(it);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(
      it,static_cast<typename IteratorType::container_type&>(*this));

    return index_type::make_iterator(static_cast<node_type*>(it.get_node()));
  }

  template<int N,typename IteratorType>
  typename nth_index_const_iterator<N>::type project(IteratorType it)const
  {
    typedef typename nth_index<N>::type index_type;

#if !defined(__SUNPRO_CC)||!(__SUNPRO_CC<0x580) /* fails in Sun C++ 5.7 */
    BOOST_STATIC_ASSERT((
      boost::mpl::contains<iterator_type_list,IteratorType>::value||
      boost::mpl::contains<const_iterator_type_list,IteratorType>::value));
#endif

    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(it);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(
      it,static_cast<const typename IteratorType::container_type&>(*this));
    return index_type::make_iterator(static_cast<node_type*>(it.get_node()));
  }
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

  template<typename Tag,typename IteratorType>
  typename index_iterator<Tag>::type project(IteratorType it)
  {
    typedef typename index<Tag>::type index_type;

#if !defined(__SUNPRO_CC)||!(__SUNPRO_CC<0x580) /* fails in Sun C++ 5.7 */
    BOOST_STATIC_ASSERT(
      (boost::mpl::contains<iterator_type_list,IteratorType>::value));
#endif

    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(it);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(
      it,static_cast<typename IteratorType::container_type&>(*this));
    return index_type::make_iterator(static_cast<node_type*>(it.get_node()));
  }

  template<typename Tag,typename IteratorType>
  typename index_const_iterator<Tag>::type project(IteratorType it)const
  {
    typedef typename index<Tag>::type index_type;

#if !defined(__SUNPRO_CC)||!(__SUNPRO_CC<0x580) /* fails in Sun C++ 5.7 */
    BOOST_STATIC_ASSERT((
      boost::mpl::contains<iterator_type_list,IteratorType>::value||
      boost::mpl::contains<const_iterator_type_list,IteratorType>::value));
#endif

    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(it);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(
      it,static_cast<const typename IteratorType::container_type&>(*this));
    return index_type::make_iterator(static_cast<node_type*>(it.get_node()));
  }
#endif

size_t get_column_size() const { return super::COLUMN_INDEX; }

void populate_column_definitions_( column_definitions& defs )const
{
   super::populate_column_definitions_( defs );
}

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

BOOST_MULTI_INDEX_PROTECTED_IF_MEMBER_TEMPLATE_FRIENDS:
  typedef typename super::copy_map_type copy_map_type;

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
  multi_index_container(
    const multi_index_container<Value,IndexSpecifierList,Allocator>& x,
    detail::do_not_copy_elements_tag):
    bfm_allocator(x.bfm_allocator::member),
    bfm_header(),
    super(x,detail::do_not_copy_elements_tag()),
    entry_count(0)
  {
    BOOST_MULTI_INDEX_CHECK_INVARIANT;
  }
#endif

  node_type* header()const
  {
    return &*bfm_header::member;
  }

  node_type* allocate_node()
  {
#ifdef BOOST_NO_CXX11_ALLOCATOR
    return &*bfm_allocator::member.allocate(1);
#else
    return &*node_allocator_traits::allocate(bfm_allocator::member,1);
#endif
  }

  void deallocate_node(node_type* x)
  {
#ifdef BOOST_NO_CXX11_ALLOCATOR
    bfm_allocator::member.deallocate(static_cast<node_pointer>(x),1);
#else
    node_allocator_traits::deallocate(bfm_allocator::member,static_cast<node_pointer>(x),1);
#endif
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

  template<typename Variant>
  std::pair<node_type*,bool> insert_(const Value& v,Variant variant)
  {
    node_type* x=0;
    node_type* res=super::insert_(v,x,variant);
    if(res==x){
      ++entry_count;
      return std::pair<node_type*,bool>(res,true);
    }
    else{
      return std::pair<node_type*,bool>(res,false);
    }
  }

  std::pair<node_type*,bool> insert_(const Value& v)
  {
    return insert_(v,detail::lvalue_tag());
  }

  std::pair<node_type*,bool> insert_rv_(const Value& v)
  {
    return insert_(v,detail::rvalue_tag());
  }

  template<typename T>
  std::pair<node_type*,bool> insert_ref_(T& t)
  {
    node_type* x=allocate_node();
    BOOST_TRY{
      new(boost::addressof(x->value())) value_type(t);
      BOOST_TRY{
        node_type* res=super::insert_(x->value(),x,detail::emplaced_tag());
        if(res==x){
          ++entry_count;
          return std::pair<node_type*,bool>(res,true);
        }
        else{
          boost::detail::allocator::destroy(boost::addressof(x->value()));
          deallocate_node(x);
          return std::pair<node_type*,bool>(res,false);
        }
      }
      BOOST_CATCH(...){
        boost::detail::allocator::destroy(boost::addressof(x->value()));
        BOOST_RETHROW;
      }
      BOOST_CATCH_END
    }
    BOOST_CATCH(...){
      deallocate_node(x);
      BOOST_RETHROW;
    }
    BOOST_CATCH_END
  }

  std::pair<node_type*,bool> insert_ref_(const value_type& x)
  {
    return insert_(x);
  }

  std::pair<node_type*,bool> insert_ref_(value_type& x)
  {
    return insert_(x);
  }

   template<BOOST_MULTI_INDEX_TEMPLATE_PARAM_PACK>
   std::pair<node_type*,bool> emplace_(
      BOOST_MULTI_INDEX_FUNCTION_PARAM_PACK)
   {
    node_type* x=allocate_node();
    BOOST_TRY{
      boost::multi_index::detail::vartempl_placement_new(
        boost::addressof(x->value()),BOOST_MULTI_INDEX_FORWARD_PARAM_PACK);
      BOOST_TRY{
        node_type* res=super::insert_(x->value(),x,detail::emplaced_tag());
        if(res==x){
          ++entry_count;
          return std::pair<node_type*,bool>(res,true);
        }
        else{
          boost::detail::allocator::destroy(boost::addressof(x->value()));
          deallocate_node(x);
          return std::pair<node_type*,bool>(res,false);
        }
      }
      BOOST_CATCH(...){
        boost::detail::allocator::destroy(boost::addressof(x->value()));
        BOOST_RETHROW;
      }
      BOOST_CATCH_END
    }
    BOOST_CATCH(...){
      deallocate_node(x);
      BOOST_RETHROW;
    }
    BOOST_CATCH_END
  }

   template< typename... Args >
   bool emplace_rocksdb_( Args&&... args )
   {
      Value v( std::forward< Args >(args)... );
      bool status = false;
      if( super::insert_rocksdb_( v ) )
      {
         status = super::_db->Write( _wopts, super::_write_buffer.GetWriteBatch()).ok();
         ++entry_count;
      }
      super::_write_buffer.Clear();

      return status;
   }

  template<typename Variant>
  std::pair<node_type*,bool> insert_(
    const Value& v,node_type* position,Variant variant)
  {
    node_type* x=0;
    node_type* res=super::insert_(v,position,x,variant);
    if(res==x){
      ++entry_count;
      return std::pair<node_type*,bool>(res,true);
    }
    else{
      return std::pair<node_type*,bool>(res,false);
    }
  }

  std::pair<node_type*,bool> insert_(const Value& v,node_type* position)
  {
    return insert_(v,position,detail::lvalue_tag());
  }

  std::pair<node_type*,bool> insert_rv_(const Value& v,node_type* position)
  {
    return insert_(v,position,detail::rvalue_tag());
  }

  template<typename T>
  std::pair<node_type*,bool> insert_ref_(
    T& t,node_type* position)
  {
    node_type* x=allocate_node();
    BOOST_TRY{
      new(boost::addressof(x->value())) value_type(t);
      BOOST_TRY{
        node_type* res=super::insert_(
          x->value(),position,x,detail::emplaced_tag());
        if(res==x){
          ++entry_count;
          return std::pair<node_type*,bool>(res,true);
        }
        else{
          boost::detail::allocator::destroy(boost::addressof(x->value()));
          deallocate_node(x);
          return std::pair<node_type*,bool>(res,false);
        }
      }
      BOOST_CATCH(...){
        boost::detail::allocator::destroy(boost::addressof(x->value()));
        BOOST_RETHROW;
      }
      BOOST_CATCH_END
    }
    BOOST_CATCH(...){
      deallocate_node(x);
      BOOST_RETHROW;
    }
    BOOST_CATCH_END
  }

  std::pair<node_type*,bool> insert_ref_(
    const value_type& x,node_type* position)
  {
    return insert_(x,position);
  }

  std::pair<node_type*,bool> insert_ref_(
    value_type& x,node_type* position)
  {
    return insert_(x,position);
  }

  template<BOOST_MULTI_INDEX_TEMPLATE_PARAM_PACK>
  std::pair<node_type*,bool> emplace_hint_(
    node_type* position,
    BOOST_MULTI_INDEX_FUNCTION_PARAM_PACK)
  {
    node_type* x=allocate_node();
    BOOST_TRY{
      boost::multi_index::detail::vartempl_placement_new(
        boost::addressof(x->value()),BOOST_MULTI_INDEX_FORWARD_PARAM_PACK);
      BOOST_TRY{
        node_type* res=super::insert_(
          x->value(),position,x,detail::emplaced_tag());
        if(res==x){
          ++entry_count;
          return std::pair<node_type*,bool>(res,true);
        }
        else{
          boost::detail::allocator::destroy(boost::addressof(x->value()));
          deallocate_node(x);
          return std::pair<node_type*,bool>(res,false);
        }
      }
      BOOST_CATCH(...){
        boost::detail::allocator::destroy(boost::addressof(x->value()));
        BOOST_RETHROW;
      }
      BOOST_CATCH_END
    }
    BOOST_CATCH(...){
      deallocate_node(x);
      BOOST_RETHROW;
    }
    BOOST_CATCH_END
  }

/*
  void erase_(node_type* x)
  {
    --entry_count;
    super::erase_(x);
    deallocate_node(x);
  }
*/

   void erase_( value_type& v )
   {
      super::erase_( v );
      super::_db->Write( _wopts, super::_write_buffer.GetWriteBatch() );
      --entry_count;
      super::_cache->invalidate( v );
      super::_write_buffer.Clear();
   }

  void delete_node_(node_type* x)
  {
    super::delete_node_(x);
    deallocate_node(x);
  }
/*
  void delete_all_nodes_()
  {
    super::delete_all_nodes_();
  }
*/
  void clear_()
  {
    super::clear_();
    super::_cache->clear();
    entry_count=0;
  }

  void swap_(multi_index_container<Value,IndexSpecifierList,Allocator>& x)
  {
    if(bfm_allocator::member!=x.bfm_allocator::member){
      detail::adl_swap(bfm_allocator::member,x.bfm_allocator::member);
    }
    std::swap(bfm_header::member,x.bfm_header::member);
    super::swap_(x);
    std::swap(entry_count,x.entry_count);
  }

  void swap_elements_(
    multi_index_container<Value,IndexSpecifierList,Allocator>& x)
  {
    std::swap(bfm_header::member,x.bfm_header::member);
    super::swap_elements_(x);
    std::swap(entry_count,x.entry_count);
  }

  bool replace_(const Value& k,node_type* x)
  {
    return super::replace_(k,x,detail::lvalue_tag());
  }

  bool replace_rv_(const Value& k,node_type* x)
  {
    return super::replace_(k,x,detail::rvalue_tag());
  }

   template< typename Modifier >
   bool modify_( Modifier& mod, value_type& v )
   {
      bool status = false;
      std::vector< size_t > modified_indices;
      if( super::modify_( mod, v, modified_indices ) )
      {
         status = super::_db->Write( _wopts, super::_write_buffer.GetWriteBatch() ).ok();

         if( status )
         {
            /* This gets a little weird because the reference passed in
            most likely belongs to the shared ptr in the cache, so updating
            the value has already updated the cache, but in case something
            doesn't line up here, we update by moving the value to itself... */
            super::_cache->get_index_cache( ID_INDEX )->update( (void*)&super::id( v ), std::move( v ), modified_indices );
         }
      }
      super::_write_buffer.Clear();

      return status;
   }


/*
  template<typename Modifier>
  bool modify_(Modifier& mod,node_type* x)
  {
    BOOST_TRY{
      mod(const_cast<value_type&>(x->value()));
    }
    BOOST_CATCH(...){
      this->erase_(x);
      BOOST_RETHROW;
    }
    BOOST_CATCH_END

    BOOST_TRY{
      if(!super::modify_(x)){
        deallocate_node(x);
        --entry_count;
        return false;
      }
      else return true;
    }
    BOOST_CATCH(...){
      deallocate_node(x);
      --entry_count;
      BOOST_RETHROW;
    }
    BOOST_CATCH_END
  }

  template<typename Modifier,typename Rollback>
  bool modify_(Modifier& mod,Rollback& back_,node_type* x)
  {
    BOOST_TRY{
      mod(const_cast<value_type&>(x->value()));
    }
    BOOST_CATCH(...){
      this->erase_(x);
      BOOST_RETHROW;
    }
    BOOST_CATCH_END

    bool b;
    BOOST_TRY{
      b=super::modify_rollback_(x);
    }
    BOOST_CATCH(...){
      BOOST_TRY{
        back_(const_cast<value_type&>(x->value()));
        if(!super::check_rollback_(x))this->erase_(x);
        BOOST_RETHROW;
      }
      BOOST_CATCH(...){
        this->erase_(x);
        BOOST_RETHROW;
      }
      BOOST_CATCH_END
    }
    BOOST_CATCH_END

    BOOST_TRY{
      if(!b){
        back_(const_cast<value_type&>(x->value()));
        if(!super::check_rollback_(x))this->erase_(x);
        return false;
      }
      else return true;
    }
    BOOST_CATCH(...){
      this->erase_(x);
      BOOST_RETHROW;
    }
    BOOST_CATCH_END
  }
*/
#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
  /* serialization */

  friend class boost::serialization::access;

  BOOST_SERIALIZATION_SPLIT_MEMBER()

  typedef typename super::index_saver_type        index_saver_type;
  typedef typename super::index_loader_type       index_loader_type;

  template<class Archive>
  void save(Archive& ar,const unsigned int version)const
  {
    const boost::serialization::collection_size_type       s(size_());
    const detail::serialization_version<value_type> value_version;
    ar<<boost::serialization::make_nvp("count",s);
    ar<<boost::serialization::make_nvp("value_version",value_version);

    index_saver_type sm(bfm_allocator::member,s);

    for(iterator it=super::begin(),it_end=super::end();it!=it_end;++it){
      boost::serialization::save_construct_data_adl(
        ar,boost::addressof(*it),value_version);
      ar<<boost::serialization::make_nvp("item",*it);
      sm.add(it.get_node(),ar,version);
    }
    sm.add_track(header(),ar,version);

    super::save_(ar,version,sm);
  }

  template<class Archive>
  void load(Archive& ar,const unsigned int version)
  {
    BOOST_MULTI_INDEX_CHECK_INVARIANT;

    clear_();
    boost::serialization::collection_size_type       s;
    detail::serialization_version<value_type> value_version;
    if(version<1){
      uint64_t sz;
      ar>>boost::serialization::make_nvp("count",sz);
      s=static_cast<boost::serialization::collection_size_type>(sz);
    }
    else{
      ar>>boost::serialization::make_nvp("count",s);
    }
    if(version<2){
      value_version=0;
    }
    else{
      ar>>boost::serialization::make_nvp("value_version",value_version);
    }

    index_loader_type lm(bfm_allocator::member,s);

    for(uint64_t n=0;n<s;++n){
      detail::archive_constructed<Value> value("item",ar,value_version);
      std::pair<node_type*,bool> p=insert_rv_(
        value.get(),super::end().get_node());
      if(!p.second)boost::throw_exception(
        boost::archive::archive_exception(
          boost::archive::archive_exception::other_exception));
      ar.reset_object_address(
        boost::addressof(p.first->value()),boost::addressof(value.get()));
      lm.add(p.first,ar,version);
    }
    lm.add_track(header(),ar,version);

    super::load_(ar,version,lm);
  }
#endif

#if defined(BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING)
  /* invariant stuff */

  bool invariant_()const
  {
    return super::invariant_();
  }

  void check_invariant_()const
  {
    BOOST_MULTI_INDEX_INVARIANT_ASSERT(invariant_());
  }
#endif

private:
  uint64_t entry_count;

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

template<int N,typename Value,typename IndexSpecifierList,typename Allocator>
typename nth_index<
  multi_index_container<Value,IndexSpecifierList,Allocator>,N>::type&
get(
  multi_index_container<Value,IndexSpecifierList,Allocator>& m)BOOST_NOEXCEPT
{
  typedef multi_index_container<
    Value,IndexSpecifierList,Allocator>    multi_index_type;
  typedef typename nth_index<
    multi_index_container<
      Value,IndexSpecifierList,Allocator>,
    N
  >::type                                  index_type;

  BOOST_STATIC_ASSERT(N>=0&&
    N<
    boost::mpl::size<
      BOOST_DEDUCED_TYPENAME multi_index_type::index_type_list
    >::type::value);

  return detail::converter<multi_index_type,index_type>::index(m);
}

template<int N,typename Value,typename IndexSpecifierList,typename Allocator>
const typename nth_index<
  multi_index_container<Value,IndexSpecifierList,Allocator>,N>::type&
get(
  const multi_index_container<Value,IndexSpecifierList,Allocator>& m
)BOOST_NOEXCEPT
{
  typedef multi_index_container<
    Value,IndexSpecifierList,Allocator>    multi_index_type;
  typedef typename nth_index<
    multi_index_container<
      Value,IndexSpecifierList,Allocator>,
    N
  >::type                                  index_type;

  BOOST_STATIC_ASSERT(N>=0&&
    N<
    boost::mpl::size<
      BOOST_DEDUCED_TYPENAME multi_index_type::index_type_list
    >::type::value);

  return detail::converter<multi_index_type,index_type>::index(m);
}

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

template<
  typename Tag,typename Value,typename IndexSpecifierList,typename Allocator
>
typename ::mira::multi_index::index<
  multi_index_container<Value,IndexSpecifierList,Allocator>,Tag>::type&
get(
  multi_index_container<Value,IndexSpecifierList,Allocator>& m)BOOST_NOEXCEPT
{
  typedef multi_index_container<
    Value,IndexSpecifierList,Allocator>         multi_index_type;
  typedef typename ::mira::multi_index::index<
    multi_index_container<
      Value,IndexSpecifierList,Allocator>,
    Tag
  >::type                                       index_type;

  return detail::converter<multi_index_type,index_type>::index(m);
}

template<
  typename Tag,typename Value,typename IndexSpecifierList,typename Allocator
>
const typename ::mira::multi_index::index<
  multi_index_container<Value,IndexSpecifierList,Allocator>,Tag>::type&
get(
  const multi_index_container<Value,IndexSpecifierList,Allocator>& m
)BOOST_NOEXCEPT
{
  typedef multi_index_container<
    Value,IndexSpecifierList,Allocator>         multi_index_type;
  typedef typename ::mira::multi_index::index<
    multi_index_container<
      Value,IndexSpecifierList,Allocator>,
    Tag
  >::type                                       index_type;

  return detail::converter<multi_index_type,index_type>::index(m);
}

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

template<
  int N,typename IteratorType,
  typename Value,typename IndexSpecifierList,typename Allocator>
typename nth_index_iterator<
  multi_index_container<Value,IndexSpecifierList,Allocator>,N>::type
project(
  multi_index_container<Value,IndexSpecifierList,Allocator>& m,
  IteratorType it)
{
  typedef multi_index_container<
    Value,IndexSpecifierList,Allocator>                multi_index_type;
  typedef typename nth_index<multi_index_type,N>::type index_type;

#if !defined(__SUNPRO_CC)||!(__SUNPRO_CC<0x580) /* Sun C++ 5.7 fails */
  BOOST_STATIC_ASSERT((
    boost::mpl::contains<
      BOOST_DEDUCED_TYPENAME multi_index_type::iterator_type_list,
      IteratorType>::value));
#endif

  BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(it);

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
  typedef detail::converter<
    multi_index_type,
    BOOST_DEDUCED_TYPENAME IteratorType::container_type> converter;
  BOOST_MULTI_INDEX_CHECK_IS_OWNER(it,converter::index(m));
#endif

  return detail::converter<multi_index_type,index_type>::iterator(
    m,static_cast<typename multi_index_type::node_type*>(it.get_node()));
}

template<
  int N,typename IteratorType,
  typename Value,typename IndexSpecifierList,typename Allocator>
typename nth_index_const_iterator<
  multi_index_container<Value,IndexSpecifierList,Allocator>,N>::type
project(
  const multi_index_container<Value,IndexSpecifierList,Allocator>& m,
  IteratorType it)
{
  typedef multi_index_container<
    Value,IndexSpecifierList,Allocator>                multi_index_type;
  typedef typename nth_index<multi_index_type,N>::type index_type;

#if !defined(__SUNPRO_CC)||!(__SUNPRO_CC<0x580) /* Sun C++ 5.7 fails */
  BOOST_STATIC_ASSERT((
    boost::mpl::contains<
      BOOST_DEDUCED_TYPENAME multi_index_type::iterator_type_list,
      IteratorType>::value||
    boost::mpl::contains<
      BOOST_DEDUCED_TYPENAME multi_index_type::const_iterator_type_list,
      IteratorType>::value));
#endif

  BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(it);

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
  typedef detail::converter<
    multi_index_type,
    BOOST_DEDUCED_TYPENAME IteratorType::container_type> converter;
  BOOST_MULTI_INDEX_CHECK_IS_OWNER(it,converter::index(m));
#endif

  return detail::converter<multi_index_type,index_type>::const_iterator(
    m,static_cast<typename multi_index_type::node_type*>(it.get_node()));
}

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

template<
  typename Tag,typename IteratorType,
  typename Value,typename IndexSpecifierList,typename Allocator>
typename index_iterator<
  multi_index_container<Value,IndexSpecifierList,Allocator>,Tag>::type
project(
  multi_index_container<Value,IndexSpecifierList,Allocator>& m,
  IteratorType it)
{
  typedef multi_index_container<
    Value,IndexSpecifierList,Allocator>         multi_index_type;
  typedef typename ::mira::multi_index::index<
    multi_index_type,Tag>::type                 index_type;

#if !defined(__SUNPRO_CC)||!(__SUNPRO_CC<0x580) /* Sun C++ 5.7 fails */
  BOOST_STATIC_ASSERT((
    boost::mpl::contains<
      BOOST_DEDUCED_TYPENAME multi_index_type::iterator_type_list,
      IteratorType>::value));
#endif

  BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(it);

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
  typedef detail::converter<
    multi_index_type,
    BOOST_DEDUCED_TYPENAME IteratorType::container_type> converter;
  BOOST_MULTI_INDEX_CHECK_IS_OWNER(it,converter::index(m));
#endif

  return detail::converter<multi_index_type,index_type>::iterator(
    m,static_cast<typename multi_index_type::node_type*>(it.get_node()));
}

template<
  typename Tag,typename IteratorType,
  typename Value,typename IndexSpecifierList,typename Allocator>
typename index_const_iterator<
  multi_index_container<Value,IndexSpecifierList,Allocator>,Tag>::type
project(
  const multi_index_container<Value,IndexSpecifierList,Allocator>& m,
  IteratorType it)
{
  typedef multi_index_container<
    Value,IndexSpecifierList,Allocator>         multi_index_type;
  typedef typename ::mira::multi_index::index<
    multi_index_type,Tag>::type                 index_type;

#if !defined(__SUNPRO_CC)||!(__SUNPRO_CC<0x580) /* Sun C++ 5.7 fails */
  BOOST_STATIC_ASSERT((
    boost::mpl::contains<
      BOOST_DEDUCED_TYPENAME multi_index_type::iterator_type_list,
      IteratorType>::value||
    boost::mpl::contains<
      BOOST_DEDUCED_TYPENAME multi_index_type::const_iterator_type_list,
      IteratorType>::value));
#endif

  BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(it);

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
  typedef detail::converter<
    multi_index_type,
    BOOST_DEDUCED_TYPENAME IteratorType::container_type> converter;
  BOOST_MULTI_INDEX_CHECK_IS_OWNER(it,converter::index(m));
#endif

  return detail::converter<multi_index_type,index_type>::const_iterator(
    m,static_cast<typename multi_index_type::node_type*>(it.get_node()));
}

/* Comparison. Simple forward to first index. */

template<
  typename Value1,typename IndexSpecifierList1,typename Allocator1,
  typename Value2,typename IndexSpecifierList2,typename Allocator2
>
bool operator==(
  const multi_index_container<Value1,IndexSpecifierList1,Allocator1>& x,
  const multi_index_container<Value2,IndexSpecifierList2,Allocator2>& y)
{
  return get<0>(x)==get<0>(y);
}

template<
  typename Value1,typename IndexSpecifierList1,typename Allocator1,
  typename Value2,typename IndexSpecifierList2,typename Allocator2
>
bool operator<(
  const multi_index_container<Value1,IndexSpecifierList1,Allocator1>& x,
  const multi_index_container<Value2,IndexSpecifierList2,Allocator2>& y)
{
  return get<0>(x)<get<0>(y);
}

template<
  typename Value1,typename IndexSpecifierList1,typename Allocator1,
  typename Value2,typename IndexSpecifierList2,typename Allocator2
>
bool operator!=(
  const multi_index_container<Value1,IndexSpecifierList1,Allocator1>& x,
  const multi_index_container<Value2,IndexSpecifierList2,Allocator2>& y)
{
  return get<0>(x)!=get<0>(y);
}

template<
  typename Value1,typename IndexSpecifierList1,typename Allocator1,
  typename Value2,typename IndexSpecifierList2,typename Allocator2
>
bool operator>(
  const multi_index_container<Value1,IndexSpecifierList1,Allocator1>& x,
  const multi_index_container<Value2,IndexSpecifierList2,Allocator2>& y)
{
  return get<0>(x)>get<0>(y);
}

template<
  typename Value1,typename IndexSpecifierList1,typename Allocator1,
  typename Value2,typename IndexSpecifierList2,typename Allocator2
>
bool operator>=(
  const multi_index_container<Value1,IndexSpecifierList1,Allocator1>& x,
  const multi_index_container<Value2,IndexSpecifierList2,Allocator2>& y)
{
  return get<0>(x)>=get<0>(y);
}

template<
  typename Value1,typename IndexSpecifierList1,typename Allocator1,
  typename Value2,typename IndexSpecifierList2,typename Allocator2
>
bool operator<=(
  const multi_index_container<Value1,IndexSpecifierList1,Allocator1>& x,
  const multi_index_container<Value2,IndexSpecifierList2,Allocator2>& y)
{
  return get<0>(x)<=get<0>(y);
}

/*  specialized algorithms */

template<typename Value,typename IndexSpecifierList,typename Allocator>
void swap(
  multi_index_container<Value,IndexSpecifierList,Allocator>& x,
  multi_index_container<Value,IndexSpecifierList,Allocator>& y)
{
  x.swap(y);
}

} /* namespace multi_index */

#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
/* class version = 1 : we now serialize the size through
 * boost::serialization::collection_size_type.
 * class version = 2 : proper use of {save|load}_construct_data.
 */
/*
namespace serialization {
template<typename Value,typename IndexSpecifierList,typename Allocator>
struct version<
  mira::multi_index_container<Value,IndexSpecifierList,Allocator>
>
{
  BOOST_STATIC_CONSTANT(int,value=2);
};
}
*/ /* namespace serialization */
#endif

/* Associated global functions are promoted to namespace boost, except
 * comparison operators and swap, which are meant to be Koenig looked-up.
 */

using multi_index::get;
using multi_index::project;

} /* namespace mira */

#undef BOOST_MULTI_INDEX_CHECK_INVARIANT
#undef BOOST_MULTI_INDEX_CHECK_INVARIANT_OF
