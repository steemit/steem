#pragma once

#include <boost/multi_index/detail/bidir_node_iterator.hpp>

namespace mira{

namespace multi_index{

namespace detail{

/* Iterator class for node-based indices with bidirectional
 * iterators (ordered and sequenced indices.)
 */

template<typename Node>
using bidir_node_iterator = boost::multi_index::detail::bidir_node_iterator< Node >;


} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace mira */
