#pragma once

#include <boost/multi_index/detail/modify_key_adaptor.hpp>

namespace mira{

namespace multi_index{

namespace detail{

template<typename Fun,typename Value,typename KeyFromValue>
using modify_key_adaptor = boost::multi_index::detail::modify_key_adaptor< Fun, Value, KeyFromValue >;

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace mira */
