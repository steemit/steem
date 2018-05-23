#ifndef __BMI_HELPERS_HPP
#define __BMI_HELPERS_HPP

namespace bmi
{
namespace detail
{
namespace helpers
{

template <int, typename... Args>
struct select;

template <typename T, typename... Args>
struct select<0, T, Args...>
  {
  typedef T type;
  };

template <typename T0, typename T1, typename... Args>
struct select<1, T0, T1, Args...>
  {
  typedef T1 type;
  };

template <typename T0, typename T1, typename T2, typename... Args>
struct select<2, T0, T1, T2, Args...>
  {
  typedef T2 type;
  };

template <typename... T>
struct type_list
  {
  typedef type_list<T...> type;
  };

} /// namespace helpers
} /// namespace detail 
} /// namespace bmi

#endif /// __BMI_HELPERS_HPP


