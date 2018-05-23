#ifndef __BMI_INDEX_ACCESS_HPP
#define __BMI_INDEX_ACCESS_HPP

namespace bmi
{
namespace detail
{

/** Helper class being able to select an index specified by Tag from given Adapter class.
*/
template <typename Adapter, typename Tag>
struct index_by_tag
  {
  /// TO BE IMPLEMENTED
  typedef int type;
  };

/** Helper class being able to select an index specified by position from given Adapter class.
*/
template <typename Adapter, int N>
struct index_by_pos
  {
  /// TO BE IMPLEMENTED
  typedef int type;
  };

} /// namespace detail 
} /// namespace bmi

#endif /// __BMI_INDEX_ACCESS_HPP

