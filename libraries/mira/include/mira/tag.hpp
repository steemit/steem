#pragma once

#include <boost/multi_index/tag.hpp>

namespace mira{

namespace multi_index{

namespace detail{

using boost::multi_index::detail::tag_marker;

template<typename T>
using is_tag = boost::multi_index::detail::is_tag< T >;

} /* namespace multi_index::detail */

template< typename T >
using tag = boost::multi_index::tag< T >;

} /* namespace multi_index */

} /* namespace mira */
