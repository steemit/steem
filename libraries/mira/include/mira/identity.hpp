/* Copyright 2003-2015 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#pragma once

#include <boost/multi_index/identity.hpp>

namespace mira{

namespace multi_index{

namespace detail{

template< typename Type >
using const_identity_base = boost::multi_index::detail::const_identity_base< Type >;

template< typename Type >
using non_const_identity_base = boost::multi_index::detail::non_const_identity_base< Type >;

} /* namespace multi_index::detail */

template< class Type >
using identity = boost::multi_index::identity< Type >;

} /* namespace multi_index */

} /* namespace mira */
