#include <boost/test/unit_test.hpp>

#include <chainbase/util/ra_indexed_container.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <random>
#include <string>

using namespace boost::multi_index;

namespace /// namespace anonymous
{
struct test_object
{
   template <class Constructor, class Allocator>
   test_object(Constructor &&c, size_t _id, Allocator a) : name(a),
                                                            id(_id)
   {
      c(*this);
   }

   std::string name;
   chainbase::oid<test_object> id;
};

std::ostream &operator<<(std::ostream &os, const test_object &a)
{
   os << '{';
   os << a.id;
   os << ", `";
   os << a.name;
   os << "'}";

   return os;
}

struct OrderedIndex;
struct CompositeOrderedIndex;

template <bool ReuseIndices>
struct TContainer
{
   typedef chainbase::ra_indexed_container<test_object,
   indexed_by<
      ordered_unique<
            tag<OrderedIndex>, member<test_object, std::string, &test_object::name>>,
      ordered_unique< tag< CompositeOrderedIndex >,
            composite_key< test_object,
               member< test_object, std::string, &test_object::name >,
               member< test_object, chainbase::oid<test_object>, &test_object::id >
               >
            >
      >,
   std::allocator<test_object>,
   ReuseIndices
   >
   type;
};

static const std::vector<std::string> s_names =
      {"Alice", "has", "a", "cat", "but", "the", "cat", "has", "insects"};

template <typename TContainer>
void verifyContainer(const TContainer &x)
{
   typedef typename TContainer::value_type TContainerValueType;

   std::cout << "Container size:" << x.size() << std::endl;
   std::cout << "Performing dump using direct container:" << std::endl;
   std::copy(x.cbegin(), x.cend(), std::ostream_iterator<TContainerValueType>(std::cout, "\n"));

   const auto &orderedIdx = x.template get<OrderedIndex>();

   std::cout << "Performing dump using ordered index:" << std::endl;
   std::copy(orderedIdx.cbegin(), orderedIdx.cend(), std::ostream_iterator<TContainerValueType>(
                                                         std::cout, "\n"));

   std::cout << "Performing lookup test..." << std::endl;
   for (const auto &a : x)
   {
      auto foundI = orderedIdx.find(a.name);
      BOOST_REQUIRE(foundI != orderedIdx.end());
      std::cout << "Name: `" << a.name << "' points to object: " << *foundI << std::endl;
   }

   std::string brokenName = s_names.front();
   brokenName += "_broken";

   auto foundI = orderedIdx.find(brokenName);
   BOOST_REQUIRE(foundI == orderedIdx.end());

   std::vector<size_t> validRandomIds;
   std::transform(x.cbegin(), x.cend(), std::back_inserter(validRandomIds),
                  [](const TContainerValueType &obj) -> size_t {
                     return obj.id;
                  });

   std::cout << "Performing random-access test..." << std::endl;
   for (const auto &id : validRandomIds)
   {
      const test_object *object = x[id];
      BOOST_REQUIRE(object != nullptr);

      std::cout << "RA index : `" << id << "' points to object: " << *object << std::endl;
   }
}

template <typename TContainer>
void fillContainer(TContainer * c, size_t limit = 0 /*0 means no limit*/)
{
   size_t counter = 0;
   for (const auto &name : s_names)
   {
      auto constructor = [&name](test_object &obj) {
         obj.name = name;
      };

      c->emplace(constructor);
      if(++counter == limit)
         break;
   }
}

template <typename TContainer>
size_t eraseFromContainer(TContainer * c, size_t idToRemove)
{
   const typename TContainer::value_type *object = (*c)[idToRemove];
   auto pointingIterator = c->iterator_to(*object);
   BOOST_REQUIRE(pointingIterator != c->end());

   size_t removedId = object->id;
   BOOST_REQUIRE(removedId == idToRemove);

   std::cout << "Attempting to erase object pointed by id: " << idToRemove << ", object: "
               << *object << std::endl;

   c->erase(pointingIterator);
   return removedId;
}

template <typename TContainer>
void clearContainer(TContainer * c)
{
   c->clear();
}

void printTestTitle(const char *title)
{
   std::string frame(strlen(title) + 4 + 4, '#');
   std::cout << std::endl
               << frame << std::endl;
   std::cout << "##  " << title << "  ##" << std::endl;
   std::cout << frame << std::endl;
}

template <bool ReuseIndices>
void basic_test()
{
   std::vector<test_object> sourceContainer;
   std::allocator<test_object> stringAlloc;
   /// tests for various constructors originally defined in multi_index_container
   typedef typename TContainer<ReuseIndices>::type TTestContainer;
   TTestContainer x, y(stringAlloc);

   std::string title("Testing with");
   title += ReuseIndices ? " " : "out ";
   title += "indices reusing...";
   printTestTitle(title.c_str());

   printTestTitle("Container initial test...");

   fillContainer(&x);
   fillContainer(&y);

   verifyContainer(x);

   printTestTitle("Container clear test...");

   std::vector<size_t> validIds, newIds;
   std::transform(y.cbegin(), y.cend(), std::back_inserter(validIds),
                  [](const typename TTestContainer::value_type &obj) -> size_t {
                     return obj.id;
                  });

   y.clear();
   verifyContainer(y);
   std::cout << "Attempting to fill container again..." << std::endl;
   fillContainer(&y);
   verifyContainer(y);
   std::transform(y.cbegin(), y.cend(), std::back_inserter(newIds),
                  [](const typename TTestContainer::value_type &obj) -> size_t {
                     return obj.id;
                  });

   /// All ids shall be reused
   BOOST_REQUIRE(validIds == newIds);

   printTestTitle("Container erasure test...");

   std::default_random_engine generator;
   std::uniform_int_distribution<size_t> distribution(1, x.size());
   size_t idToRemove = distribution(generator);

   size_t removedId = eraseFromContainer(&x, idToRemove);
   verifyContainer(x);

   printTestTitle("Container insertion after erasure test...");

   auto constructor = [](test_object &obj) {
      std::string newName(s_names.front());
      newName += "-";
      newName += s_names.back();

      obj.name = newName;
   };

   auto ii = x.emplace(constructor);

   BOOST_REQUIRE(ii.second);                                                           /// Element must be inserted
   BOOST_REQUIRE(ReuseIndices ? ii.first->id._id == removedId : ii.first->id._id > removedId); /// Id shall be reused

   verifyContainer(x);

   printTestTitle("Add 3, remove 2nd, add 1, container test...");
   clearContainer(&x);
   fillContainer(&x, 3);
   removedId = eraseFromContainer(&x, 2);

   ii = x.emplace(constructor);

   BOOST_REQUIRE(ii.second);                                                           /// Element must be inserted
   BOOST_REQUIRE(ReuseIndices ? ii.first->id._id == removedId : ii.first->id._id > removedId); /// Id shall be reused
   
   verifyContainer(x);
}

} /// namespace anonymous

BOOST_AUTO_TEST_SUITE(ra_indexed_container_tests)

BOOST_AUTO_TEST_CASE(basic_tests)
{
   basic_test<true>();
   basic_test<false>();
}

BOOST_AUTO_TEST_SUITE_END()
