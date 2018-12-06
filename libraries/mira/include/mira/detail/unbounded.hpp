#pragma once

#include <boost/multi_index/detail/unbounded.hpp>

namespace mira{

namespace multi_index{

namespace detail{

using boost::multi_index::detail::unbounded_helper;

typedef unbounded_helper (*unbounded_type)(unbounded_helper);

} /* namespace multi_index::detail */

namespace detail{

using boost::multi_index::detail::none_unbounded_tag;
using boost::multi_index::detail::lower_unbounded_tag;
using boost::multi_index::detail::upper_unbounded_tag;
using boost::multi_index::detail::both_unbounded_tag;

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace mira */
