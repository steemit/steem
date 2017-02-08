#pragma once

#include <graphene/db2/database.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/interprocess/containers/vector.hpp>

#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>

namespace steemit {
    namespace chain2 {

        using namespace boost::multi_index;
        namespace db2 = graphene::db2;
        namespace bip = boost::interprocess;
        using db2::allocator;

        typedef uint64_t options_type;

        typedef bip::vector<char, allocator < char> >
        buffer_type;
        typedef buffer_type packed_operation_type;
        typedef bip::vector<packed_operation_type, typename packed_operation_type::allocator_type> packed_operations_type;

        enum object_types {
            block_object_type,
            transaction_object_type,
            account_object_type
        };

    }
}
