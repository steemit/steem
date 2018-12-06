/* Copyright 2003-2015 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#pragma once

//#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
//#include <boost/mpl/bool.hpp>
//#include <boost/type_traits/is_same.hpp>

#include <boost/multi_index/detail/raw_ptr.hpp>

namespace mira{

namespace multi_index{

namespace detail{

/* gets the underlying pointer of a pointer-like value */

using boost::multi_index::detail::raw_ptr;

/*
template<typename RawPointer>
inline RawPointer raw_ptr(RawPointer const& p, boost::mpl::true_)
{
  return p;
}

template<typename RawPointer,typename Pointer>
inline RawPointer raw_ptr(Pointer const& p, boost::mpl::false_)
{
  return p==Pointer(0)?0:&*p;
}

template<typename RawPointer,typename Pointer>
inline RawPointer raw_ptr(Pointer const& p)
{
  return raw_ptr<RawPointer>(p,boost::is_same<RawPointer,Pointer>());
}
*/

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace mira */
