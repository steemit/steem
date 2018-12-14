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
#include <boost/mpl/at.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/size.hpp>
#include <mira/detail/index_base.hpp>
#include <mira/detail/is_index_list.hpp>
#include <boost/static_assert.hpp>

namespace mira{

namespace multi_index{

namespace detail{

/* MPL machinery to construct a linear hierarchy of indices out of
 * a index list.
 */

struct index_applier
{
  template<typename IndexSpecifierMeta,typename SuperMeta>
  struct apply
  {
    typedef typename IndexSpecifierMeta::type            index_specifier;
    typedef typename index_specifier::
      BOOST_NESTED_TEMPLATE index_class<SuperMeta>::type type;
  };
};

template<int N,typename Value,typename IndexSpecifierList,typename Allocator>
struct nth_layer
{
  BOOST_STATIC_CONSTANT(int,length=boost::mpl::size<IndexSpecifierList>::value);

  typedef typename  boost::mpl::eval_if_c<
    N==length,
    boost::mpl::identity<index_base<Value,IndexSpecifierList,Allocator> >,
    boost::mpl::apply2<
      index_applier,
      boost::mpl::at_c<IndexSpecifierList,length-1-N>,
      nth_layer<N+1,Value,IndexSpecifierList,Allocator>
    >
  >::type type;
};

template<typename Value,typename IndexSpecifierList,typename Allocator>
struct multi_index_base_type:nth_layer<0,Value,IndexSpecifierList,Allocator>
{
  BOOST_STATIC_ASSERT(detail::is_index_list<IndexSpecifierList>::value);
};

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace boost */
