#pragma once

#include <boost/multi_index/detail/archive_constructed.hpp>

namespace mira{

namespace multi_index{

namespace detail{

template<typename T>
using archive_constructed = boost::multi_index::detail::archive_constructed< T >;

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace mira */
