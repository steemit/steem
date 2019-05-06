/* Copyright 2003-2015 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#pragma once

#include <boost/multi_index/mem_fun.hpp>

namespace mira{

namespace multi_index{

/* mem_fun implements a read-only key extractor based on a given non-const
 * member function of a class.
 * const_mem_fun does the same for const member functions.
 * Additionally, mem_fun  and const_mem_fun are overloaded to support
 * referece_wrappers of T and "chained pointers" to T's. By chained pointer
 * to T we  mean a type P such that, given a p of Type P
 *   *...n...*x is convertible to T&, for some n>=1.
 * Examples of chained pointers are raw and smart pointers, iterators and
 * arbitrary combinations of these (vg. T** or unique_ptr<T*>.)
 */

template<class Class,typename Type,Type (Class::*PtrToMemberFunction)()const>
using const_mem_fun = boost::multi_index::const_mem_fun< Class, Type, PtrToMemberFunction >;

} /* namespace multi_index */

} /* namespace mira */
