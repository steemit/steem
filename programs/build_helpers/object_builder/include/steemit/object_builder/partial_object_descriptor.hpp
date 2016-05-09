#pragma once

#include <string>
#include <vector>

#include <fc/reflect/reflect.hpp>

namespace steemit { namespace object_builder {

struct partial_object_descriptor
{
   std::string name;
   std::string hpp;
   std::vector< std::pair< std::string, std::string > > indexes;
};

} }

FC_REFLECT( steemit::object_builder::partial_object_descriptor,
   (name)
   (hpp)
   (indexes)
)
