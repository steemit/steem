#pragma once

#include <boost/multi_index/detail/cons_stdtuple.hpp>

namespace mira{

namespace multi_index{

namespace detail{

using boost::multi_index::detail::cons_stdtuple_ctor_terminal;

template<typename StdTuple,std::size_t N>
using cons_stdtuple_ctor_normal = boost::multi_index::detail::cons_stdtuple_ctor_normal< StdTuple, N>;

template<typename StdTuple,std::size_t N=0>
using cons_stdtuple_ctor = boost::multi_index::detail::cons_stdtuple_ctor< StdTuple, N >;

template<typename StdTuple,std::size_t N>
using cons_stdtuple = boost::multi_index::detail::cons_stdtuple< StdTuple, N >;

using boost::multi_index::detail::make_cons_stdtuple;

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace mira */
