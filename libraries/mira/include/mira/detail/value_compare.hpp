#pragma once

#include <boost/multi_index/detail/value_compare.hpp>

namespace mira{

namespace multi_index{

namespace detail{

template<typename Value,typename KeyFromValue,typename Compare>
using value_comparison = boost::multi_index::detail::value_comparison< Value, KeyFromValue, Compare >;

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace mira */

