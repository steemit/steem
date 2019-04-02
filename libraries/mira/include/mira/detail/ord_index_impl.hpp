/* Copyright 2003-2018 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 *
 * The internal implementation of red-black trees is based on that of SGI STL
 * stl_tree.h file:
 *
 * Copyright (c) 1996,1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 */

#pragma once

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <boost/call_traits.hpp>
#include <boost/core/addressof.hpp>
#include <boost/core/demangle.hpp>
#include <boost/detail/no_exceptions_support.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/foreach_fwd.hpp>
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/move/core.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/push_front.hpp>
#include <mira/detail/rocksdb_iterator.hpp>
#include <mira/detail/slice_compare.hpp>
#include <boost/multi_index/detail/vartempl_support.hpp>
#include <boost/ref.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/type_traits/is_same.hpp>
#include <utility>
#include <memory>
#include <functional>

#include <fc/log/logger.hpp>

#include <iostream>
#include <iterator>

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
#include <initializer_list>
#endif

#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
#include <boost/archive/archive_exception.hpp>
#include <boost/bind.hpp>
#include <mira/detail/duplicates_iterator.hpp>
#include <boost/throw_exception.hpp>
#endif

#if defined(BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING)
#define BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT_OF(x)                    \
  detail::scope_guard BOOST_JOIN(check_invariant_,__LINE__)=                 \
    detail::make_obj_guard(x,&ordered_index_impl::check_invariant_);         \
  BOOST_JOIN(check_invariant_,__LINE__).touch();
#define BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT                          \
  BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT_OF(*this)
#else
#define BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT_OF(x)
#define BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT
#endif

#define ROCKSDB_ITERATOR_PARAM_PACK super::_handles, COLUMN_INDEX, super::_db, *_cache

