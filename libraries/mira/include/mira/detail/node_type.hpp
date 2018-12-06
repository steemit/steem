/* Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#pragma once

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/detail/workaround.hpp>
#include <boost/mpl/bind.hpp>
#include <boost/mpl/reverse_iter_fold.hpp>
#include <boost/mpl/deref.hpp>
#include <mira/multi_index_container_fwd.hpp>
#include <mira/detail/header_holder.hpp>
#include <mira/detail/index_node_base.hpp>
#include <mira/detail/is_index_list.hpp>
#include <boost/static_assert.hpp>

namespace mira{

namespace multi_index{

namespace detail{

struct index_node_applier
{
  template<typename IndexSpecifierIterator,typename Super>
  struct apply
  {
    typedef typename boost::mpl::deref<IndexSpecifierIterator>::type index_specifier;
    typedef typename index_specifier::
      BOOST_NESTED_TEMPLATE node_class<Super>::type type;
  };
};

template<typename Value,typename IndexSpecifierList,typename Allocator>
struct multi_index_node_type
{
  BOOST_STATIC_ASSERT(detail::is_index_list<IndexSpecifierList>::value);

  typedef typename boost::mpl::reverse_iter_fold<
    IndexSpecifierList,
    index_node_base<Value,Allocator>,
    boost::mpl::bind2<index_node_applier,boost::mpl::_2,boost::mpl::_1>
  >::type type;
};

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace mira */
