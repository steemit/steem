/* Copyright 2003-2017 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#pragma once

#include <boost/multi_index/detail/promotes_arg.hpp>

namespace mira{

namespace multi_index{

namespace detail{

template<typename F,typename Arg1,typename Arg2>
using promotes_1st_arg = boost::multi_index::detail::promotes_1st_arg< F, Arg1, Arg2 >;

template<typename F,typename Arg1,typename Arg2>
using promotes_2nd_arg = boost::multi_index::detail::promotes_2nd_arg< F, Arg1, Arg2 >;

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace mira */
