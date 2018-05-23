#ifndef __BMI_IMPLICIT_BASE_HPP
#define __BMI_IMPLICIT_BASE_HPP

#include "detail/bmi_helpers.hpp"

namespace bmi
{

template <class DataSourceSpecifierList, class Value, class IndexSpecifierList, class Allocator>
class adapter;

namespace detail
{

/** This class is responsible for retrieving implicit base class (first index defined in the adapter class) to implicitly inherit from.
    Such inheritance is needed to provide all methods defined in such index, directly at container level.
*/
template <class DataSourceSpecifierList, class Value, class IndexSpecifierList, class Allocator>
class adapter_implicit_base
  {
  typedef typename adapter<DataSourceSpecifierList, Value, IndexSpecifierList, Allocator> Adapter;
  typedef typename IndexSpecifierList::FirstIndexSpecifier FirstIndexSpecifier;
  
  public:
    typedef typename FirstIndexSpecifier::template index_implementation_class<Adapter>::type type;
  };

} /// namespace detail 
} /// namespace bmi

#endif ///__BMI_IMPLICIT_BASE_HPP

