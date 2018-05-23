#ifndef __BMI_DATA_SOURCE_HPP
#define __BMI_DATA_SOURCE_HPP

#include <stddef.h>
#include <string>
#include <xutility>

namespace bmi
{

/** Represents an abstract base class for each data source class being specified for bmi::adapter class.
    There can be 2 kinds of data sources:
    - pointing to persistent storage, where there is no changed/new/removed (not-yet-committed) objects,
    - pointing to the volatile state storage, where are only changed/new/removed objects

    The order of data_sources specified on the bmi::data_sources list is significant. The earlier specified
    data source can hide/replace objects retrieved from subsuequent data sources, to correctly adjust incomming, not yet
    committed changes.

    The bmi::adapter client shall inherit its own datasource class from this one, reimplement all pure methods and what
    more important, override in the subclass template methods having empty implementations here.

    Also every subclass must provide static method which allows to instantiate given data_source subclass i.e.:

    class volatile_state_data_source : public data_source<account_object, vs_storage_iterator>
    {
      static volatile_state_data_source* instance();
    };
*/
template <class Value, class Iterator, class ReverseIterator = std::reverse_iterator<Iterator> > 
class data_source
  {
  public:
    /** This method allows to verify at bmi::adapter startup time all indices being defined by the container.
        Actual implementation shall take care for matching given index list against exiting ones or creating new indices
        in the storage.

        \param IndexSpecifier - the class describing the index, offering:
        - key_from_value,
        - key_compare,
        - tag_list_type

        types.
    */
    template <typename... IndexSpecifier>
    void verify_defined_indices()
      {
      /// Nothing to do here. Override this method in subclass if needed.
      }

  /// Contents traversal:
    /** Allows to traverse data source by preserving an order specific to given index.
    */
    template <typename IndexSpecifier>
    Iterator begin() const
      {
      /// Needs to be reimplemented in the subclass.
      assert(false);
      return Iterator();
      }

    template <typename IndexSpecifier>
    Iterator end() const
      {
      /// Needs to be reimplemented in the subclass.
      assert(false);
      return Iterator();
      }

    template <typename IndexSpecifier>
    ReverseIterator rbegin() const
      {
      /// Needs to be reimplemented in the subclass.
      assert(false);
      return Iterator();
      }

    template <typename IndexSpecifier>
    ReverseIterator rend() const
      {
      /// Needs to be reimplemented in the subclass.
      assert(false);
      return Iterator();
      }

  /// Lookup:
    template <typename IndexSpecifier>
    Iterator find(const typename IndexSpecifier::key_from_value::result_type& key) const
      {
      /// Needs to be reimplemented in the subclass.
      assert(false);
      return Iterator();
      }

    template <typename IndexSpecifier>
    Iterator lower_bound(Iterator start, const typename IndexSpecifier::key_from_value::result_type& key) const
      {
      /// Needs to be reimplemented in the subclass.
      assert(false);
      return Iterator();
      }

    template <typename IndexSpecifier>
    Iterator upper_bound(const typename IndexSpecifier::key_from_value::result_type& key) const
      {
      /// Needs to be reimplemented in the subclass.
      assert(false);
      return Iterator();
      }

  /// Insertion/Modification:
    /// TO BE IMPLEMENTED
    /// define methods for inserting/modification, incl. unique index conflict detection

  /// Removal:
    virtual Iterator erase(Iterator pos) = 0;

  /// Other helper services:
    /// Allows to filter out object during iteration
    virtual bool is_modified(const Value& object) const = 0;
    /// Allows to filter out object during iteration
    virtual bool is_removed(const Value& object) const = 0;

    /// Returns true if given data source is empty.
    virtual bool is_empty() const = 0;
    /** Allows to retrieve numbers of:
       - total committed objects (function retval)
       - just created, not yet committed objects
       - just deleted, not yet committed objects.
    */
    virtual size_t count_objects(size_t* newUncommitted, size_t* removedUncommitted) const = 0;

  protected:
    virtual ~data_source() {}
  };

} /// namespace bmi

#endif /// __BMI_DATA_SOURCE_HPP

