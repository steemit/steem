#ifndef __RA_INDEXED_CONTAINER_HPP
#define __RA_INDEXED_CONTAINER_HPP

#if !defined(NDEBUG)
#define BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING
#define BOOST_MULTI_INDEX_ENABLE_SAFE_MODE
#endif

#include <boost/multi_index_container.hpp>

#include <cstddef>
#include <vector>

namespace steem
{

  namespace chain
  {
    /** Dedicated class providing functionality of random_access (via index assigned at object emplacement)
    and fast lookup, as boost::multi_index::multi_index_container does.
    Supports safe object removal and reusing of previously assigned index.
    */
    template <typename Value, typename IndexSpecifierList, typename Allocator = std::allocator<Value>>
    class ra_indexed_container :
      protected boost::multi_index::multi_index_container<Value, IndexSpecifierList, Allocator>
    {
      typedef ::boost::multi_index::multi_index_container<Value, IndexSpecifierList, Allocator> base_class;

    public:
      typedef typename base_class::ctor_args_list ctor_args_list;
      typedef typename base_class::index_specifier_type_list index_specifier_type_list;
      typedef typename base_class::index_type_list index_type_list;

      typedef typename base_class::iterator_type_list iterator_type_list;
      typedef typename base_class::const_iterator_type_list const_iterator_type_list;
      typedef typename base_class::value_type value_type;
      typedef typename base_class::allocator_type allocator_type;
      typedef typename base_class::iterator iterator;
      typedef typename base_class::const_iterator const_iterator;

      template <typename Y>
      using index=typename base_class::template index<Y>;

      using base_class::multi_index_container;
      using base_class::cbegin;
      using base_class::cend;
      using base_class::begin;
      using base_class::end;
      using base_class::size;
      using base_class::iterator_to;
      using base_class::find;

      const value_type *operator[](size_t idx) const
      {
        assert(idx < RandomAccessStorage.size());
        /// \warning RA container can have gaps caused by element removal.
        if (RandomAccessStorage[idx] != end())
        {
          const auto &iterator = RandomAccessStorage[idx];
          const value_type &object = *iterator;
          return &object;
        }

        return nullptr;
      }

      template <typename Tag>
      const typename index<Tag>::type &get() const
      {
        return base_class::template get<Tag>();
      }

      template <typename Constructor>
      std::pair<iterator, bool> emplace(Constructor &&c)
      {
        std::size_t index = retrieveNextId();
        auto allocator = this->get_allocator();

        auto ii = base_class::emplace(c, index, allocator);

        if (ii.second)
        {
          assert(index == getIndex(*ii.first) && "Built object must have assigned actual id");

          if (RandomAccessStorage.size() == index)
            RandomAccessStorage.push_back(ii.first);
          else
            RandomAccessStorage[index] = ii.first;

          if (FreeIndices.empty() == false)
          {
            assert(FreeIndices.back() == index);
            FreeIndices.pop_back();
          }
        }

        return std::move(ii);
      }

      iterator erase(iterator position)
      {
        const value_type &object = *position;
        size_t index = getIndex(object);
        if (index <= this->size() - 1)
          FreeIndices.emplace_back(index);

        RandomAccessStorage[index] = this->cend();

        return base_class::erase(position);
      }

      void clear()
      {
        FreeIndices.clear();
        RandomAccessStorage.clear();
        RandomAccessStorage.push_back(end());
        base_class::clear();
      }

    private:
      /** Allows to query for the next random access identifier to be assigned to the new object being
      constructed & stored in this container.
      */
      std::size_t retrieveNextId() const
      {
        return FreeIndices.empty() ? this->size() + 1 : FreeIndices.back();
      }

      std::size_t getIndex(const value_type &object) const
      {
        return object.id;
      }

      /// Class members:
    private:
      typedef std::vector<std::size_t, Allocator> IndicesContainer;
      typedef std::vector<typename base_class::const_iterator, Allocator> RandomAccessContainer;
      /// Stores RA-indexes of elemements being removed from container (if they didn't placed at the end of container).
      IndicesContainer FreeIndices;
      /** If at given position element has been erased, held iterator == end()
      \warning Size of this container can be bigger than size of main multi_index_container, since this one can
      store gaps previously pointing to erased objects. Such gaps will be reused an subsequent insertion.
      At the 0th element stores null element, to conform NULL_INDEX schema in the ptr_ref class.
      */
      RandomAccessContainer RandomAccessStorage = RandomAccessContainer(1);
    };

   /** Helper trait useful to determine if specified boost::multi_index_container has defined
         MasterIndexTag (pointing to random_access index holding objects uniquely identified by their
         position in the sequence)
   */
   template<typename MultiIndexType>
   struct has_master_index
   {
     typedef typename std::is_same<MultiIndexType, steem::chain::ra_indexed_container<
       typename MultiIndexType::value_type,
       typename MultiIndexType::index_specifier_type_list,
       typename MultiIndexType::allocator_type>>::type type;
     enum { value = type::value };
   };
    
  } /// namespace chain

} /// namespace steem

#endif /// __RA_INDEXED_CONTAINER_HPP

