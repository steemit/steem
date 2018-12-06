#pragma once

#include <boost/multi_index/detail/serialization_version.hpp>

namespace mira{

namespace multi_index{

namespace detail{

template< typename T >
using serialization_version = boost::multi_index::detail::serialization_version< T >;

} /* namespace multi_index::detail */

} /* namespace multi_index */

namespace serialization {

template<typename T>
using version = boost::serialization::version< T >;

} /* namespace serialization */

} /* namespace mira */
