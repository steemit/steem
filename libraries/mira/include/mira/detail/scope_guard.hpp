#pragma once

#include <boost/multi_index/detail/scope_guard.hpp>

namespace mira{

namespace multi_index{

namespace detail{

using boost::multi_index::detail::scope_guard_impl_base;

typedef const scope_guard_impl_base& scope_guard;

using boost::multi_index::detail::null_guard;

template< bool cond, class T >
using null_guard_return = boost::multi_index::detail::null_guard_return< cond, T >;

template< typename F >
using scope_guard_impl0 = boost::multi_index::detail::scope_guard_impl0 < F >;

template< typename F, typename P1 >
using scope_guard_impl1 = boost::multi_index::detail::scope_guard_impl1< F, P1 >;

template< typename F, typename P1, typename P2 >
using scope_guard_impl2 = boost::multi_index::detail::scope_guard_impl2< F, P1, P2 >;

template< typename F, typename P1, typename P2, typename P3 >
using scope_guard_impl3 = boost::multi_index::detail::scope_guard_impl3< F, P1, P2, P3 >;

template< typename F, typename P1, typename P2, typename P3, typename P4 >
using scope_guard_impl4 = boost::multi_index::detail::scope_guard_impl4< F, P1, P2, P3, P4 >;


template< typename Obj, typename MemFun >
using obj_scop_guard_impl0 = boost::multi_index::detail::obj_scope_guard_impl0< Obj, MemFun >;

template< typename Obj, typename MemFun, typename P1 >
using obj_scop_guard_impl1 = boost::multi_index::detail::obj_scope_guard_impl1< Obj, MemFun, P1 >;

template< typename Obj, typename MemFun, typename P1, typename P2 >
using obj_scop_guard_impl2 = boost::multi_index::detail::obj_scope_guard_impl2< Obj, MemFun, P1, P2 >;

template< typename Obj, typename MemFun, typename P1, typename P2, typename P3 >
using obj_scop_guard_impl3 = boost::multi_index::detail::obj_scope_guard_impl3< Obj, MemFun, P1, P2, P3 >;

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace mira */
