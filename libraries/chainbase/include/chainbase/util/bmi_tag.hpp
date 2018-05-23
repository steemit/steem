#ifndef __BMI_TAG_HPP
#define __BMI_TAG_HPP

#include "detail/bmi_helpers.hpp"

namespace bmi
{

namespace detail
{
struct tag_marker {};
} /// namespace detail

template <typename... Tag>
struct tag : detail::tag_marker
  {
  typedef typename detail::helpers::type_list<Tag...>::type TagList;
  };

} /// namespace bmi


#endif /// __BMI_TAG_HPP

