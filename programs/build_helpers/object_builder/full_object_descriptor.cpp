
#include <fc/strutil.hpp>

#include <fc/exception/exception.hpp>

#include <steemit/object_builder/full_object_descriptor.hpp>
#include <steemit/object_builder/partial_object_descriptor.hpp>

namespace steemit { namespace object_builder {

full_object_descriptor::full_object_descriptor( const std::string& _basename )
{
   basename = _basename;
}

void full_object_descriptor::add_index( const std::pair< std::string, std::string >& index )
{
   // if this fails, it means multiple .object files are trying to define the same index
   bool inserted = index_name_set.insert( index.first ).second;
   wdump( (index) );
   FC_ASSERT( inserted );
   indexes.push_back( index );
   return;
}

void full_object_descriptor::add_ns( const std::string& ns )
{
   // if this fails, you're trying to redefine an already existing subobject
   // (i.e. multiple .object files with same object name in same namespace)
   bool inserted = namespace_set.insert( ns ).second;
   FC_ASSERT( inserted );
   namespaces.push_back( ns );
   return;
}

void full_object_descriptor::add_partial_descriptor( const partial_object_descriptor& pdesc )
{
   add_ns( fc::rsplit2( pdesc.name, "::" ).first );
   for( const std::pair< std::string, std::string >& idx : pdesc.indexes )
      add_index( idx );
   hpps.push_back( pdesc.hpp );
}

} }
