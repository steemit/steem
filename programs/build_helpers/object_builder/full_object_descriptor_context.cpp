
#include <boost/algorithm/string/predicate.hpp>

#include <steemit/object_builder/full_object_descriptor.hpp>

#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>

#include <fc/variant.hpp>
#include <fc/variant_object.hpp>

#include <string>
#include <utility>

namespace steemit { namespace object_builder {

struct full_object_descriptor_context
{
   std::string subobject_includes;
   std::string object_name;
   std::string subobject_name_public_list;
   std::string subobject_name_bubble_list;
   std::string index_structs;
   std::string indexes;
   std::string object_basename;
};

} }

FC_REFLECT( steemit::object_builder::full_object_descriptor_context,
   (subobject_includes)
   (object_name)
   (subobject_name_public_list)
   (subobject_name_bubble_list)
   (index_structs)
   (indexes)
   (object_basename)
)

namespace steemit { namespace object_builder {

using boost::algorithm::ends_with;

full_object_descriptor_context build_context( const full_object_descriptor& fdesc )
{
   full_object_descriptor_context ctx;

   const std::string& basename = fdesc.basename;
   ctx.object_name = basename;
   if( ends_with( basename, "_object" ) )
      ctx.object_basename = basename.substr(0, basename.size()-7);
   ctx.subobject_name_public_list = "";
   bool first = true;
   for( const std::string& ns : fdesc.namespaces )
   {
      if( first )
         first = false;
      else
         ctx.subobject_name_public_list += ", ";
      ctx.subobject_name_public_list += "public "+ns+"::"+basename;
      ctx.subobject_name_bubble_list += "   ("+ns+"::"+basename+")\n";
   }

   for( const std::string& hpp : fdesc.hpps )
   {
      ctx.subobject_includes += "#include <";
      ctx.subobject_includes += hpp;
      ctx.subobject_includes += ">\n";
   }

   first = true;
   for( const std::pair< std::string, std::string >& index : fdesc.indexes )
   {
      if( index.first != "" )
      {
         ctx.index_structs += "struct ";
         ctx.index_structs += index.first;
         ctx.index_structs += ";\n";
      }
      if( first )
         first = false;
      else
         ctx.indexes += ",\n";
      ctx.indexes += index.second;
   }
   return ctx;
}

fc::variant_object build_context_variant( const full_object_descriptor& fdesc )
{
   fc::variant v;
   to_variant( build_context(fdesc), v );
   return v.get_object();
}

} }
