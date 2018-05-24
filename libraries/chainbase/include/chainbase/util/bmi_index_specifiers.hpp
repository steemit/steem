#ifndef __BMI_INDEX_SPECIFIERS_HPP
#define __BMI_INDEX_SPECIFIERS_HPP

#include "bmi_tag.hpp"

#include "detail/bmi_helpers.hpp"
#include "detail/bmi_ordered_index.hpp"

#include <type_traits>

namespace bmi
{

/** Allows to specify index definition by providing tag list, key_from_value converter and comparator.
*/
template <bool Unique, typename... Args>
class ordered_index_specifier
  {
  typedef typename ordered_index_specifier<Unique, Args...>  ThisType;

  typedef typename detail::helpers::select<0, Args...>::type Arg1;
  typedef typename detail::helpers::select<1, Args...>::type Arg2;
  typedef typename detail::helpers::select<2, Args...>::type Arg3;
  typedef typename std::is_convertible<Arg1, detail::tag_marker>::type IsTag;

  public:
    typedef typename std::conditional<IsTag::value, Arg1, tag<>>::type tag_list_type;
    typedef typename std::conditional<IsTag::value, Arg2, Arg1>::type key_from_value_type;
    typedef typename std::conditional<IsTag::value, Arg3, Arg2>::type compare_type;
    typedef typename std::integral_constant<bool, Unique>::type is_unique_type;

    template <class Adapter>
    struct index_implementation_class
      {
      typedef detail::ordered_index<ThisType, key_from_value_type, compare_type, Unique> type;
      };
  };

template <typename... Args>
using ordered_unique = ordered_index_specifier<true, Args...>;

template <typename... Args>
using ordered_nonunique = ordered_index_specifier<false, Args...>;

namespace detail
{
template <class Class, typename Type, Type Class::*PtrToMember>
struct const_member
{
   Type& operator() (const Class& x) const
      { return x.*PtrToMember; }
};

template <class Class, typename Type, Type Class::*PtrToMember>
struct non_const_member
{
   const Type& operator() (const Class& x) const
      { return x.*PtrToMember; }
   Type& operator() (Class& x) const
      { return x.*PtrToMember; }
};
} // namespace detail

template <class Class, typename Type, Type Class::*PtrToMember>
struct member : std::conditional_t<std::is_const<Type>::value,
                                   detail::const_member<Class, Type, PtrToMember>,
                                   detail::non_const_member<Class, Type, PtrToMember>>
{
   typedef Type result_type;

   template <typename ChainedPtr>
   std::enable_if_t<!std::is_convertible<const ChainedPtr&, const Class&>::value, Type&>
   operator() (const ChainedPtr& x) const
      { return operator()(*x); }
};

template <class Class, typename Type, Type (Class::*PtrToMemberFunction)() const>
struct const_mem_fun
{
   typedef typename remove_reference<Type>::type result_type;

   template <typename ChainedPtr>
   std::enable_if_t<std::is_convertible<const ChainedPtr&, const Class&>, Type>
   operator() (const ChainedPtr& x) const
      { return operator()(*x); }

  Type operator() (const Class& x) const
   { return (x.*PtrToMemberFunction)(); }
};

} /// namespace bmi

#endif /// __BMI_INDEX_SPECIFIERS_HPP
