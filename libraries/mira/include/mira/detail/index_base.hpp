/* Copyright 2003-2017 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#pragma once

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/core/addressof.hpp>
#include <boost/detail/allocator_utilities.hpp>
#include <boost/detail/no_exceptions_support.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility.hpp>
#include <boost/mpl/vector.hpp>
#include <mira/detail/do_not_copy_elements_tag.hpp>
#include <boost/multi_index/detail/vartempl_support.hpp>
#include <mira/multi_index_container_fwd.hpp>
#include <boost/tuple/tuple.hpp>
#include <utility>

#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/utilities/write_batch_with_index.h>

namespace mira{

namespace multi_index{

namespace detail{

/* The role of this class is threefold:
 *   - tops the linear hierarchy of indices.
 *   - terminates some cascading backbone function calls (insert_, etc.),
 *   - grants access to the backbone functions of the final
 *     multi_index_container class (for access restriction reasons, these
 *     cannot be called directly from the index classes.)
 */

//struct lvalue_tag{};
//struct rvalue_tag{};
//struct emplaced_tag{};

template<typename Value,typename IndexSpecifierList,typename Allocator>
class index_base
{
protected:
   typedef multi_index_container<
      Value,IndexSpecifierList,Allocator>       final_type;
   typedef boost::tuples::null_type            ctor_args_list;
   typedef typename std::allocator< Value >    final_allocator_type;
   typedef boost::mpl::vector0<>               index_type_list;
   typedef boost::mpl::vector0<>               iterator_type_list;
   typedef boost::mpl::vector0<>               const_iterator_type_list;

   explicit index_base(const ctor_args_list&){}

   typedef Value                             value_type;

   typedef boost::true_type                  is_terminal_node;
   typedef boost::false_type                 id_type;
   typedef boost::false_type                 id_from_value;
   typedef boost::false_type                 primary_index_type;
   typedef boost::false_type                 iterator;

   db_ptr                                    _db;
   ::rocksdb::WriteBatch                     _write_buffer;
   column_handles                            _handles;

   static const size_t                       COLUMN_INDEX = 0;

   index_base(
      const index_base<Value,IndexSpecifierList,Allocator>&,
      do_not_copy_elements_tag )
   {}

   ~index_base()
   {
      cleanup_column_handles();
   }

   void flush() {}

   bool insert_rocksdb_( const Value& v )
   {
      return true;
   }

   void erase_(value_type& x) {}

   void clear_(){}

   template< typename Modifier >
   bool modify_( Modifier& mod, value_type& v, std::vector< size_t >& )
   {
      mod( v );
      return true;
   }

   void populate_column_definitions_( column_definitions& defs )const
   {
      defs.emplace_back(
         ::rocksdb::kDefaultColumnFamilyName,
         ::rocksdb::ColumnFamilyOptions()
      );
   }

   void cache_first_key() {}

   void commit_first_key_update() {}

   void reset_first_key_update() {}

   void cleanup_column_handles()
   {
      for( auto* h : _handles )
         delete h;

      _handles.clear();
   }

   void dump_lb_call_counts() {}

   /* access to backbone memfuns of Final class */

   final_type&       final(){return *static_cast<final_type*>(this);}
   const final_type& final()const{return *static_cast<const final_type*>(this);}

   bool        final_empty_()const{return final().empty_();}
   std::size_t final_size_()const{return final().size_();}
   std::size_t final_max_size_()const{return final().max_size_();}


   template< BOOST_MULTI_INDEX_TEMPLATE_PARAM_PACK >
   bool final_emplace_rocksdb_(
      BOOST_MULTI_INDEX_FUNCTION_PARAM_PACK)
   {
      return final().emplace_rocksdb_(BOOST_MULTI_INDEX_FORWARD_PARAM_PACK);
   }

   void final_erase_( value_type& v )
   {
      final().erase_( v );
   }

   void final_clear_() { final().clear_(); }

   template< typename Modifier >
   bool final_modify_( Modifier& mod, value_type& x )
   {
      return final().modify_( mod, x );
   }

   size_t final_get_column_size()
   {
      return final().get_column_size();
   }
};

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace mira */
