#pragma once

#include <boost/multi_index/detail/no_duplicate_tags.hpp>

namespace mira{

namespace multi_index{

namespace detail{

using boost::multi_index::detail::duplicate_tag_mark;
using boost::multi_index::detail::duplicate_tag_marker;
using boost::multi_index::detail::no_duplicate_tags;
using boost::multi_index::detail::duplicate_tag_list_marker;
using boost::multi_index::detail::no_duplicate_tags_in_index_list;

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace mira */
