#ifndef __BMI_ADAPTER_HPP
#define __BMI_ADAPTER_HPP

#include "bmi_data_sources.hpp"
#include "bmi_index_specifiers.hpp"
#include "bmi_indexed_by.hpp"
#include "bmi_tag.hpp"

#include "detail/bmi_implicit_base.hpp"
#include "detail/bmi_index_access.hpp"

#include <memory>

namespace bmi
{

/** Main Boost MultiIndex Adapter class which allows to expose data from multiple datasources into one unified way.
    \param DataSourceSpecifierList - instance of bmi::data_sources class which specifies list of data source classes
                                     (they shall inherit abstract class bmi::data_source). The order is significant -
                                     data coming from sources specified at begin of list take precence over those
                                     provided by subsequent data sources.
    \param Value                   - type being stored in the container. Value class must expose `id` field uniquely
                                     identifying the object in container,
    \param IndexSpecifierList      - `indexed_by` instance containing a list specifying the indices provided by this container.
*/
template <class DataSourceSpecifierList, class Value, class IndexSpecifierList, class Allocator = std::allocator<Value> >
class adapter : public detail::adapter_implicit_base<DataSourceSpecifierList, Value, IndexSpecifierList, Allocator>::type
  {
  typedef typename detail::adapter_implicit_base<DataSourceSpecifierList, Value, IndexSpecifierList, Allocator>::type super;
  typedef typename adapter<DataSourceSpecifierList, Value, IndexSpecifierList, Allocator> this_class;

  public:
    typedef IndexSpecifierList index_specifier_type_list;
    typedef Value value_type;
    typedef Allocator allocator_type;

    typedef typename super::iterator iterator;
    typedef typename super::const_iterator const_iterator;
    typedef typename super::reverse_iterator reverse_iterator;
    typedef typename super::const_reverse_iterator const_reverse_iterator;

  /// Class construction 
    adapter() = default;
    adapter(const adapter&) = delete;
    adapter(adapter&&) = delete;
    adapter& operator=(const adapter&) = delete;
    adapter& operator=(adapter&&) = delete;

  /// Explicit access to the defined indices
    template <typename Tag>
    typename detail::index_by_tag<this_class, Tag>::type& get()
      {
      /// TO BE IMPLEMENTED
      }

    template <typename Tag>
    const typename detail::index_by_tag<this_class, Tag>::type& get() const
      {
      /// TO BE IMPLEMENTED
      }

    template <int N>
    typename detail::index_by_pos<this_class, N>::type& get()
      {
      /// TO BE IMPLEMENTED
      }

    template <int N>
    const typename detail::index_by_pos<this_class, N>::type& get() const
      {
      /// TO BE IMPLEMENTED
      }

  private:
    /// Implement DataSource instantiation
    /// Implement helper traits to choose optimal version of iterator (simple or concatenation one)
  };

} /// namespace bmi

#endif /// __BMI_ADAPTER_HPP

