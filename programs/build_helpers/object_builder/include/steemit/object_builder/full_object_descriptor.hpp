#pragma once

#include <set>
#include <utility>
#include <vector>

#include <fc/reflect/reflect.hpp>

namespace steemit { namespace object_builder {

struct partial_object_descriptor;

struct full_object_descriptor
{
   full_object_descriptor( const std::string& _basename );

   std::string basename;

   std::set< std::string > index_name_set;
   std::vector< std::pair< std::string, std::string > > indexes;
   std::vector< std::string > hpps;
   std::set< std::string > namespace_set;
   std::vector< std::string > namespaces;

   void add_partial_descriptor( const partial_object_descriptor& pdesc );
   void add_index( const std::pair< std::string, std::string >& index );
   void add_ns( const std::string& ns );
};

} }
