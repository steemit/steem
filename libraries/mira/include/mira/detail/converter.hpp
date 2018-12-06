#pragma once

namespace mira{

namespace multi_index{

namespace detail{

template<typename MultiIndexContainer,typename Index>
struct converter
{
  static const Index& index(const MultiIndexContainer& x){return x;}
  static Index&       index(MultiIndexContainer& x){return x;}

  static typename Index::const_iterator const_iterator(
    const MultiIndexContainer& x,typename MultiIndexContainer::node_type* node)
  {
    return x.Index::make_iterator(node);
  }

  static typename Index::iterator iterator(
    MultiIndexContainer& x,typename MultiIndexContainer::node_type* node)
  {
    return x.Index::make_iterator(node);
  }
};

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace mira */
