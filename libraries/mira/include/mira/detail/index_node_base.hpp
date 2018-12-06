/* Copyright 2003-2016 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#pragma once

#include <boost/multi_index/detail/index_node_base.hpp>

namespace mira{

namespace multi_index{

namespace detail{

template<typename Value>
using pod_value_holder = boost::multi_index::detail::pod_value_holder< Value >;

template<typename Value,typename Allocator>
struct index_node_base:private pod_value_holder<Value>
{
  typedef index_node_base base_type; /* used for serialization purposes */
  typedef Value           value_type;
  typedef Allocator       allocator_type;

#include <boost/multi_index/detail/ignore_wstrict_aliasing.hpp>

  value_type& value()
  {
    return *reinterpret_cast<value_type*>(&this->space);
  }

  const value_type& value()const
  {
    return *reinterpret_cast<const value_type*>(&this->space);
  }

#include <boost/multi_index/detail/restore_wstrict_aliasing.hpp>

  static index_node_base* from_value(const value_type* p)
  {
    return static_cast<index_node_base *>(
      reinterpret_cast<pod_value_holder<Value>*>( /* std 9.2.17 */
        const_cast<value_type*>(p)));
  }

};

template<typename Node,typename Value>
Node* node_from_value(const Value* p)
{
  typedef typename Node::allocator_type allocator_type;
  return static_cast<Node*>(
    index_node_base<Value,allocator_type>::from_value(p));
}

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace boost */
