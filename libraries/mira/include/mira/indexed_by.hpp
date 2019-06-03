#pragma once

#include <boost/multi_index/indexed_by.hpp>

namespace mira{

namespace multi_index{

template< typename... Args >
using indexed_by = boost::multi_index::indexed_by< Args... >;

} /* namespace multi_index */

} /* namespace mira */
