/* Copyright 2003-2015 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#pragma once

#include <boost/multi_index/ordered_index.hpp>
#include <mira/detail/ord_index_impl.hpp>
#include <mira/ordered_index_fwd.hpp>

namespace mira{

namespace multi_index{

namespace detail{

/* no augment policy for plain ordered indices */

using boost::multi_index::detail::null_augment_policy;

} /* namespace multi_index::detail */

/* ordered_index specifiers */

template<typename Arg1,typename Arg2,typename Arg3>
struct ordered_unique
{
  typedef typename detail::ordered_index_args<
    Arg1,Arg2,Arg3>                                index_args;
  typedef typename index_args::tag_list_type::type tag_list_type;
  typedef typename index_args::key_from_value_type key_from_value_type;
  typedef typename index_args::compare_type        compare_type;

  template<typename SuperMeta>
  struct index_class
  {
    typedef detail::ordered_index<
      key_from_value_type,compare_type,
      SuperMeta,tag_list_type,detail::ordered_unique_tag,
      detail::null_augment_policy>                        type;
  };
};

} /* namespace multi_index */

} /* namespace mira */
