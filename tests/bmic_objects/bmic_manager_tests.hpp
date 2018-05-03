#pragma once

#include <boost/multi_index/composite_key.hpp>
#include <chainbase/util/object_id.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <random>
#include <string>

using namespace boost::multi_index;

namespace bmic
{
   struct test_object
   {
      template <class Constructor, class Allocator>
      test_object(Constructor&& c, Allocator a )
      : id( 0 ), val( 0 ), name( a )
      {
         c(*this);
      }

      template <class Constructor, class Allocator>
      test_object(Constructor&& c, int64_t _id, Allocator a )
      : id( _id ), val( 0 ), name( a )
      {
         c(*this);
      }

      chainbase::oid<test_object> id;
      uint32_t val;
      std::string name;
   };

   struct OrderedIndex;
   struct CompositeOrderedIndex;

   using test_object_index = boost::multi_index_container<test_object,
   indexed_by<
      ordered_unique<
            tag<OrderedIndex>, member<test_object, chainbase::oid<test_object>, &test_object::id>>,
      ordered_unique< tag< CompositeOrderedIndex >,
            composite_key< test_object,
               member< test_object, std::string, &test_object::name >,
               member< test_object, uint32_t, &test_object::val >
               >
            >
      >,
   std::allocator<test_object>
   >;

   struct test_object2
   {
      template <class Constructor, class Allocator>
      test_object2(Constructor&& c, Allocator a )
      : id( 0 ), val( 0 )
      {
         c(*this);
      }

      chainbase::oid<test_object2> id;
      uint32_t val;
   };

   struct OrderedIndex2;
   struct CompositeOrderedIndex2;

   using test_object_index2 = boost::multi_index_container<test_object2,
   indexed_by<
      ordered_unique<
            tag<OrderedIndex2>, member<test_object2, chainbase::oid<test_object2>, &test_object2::id>>,
      ordered_unique< tag< CompositeOrderedIndex2 >,
            composite_key< test_object2,
               member< test_object2, uint32_t, &test_object2::val >,
               member<test_object2, chainbase::oid<test_object2>, &test_object2::id>
               >
            >
      >,
   std::allocator<test_object2>
   >;

   struct test_object3
   {
      template <class Constructor, class Allocator>
      test_object3(Constructor&& c, Allocator a )
      : id( 0 ), val( 0 ), val2( 0 ), val3( 0 )
      {
         c(*this);
      }

      chainbase::oid<test_object3> id;
      uint32_t val;
      uint32_t val2;
      uint32_t val3;
   };

   struct OrderedIndex3;
   struct CompositeOrderedIndex3a;
   struct CompositeOrderedIndex3b;

   using test_object_index3 = boost::multi_index_container<test_object3,
   indexed_by<
      ordered_unique<
            tag<OrderedIndex3>, member<test_object3, chainbase::oid<test_object3>, &test_object3::id>>,
      ordered_unique< tag< CompositeOrderedIndex3a>,
            composite_key< test_object3,
               member< test_object3, uint32_t, &test_object3::val >,
               member< test_object3, uint32_t, &test_object3::val2 >
               >
            >,
      ordered_unique< tag< CompositeOrderedIndex3b>,
            composite_key< test_object3,
               member< test_object3, uint32_t, &test_object3::val >,
               member< test_object3, uint32_t, &test_object3::val3 >
               >
            >
      >,
   std::allocator<test_object3>
   >;
}
