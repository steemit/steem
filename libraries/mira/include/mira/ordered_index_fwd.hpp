/* Copyright 2003-2015 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#pragma once

//#include <boost/multi_index/detail/ord_index_args.hpp>
//#include <boost/multi_index/detail/ord_index_impl_fwd.hpp>

#include <mira/detail/ord_index_args.hpp>

namespace mira{

namespace multi_index{

template<typename Arg1,typename Arg2=boost::mpl::na,typename Arg3=boost::mpl::na>
struct ordered_unique;

} /* namespace multi_index */

} /* namespace mira */
