#ifndef __RA_INDEXED_CONTAINER_HPP
#define __RA_INDEXED_CONTAINER_HPP

#include <chainbase/util/object_id.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>

#include <boost/throw_exception.hpp>

#include <cstddef>
#include <vector>

namespace chainbase
{
   using boost::multi_index::indexed_by;
   using boost::multi_index::hashed_non_unique;
   using boost::multi_index::hashed_unique;
   using boost::multi_index::ordered_unique;
   using boost::multi_index::ordered_non_unique;
   using boost::multi_index::random_access;
   using boost::multi_index::sequenced;

/** Dedicated class providing functionality of random_access (via index assigned at object emplacement)
and fast lookup, as boost::multi_index::multi_index_container does.
Supports safe object removal and reusing of previously assigned index.
*/
template <typename Value, typename IndexSpecifierList, typename Allocator = std::allocator<Value>>
class ra_indexed_container : protected boost::multi_index::multi_index_container<Value,
                                       IndexSpecifierList, Allocator>
{
   typedef boost::multi_index::multi_index_container<Value, IndexSpecifierList, Allocator> base_class;

public:
   using typename base_class::ctor_args_list;
   using typename base_class::index_specifier_type_list;
   using typename base_class::index_type_list;

   using typename base_class::iterator_type_list;
   using typename base_class::const_iterator_type_list;
   using typename base_class::value_type;
   using typename base_class::allocator_type;
   using typename base_class::iterator;
   using typename base_class::const_iterator;

   template <typename Y>
   using index=typename base_class::template index<Y>;

   using typename base_class::node_type;
  
   using base_class::cbegin;
   using base_class::cend;
   using base_class::begin;
   using base_class::end;
   using base_class::size;
   using base_class::iterator_to;
   using base_class::get_allocator;

   explicit ra_indexed_container(const allocator_type& alloc = allocator_type()) :
      base_class(alloc),
      FreeIndices(alloc),
      RandomAccessStorage(1, alloc)
   {
   }

   ra_indexed_container(const ra_indexed_container&) = default;
   ra_indexed_container& operator=(const ra_indexed_container&) = default;
   
   ra_indexed_container(ra_indexed_container&&) = default;
   ra_indexed_container& operator=(ra_indexed_container&&) = default;
   
   const value_type *operator[](size_t idx) const
   {
      if( idx >= RandomAccessStorage.size() ) BOOST_THROW_EXCEPTION( std::out_of_range("index not found") );
      /// \warning RA container can have gaps caused by element removal.
      if (RandomAccessStorage[idx] != end())
      {
         const auto &iterator = RandomAccessStorage[idx];
         const value_type &object = *iterator;
         return &object;
      }

      return nullptr;
   }

   const_iterator find(const oid<value_type>& id) const
   {
      std::size_t idx = id;
      if(idx >= RandomAccessStorage.size()) BOOST_THROW_EXCEPTION( std::out_of_range("id not found") );

      return RandomAccessStorage[idx];
   }

   template <typename Tag>
   const typename index<Tag>::type &get() const
   {
      return base_class::template get<Tag>();
   }

   template <typename Modifier>
   bool modify(iterator position, Modifier mod)
   {
      const value_type& object = *position;
      const auto cachedRAIdentifier = getIndex(object);

      bool result = base_class::modify(position, mod);

      if(result)
      {
         const value_type& changedObject = *position;

         const auto newRAIdentifier = getIndex(changedObject);

         if(newRAIdentifier != cachedRAIdentifier)
         {
            /// Changing object identifier is disallowed !!!
            BOOST_THROW_EXCEPTION(std::runtime_error("Cannot change persistent object id"));
         }
      }

      return result;
   }

   template <typename Constructor>
   std::pair<iterator, bool> emplace(Constructor &&c)
   {
      std::size_t index = retrieveNextId();

      auto node_ii = base_class::emplace_(c, index, this->get_allocator());

      std::pair<iterator, bool> ii = std::make_pair(const_iterator(node_ii.first), node_ii.second);

      if (node_ii.second)
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

   std::pair<iterator, bool> emplace(value_type&& source)
   {
      auto actualId = retrieveNextId();
      
      /// Be sure that object moved to the container storage has set correct id
      auto constructor = [&source, actualId](value_type& newObject)
      {
         newObject = std::move(source);
         newObject.id._id = actualId;
      };

      return emplace(std::move(constructor));
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
   RandomAccessContainer RandomAccessStorage = RandomAccessContainer(1, this->get_allocator());
};

/** Helper trait useful to determine if specified boost::multi_index_container has defined
      MasterIndexTag (pointing to random_access index holding objects uniquely identified by their
      position in the sequence)
*/
template<typename MultiIndexType>
struct has_master_index
{
   typedef typename std::is_same<MultiIndexType, ra_indexed_container<
      typename MultiIndexType::value_type,
      typename MultiIndexType::index_specifier_type_list,
      typename MultiIndexType::allocator_type>>::type type;
   enum { value = type::value };
};
   
} /// namespace chainbase

#endif /// __RA_INDEXED_CONTAINER_HPP