namespace mira{

namespace multi_index{

namespace detail{

/* ordered_index adds a layer of ordered indexing to a given Super and accepts
 * an augmenting policy for optional addition of order statistics.
 */

/* Most of the implementation of unique and non-unique indices is
 * shared. We tell from one another on instantiation time by using
 * these tags.
 */

struct ordered_unique_tag{};

template<
  typename KeyFromValue,typename Compare,
  typename SuperMeta,typename TagList,typename Category,typename AugmentPolicy
>
class ordered_index;

template<
  typename KeyFromValue,typename Compare,
  typename SuperMeta,typename TagList,typename Category,typename AugmentPolicy
>
class ordered_index_impl:
  BOOST_MULTI_INDEX_PROTECTED_IF_MEMBER_TEMPLATE_FRIENDS SuperMeta::type

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
  ,public safe_mode::safe_container<
    ordered_index_impl<
      KeyFromValue,Compare,SuperMeta,TagList,Category,AugmentPolicy> >
#endif
{

  typedef typename SuperMeta::type                   super;

public:
  /* types */

  typedef typename KeyFromValue::result_type         key_type;
  typedef typename super::value_type                 value_type;
  typedef KeyFromValue                               key_from_value;
  typedef slice_comparator<
    key_type,
    Compare >                                        key_compare;
  typedef boost::tuple<key_from_value,key_compare>          ctor_args;
  typedef typename super::final_allocator_type       allocator_type;
#ifdef BOOST_NO_CXX11_ALLOCATOR
  typedef typename allocator_type::reference         reference;
  typedef typename allocator_type::const_reference   const_reference;
#else
  typedef value_type&                                reference;
  typedef const value_type&                          const_reference;
#endif

   typedef boost::false_type                          is_terminal_node;

   typedef typename boost::mpl::if_<
      typename super::is_terminal_node,
      ordered_index_impl<
         KeyFromValue,
         Compare,
         SuperMeta,
         TagList,
         Category,
         AugmentPolicy >,
      typename super::primary_index_type >::type      primary_index_type;

//*
   typedef typename boost::mpl::if_<
      typename super::is_terminal_node,
      key_type,
      typename super::id_type >::type                 id_type;
//*/
//   typedef typename primary_index_type::id_type         id_type;

//*
   typedef typename boost::mpl::if_<
      typename super::is_terminal_node,
      KeyFromValue,
      typename super::id_from_value >::type           id_from_value;
//*/

//   typedef typename primary_index_type::id_from_value   id_from_value;

/*
   typedef object_cache<
      value_type,
      id_type,
      id_from_value >                                 object_cache_type;

   typedef object_cache_factory<
      value_type,
      id_type,
      id_from_value >                                 object_cache_factory_type;
*/
   typedef rocksdb_iterator<
      value_type,
      key_type,
      KeyFromValue,
      key_compare,
      id_type,
      id_from_value >                                iterator;

  typedef iterator                                   const_iterator;

  typedef typename iterator::cache_type               object_cache_type;
  typedef typename object_cache_type::factory_type    object_cache_factory_type;

  typedef std::size_t                                size_type;
  typedef std::ptrdiff_t                             difference_type;
#ifdef BOOST_NO_CXX11_ALLOCATOR
  typedef typename allocator_type::pointer           pointer;
  typedef typename allocator_type::const_pointer     const_pointer;
#else
  typedef std::allocator_traits<allocator_type>      allocator_traits;
  typedef typename allocator_traits::pointer         pointer;
  typedef typename allocator_traits::const_pointer   const_pointer;
#endif
  typedef typename
    boost::reverse_iterator<iterator>                reverse_iterator;
  typedef typename
    boost::reverse_iterator<const_iterator>          const_reverse_iterator;
  typedef TagList                                    tag_list;

protected:
  typedef boost::tuples::cons<
    ctor_args,
    typename super::ctor_args_list>                  ctor_args_list;
  typedef typename boost::mpl::push_front<
    typename super::index_type_list,
    ordered_index<
      KeyFromValue,Compare,
      SuperMeta,TagList,Category,AugmentPolicy
    > >::type                                        index_type_list;
  typedef typename boost::mpl::push_front<
    typename super::iterator_type_list,
    iterator>::type    iterator_type_list;
  typedef typename boost::mpl::push_front<
    typename super::const_iterator_type_list,
    const_iterator>::type                            const_iterator_type_list;

protected:
#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
  typedef safe_mode::safe_container<
    ordered_index_impl>                              safe_super;
#endif

  typedef typename boost::call_traits<
    value_type>::param_type                          value_param_type;
  typedef typename boost::call_traits<
    key_type>::param_type                            key_param_type;

  /* Needed to avoid commas in BOOST_MULTI_INDEX_OVERLOADS_TO_VARTEMPL
   * expansion.
   */

   typedef std::pair<
      typename primary_index_type::iterator,
      bool >                                          emplace_return_type;

   static const size_t                                COLUMN_INDEX = super::COLUMN_INDEX + 1;

   std::shared_ptr< object_cache_type >               _cache;

   uint32_t                                           _key_modification_count = 0;
   rocksdb::FlushOptions                              _flush_opts;

   fc::optional< key_type >                           _first_key;
   fc::optional< key_type >                           _first_key_update;
   bool                                               _delete_first_key = false;

public:

  /* construct/copy/destroy
   * Default and copy ctors are in the protected section as indices are
   * not supposed to be created on their own. No range ctor either.
   * Assignment operators defined at ordered_index rather than here.
   */

  allocator_type get_allocator()const BOOST_NOEXCEPT
  {
    return this->final().get_allocator();
  }

  /* iterators */

   iterator begin() BOOST_NOEXCEPT
   {
      if( _first_key.valid() )
         return make_iterator( *_first_key );
      return iterator::begin( ROCKSDB_ITERATOR_PARAM_PACK );
   }

   const_iterator
      begin()const BOOST_NOEXCEPT
   {
      if( _first_key.valid() )
         return make_iterator( *_first_key );
      return const_iterator::begin( ROCKSDB_ITERATOR_PARAM_PACK );
   }

  iterator
    end()BOOST_NOEXCEPT{return iterator::end( ROCKSDB_ITERATOR_PARAM_PACK ); }
  const_iterator
    end()const BOOST_NOEXCEPT{return const_iterator::end( ROCKSDB_ITERATOR_PARAM_PACK ); }
  reverse_iterator
    rbegin()BOOST_NOEXCEPT{return boost::make_reverse_iterator(end());}
  const_reverse_iterator
    rbegin()const BOOST_NOEXCEPT{return boost::make_reverse_iterator(end());}
  reverse_iterator
    rend()BOOST_NOEXCEPT{return boost::make_reverse_iterator(begin());}
  const_reverse_iterator
    rend()const BOOST_NOEXCEPT{return boost::make_reverse_iterator(begin());}
  const_iterator
    cbegin()const BOOST_NOEXCEPT{return begin();}
  const_iterator
    cend()const BOOST_NOEXCEPT{return end();}
  const_reverse_iterator
    crbegin()const BOOST_NOEXCEPT{return rbegin();}
  const_reverse_iterator
    crend()const BOOST_NOEXCEPT{return rend();}

   iterator iterator_to( const value_type& x )
   {
      return make_iterator( key( x ) );
   }

  const_iterator iterator_to( const value_type& x )const
  {
    return make_iterator( key( x ) );
  }

   void flush()
   {
      super::flush();
      super::_db->Flush( rocksdb::FlushOptions(), super::_handles[ COLUMN_INDEX ] );
   }

  /* capacity */

  bool      empty()const BOOST_NOEXCEPT{return this->final_empty_();}
  size_type size()const BOOST_NOEXCEPT{return this->final_size_();}
  size_type max_size()const BOOST_NOEXCEPT{return this->final_max_size_();}

  /* modifiers */

  BOOST_MULTI_INDEX_OVERLOADS_TO_VARTEMPL(
    emplace_return_type,emplace,emplace_impl)

  BOOST_MULTI_INDEX_OVERLOADS_TO_VARTEMPL_EXTRA_ARG(
    iterator,emplace_hint,emplace_hint_impl,iterator,position)

  std::pair<iterator,bool> insert(const value_type& x)
  {
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    return this->final_insert_( x );
  }

   iterator erase( iterator position )
   {
      BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
      BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;

      value_type v = *position;
      ++position;
      this->final_erase_( v );
      return position;
   }

   template< typename Modifier >
   bool modify( iterator position, Modifier mod )
   {
      BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR( position );
      BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;

      value_type v = *position;
      return this->final_modify_( mod, v );
   }

   template< typename Modifier, typename Rollback >
   bool modify( iterator position, Modifier mod, Rollback back_ )
   {
      BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR( position );
      BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;

      return this->final_modify_( mod, back_, *position );
   }

  void clear()BOOST_NOEXCEPT
  {
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    this->final_clear_();
  }

  /* observers */

  key_from_value key_extractor()const{return key;}
  key_compare    key_comp()const{return comp_;}

  /* set operations */

  /* Internally, these ops rely on const_iterator being the same
   * type as iterator.
   */

   template< typename CompatibleKey >
   iterator find( const CompatibleKey& x )const
   {
      return iterator::find( ROCKSDB_ITERATOR_PARAM_PACK, x );
   }

   template<typename CompatibleKey>
   iterator lower_bound( const CompatibleKey& x )const
   {
      return iterator::lower_bound( ROCKSDB_ITERATOR_PARAM_PACK, x );
   }

   iterator upper_bound( const key_type& x )const
   {
      return iterator::upper_bound( ROCKSDB_ITERATOR_PARAM_PACK, x );
   }

   template< typename CompatibleKey >
   iterator upper_bound( const CompatibleKey& x )const
   {
      return iterator::upper_bound( ROCKSDB_ITERATOR_PARAM_PACK, x );
   }

  /* range */

   template< typename LowerBounder >
   std::pair< iterator, iterator >
   range( LowerBounder lower, key_type& upper )const
   {
      return iterator::range( ROCKSDB_ITERATOR_PARAM_PACK, lower, upper );
   }

   template< typename CompatibleKey >
   std::pair< iterator, iterator >
   equal_range( const CompatibleKey& key )const
   {
      return iterator::equal_range( ROCKSDB_ITERATOR_PARAM_PACK, key );
   }

BOOST_MULTI_INDEX_PROTECTED_IF_MEMBER_TEMPLATE_FRIENDS:
  ordered_index_impl(const ctor_args_list& args_list):
    super(args_list.get_tail()),
    _cache( object_cache_factory_type::get_shared_cache() ),
    key(boost::tuples::get<0>(args_list.get_head())),
    comp_(boost::tuples::get<1>(args_list.get_head()))
  {
    empty_initialize();
    _cache->set_index_cache( COLUMN_INDEX, std::make_unique< index_cache< value_type, key_type, key_from_value > >() );
  }

  ordered_index_impl(
    const ordered_index_impl<
      KeyFromValue,Compare,SuperMeta,TagList,Category,AugmentPolicy>& x):
    super(x),

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
    safe_super(),
#endif
    key(x.key),
    _cache( object_cache_factory_type::get_shared_cache() ),
    comp_(x.comp_)
  {
    /* Copy ctor just takes the key and compare objects from x. The rest is
     * done in a subsequent call to copy_().
     */
    _cache = new object_cache_type;
  }

  ordered_index_impl(
     const ordered_index_impl<
       KeyFromValue,Compare,SuperMeta,TagList,Category,AugmentPolicy>& x,
     do_not_copy_elements_tag):
    super(x,do_not_copy_elements_tag()),

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
    safe_super(),
#endif
    _cache( object_cache_factory_type::get_shared_cache() ),
    key(x.key),
    comp_(x.comp_)
  {
    empty_initialize();
  }

  ~ordered_index_impl()
  {
    /* the container is guaranteed to be empty by now */
  }

   iterator       make_iterator( const key_type& key )
   {
      return iterator( ROCKSDB_ITERATOR_PARAM_PACK, key );
   }

   iterator       make_iterator( const ::rocksdb::Slice& s )
   {
      return iterator( ROCKSDB_ITERATOR_PARAM_PACK, s );
   }

   const_iterator make_iterator( const key_type& key )const
   {
      return const_iterator( ROCKSDB_ITERATOR_PARAM_PACK, key );
   }

   const_iterator make_iterator( const ::rocksdb::Slice& s )const
   {
      return const_iterator( ROCKSDB_ITERATOR_PARAM_PACK, s );
   }

   bool insert_rocksdb_( const value_type& v )
   {
      if( super::insert_rocksdb_( v ) )
      {
         ::rocksdb::Status s;
         ::rocksdb::PinnableSlice read_buffer;
         key_type new_key = key( v );
         ::rocksdb::PinnableSlice key_slice;
         pack_to_slice< key_type >( key_slice, new_key );

         s = super::_db->Get(
            ::rocksdb::ReadOptions(),
            super::_handles[ COLUMN_INDEX],
            key_slice,
            &read_buffer );

         // Key already exists, uniqueness constraint violated
         if( s.ok() )
         {
            ilog( "Key ${k} already exists. Object: ${o}", ("k",new_key)("o", v) );
            return false;
         }

         ::rocksdb::PinnableSlice value_slice;

         if( COLUMN_INDEX == 1 )
         {
            // Insert base case
            pack_to_slice( value_slice, v );
         }
         else
         {
            // Insert referential case
            pack_to_slice( value_slice, id( v ) );
         }

         s = super::_write_buffer.Put(
            super::_handles[ COLUMN_INDEX ],
            key_slice,
            value_slice );

         if( ( _first_key.valid() && key_comp()( new_key, *_first_key ) )
            || !_first_key.valid() )
         {
            _first_key_update = new_key;
         }

         return s.ok();
      }
      return false;
  }

   void erase_( value_type& v )
   {
      super::erase_( v );

      auto old_key = key( v );
      PinnableSlice old_key_slice;
      pack_to_slice( old_key_slice, old_key );

      super::_write_buffer.Delete(
         super::_handles[ COLUMN_INDEX ],
         old_key_slice );

      if( _first_key.valid() && ( key_comp()( old_key, *_first_key ) == key_comp()( *_first_key, old_key ) ) )
      {
         auto new_key_itr = ++find( *_first_key );
         if( new_key_itr != end() )
         {
            _first_key_update = key( *new_key_itr );
         }
         else
         {
            _delete_first_key = true;
         }
      }
   }

  void clear_()
  {
      super::_db->DropColumnFamily( super::_handles[ COLUMN_INDEX ] );
      super::clear_();
      empty_initialize();
   }

   template< typename Modifier >
   bool modify_( Modifier mod, value_type& v, std::vector< size_t >& modified_indices )
   {
      key_type old_key = key( v );
      if( super::modify_( mod, v, modified_indices ) )
      {
         ::rocksdb::Status s = ::rocksdb::Status::OK();

         key_type new_key = key( v );
         PinnableSlice new_key_slice;

         PinnableSlice value_slice;

         if( COLUMN_INDEX == 1 )
         {
            // Primary key cannot change
            if( new_key != old_key )
               return false;

            pack_to_slice( new_key_slice, new_key );
            pack_to_slice( value_slice, v );
         }
         else if( new_key != old_key )
         {
            size_t col_index = COLUMN_INDEX;
            modified_indices.push_back( col_index );

            ::rocksdb::PinnableSlice read_buffer;

            pack_to_slice( new_key_slice, new_key );

            s = super::_db->Get(
               ::rocksdb::ReadOptions(),
               super::_handles[ COLUMN_INDEX ],
               new_key_slice,
               &read_buffer );

            // New key already exists, uniqueness constraint violated
            if( s.ok() )
            {
               ilog( "Key ${k} already exists. Object: ${o}", ("k",new_key)("o", v) );
               return false;
            }

            PinnableSlice old_key_slice;
            pack_to_slice( old_key_slice, old_key );

            s = super::_write_buffer.Delete(
               super::_handles[ COLUMN_INDEX ],
               old_key_slice );

            if( !s.ok() ) return false;

            pack_to_slice( value_slice, id( v ) );

            ++_key_modification_count;
         }
         else
         {
            return true;
         }

         s = super::_write_buffer.Put(
            super::_handles[ COLUMN_INDEX ],
            new_key_slice,
            value_slice );

         // If the new key is less than the current first, it will be the new first key
         if( _first_key.valid() && key_comp()( new_key, *_first_key ) )
         {
            _first_key_update = new_key;
         }
         // Else if we are updating the current first key AND the new key is different...
         else if( _first_key.valid() &&  key_comp()( *_first_key, new_key )
            && ( key_comp()( old_key, *_first_key ) == key_comp()( *_first_key, old_key ) ) )
         {
            // Find the second key (first_key_update)
            auto first_key_update_itr = ++find( *_first_key );
            if( first_key_update_itr != end() )
            {
               auto first_key_update = key( *first_key_update_itr );
               // If the second key is less than the new key, then the second key is the next first key
               if( key_comp()( first_key_update, new_key ) )
               {
                  _first_key_update = first_key_update;
               }
               // Otherise the new key is between the first key and the second key and will be the next fist key
               else
               {
                  _first_key_update = new_key;
               }
            }
            // If there is no second key, then the new key is the next first key
            else
            {
               _first_key_update = new_key;
            }
         }

         if( _key_modification_count > 1500 )
         {
            super::_db->Flush( _flush_opts, super::_handles[ COLUMN_INDEX ] );
            _key_modification_count = 0;
         }

         return s.ok();
      }

      return false;
   }

   void populate_column_definitions_( column_definitions& defs )const
   {
      super::populate_column_definitions_( defs );
      // TODO: Clean this up so it outputs the tag name instead of a tempalte type.
      // But it is unique, so it works.
      std::string tags = boost::core::demangle( typeid( tag_list ).name() );
      //std::cout <<  COLUMN_INDEX << ':' << tags << std::endl;
      defs.emplace_back(
         tags,
         ::rocksdb::ColumnFamilyOptions()
      );
      defs.back().options.comparator = &comp_;
   }

   void cache_first_key()
   {
      super::cache_first_key();
      if( !_first_key.valid() )
      {
         auto b = iterator::begin( ROCKSDB_ITERATOR_PARAM_PACK );
         if( b != end() )
         {
            _first_key = key( *b );
         }
      }
   }

   void commit_first_key_update()
   {
      super::commit_first_key_update();

      if( _first_key_update.valid() )
      {
         _first_key = _first_key_update;
         _first_key_update.reset();
      }
      else if( _delete_first_key )
      {
         _first_key.reset();
         _delete_first_key = false;
      }
   }

   void reset_first_key_update()
   {
      super::reset_first_key_update();
      _first_key_update.reset();
      _delete_first_key = false;
   }

   void dump_lb_call_counts()
   {
      super::dump_lb_call_counts();
      ilog( boost::core::demangle( typeid( tag_list ).name() ) );
      wdump( (iterator::lb_call_count())(iterator::lb_prev_call_count())(iterator::lb_no_prev_count())(iterator::lb_miss_count()) );
   }

private:
  void empty_initialize() {}


  /* emplace entry point */

   template< BOOST_MULTI_INDEX_TEMPLATE_PARAM_PACK >
   std::pair< typename primary_index_type::iterator, bool >
   emplace_impl( BOOST_MULTI_INDEX_FUNCTION_PARAM_PACK )
   {
      BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;

      value_type v( std::forward< Args >(args)... );

      bool res = this->final_emplace_rocksdb_( v );

      if ( res )
      {
         std::lock_guard< std::mutex > lock( _cache->get_lock() );
         _cache->cache( v );
      }

      return std::pair< typename primary_index_type::iterator, bool >(
         res ? primary_index_type::iterator_to( v ) :
               primary_index_type::end(),
         res
      );
  }

protected: /* for the benefit of AugmentPolicy::augmented_interface */
  key_from_value key;
  key_compare    comp_;
   id_from_value id;

#if defined(BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING)&&\
    BOOST_WORKAROUND(__MWERKS__,<=0x3003)
#pragma parse_mfunc_templ reset
#endif
};

template<
  typename KeyFromValue,typename Compare,
  typename SuperMeta,typename TagList,typename Category,typename AugmentPolicy
>
class ordered_index:
  public AugmentPolicy::template augmented_interface<
    ordered_index_impl<
      KeyFromValue,Compare,SuperMeta,TagList,Category,AugmentPolicy
    >
  >::type
{
  typedef typename AugmentPolicy::template
    augmented_interface<
      ordered_index_impl<
        KeyFromValue,Compare,
        SuperMeta,TagList,Category,AugmentPolicy
      >
    >::type                                       super;
public:
  typedef typename super::ctor_args_list          ctor_args_list;
  typedef typename super::allocator_type          allocator_type;
  typedef typename super::iterator                iterator;

  /* construct/copy/destroy
   * Default and copy ctors are in the protected section as indices are
   * not supposed to be created on their own. No range ctor either.
   */


protected:
  ordered_index(
    const ctor_args_list& args_list):
    super(args_list){}

  ordered_index(const ordered_index& x):super(x){};

  ordered_index(const ordered_index& x,do_not_copy_elements_tag):
    super(x,do_not_copy_elements_tag()){};
};

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace mira */
