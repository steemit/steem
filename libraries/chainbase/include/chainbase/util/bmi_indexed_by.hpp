#ifndef __BMI_INDEXED_BY_HPP
#define __BMI_INDEXED_BY_HPP

#include "detail/bmi_helpers.hpp"

namespace bmi
{

template <typename... Indices>
struct indexed_by
  {
  typedef typename detail::helpers::select<0, Indices...>::type  FirstIndexSpecifier;
  typedef typename detail::helpers::type_list<Indices...>::type  IndexSpecifierList;
  };

} /// namespace bmi

#endif /// __BMI_INDEXED_BY_HPP

