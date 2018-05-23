#ifndef __BMI_DATA_SOURCES_HPP
#define __BMI_DATA_SOURCES_HPP

#include "detail/bmi_helpers.hpp"

namespace bmi
{

/** Allows to specify list of data source classes, providing data to the BMIC adapter class.
    The order of specified data sources is significant.
    Specified 
*/
template <typename... DataSource>
struct data_sources
  {
  typedef typename detail::helpers::type_list<DataSource...>::type DataSourceList;
  };

} /// namespace bmi

#endif /// __BMI_DATA_SOURCES_HPP

