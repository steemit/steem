/* Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#pragma once

#include <boost/multi_index/detail/ord_index_args.hpp>

namespace mira{

namespace multi_index{

namespace detail{

template<typename KeyFromValue>
using index_args_default_compare = boost::multi_index::detail::index_args_default_compare< KeyFromValue >;

template<typename Arg1,typename Arg2,typename Arg3>
using ordered_index_args = boost::multi_index::detail::ordered_index_args< Arg1, Arg2, Arg3 >;

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace mira */
