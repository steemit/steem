
#include <boost/algorithm/string/predicate.hpp>

#include <fc/exception/exception.hpp>

#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>

#include <fc/reflect/variant.hpp>

#include <fc/filesystem.hpp>
#include <fc/strutil.hpp>

#include <steemit/object_builder/builder_state.hpp>
#include <steemit/object_builder/full_object_descriptor_context.hpp>
#include <steemit/object_builder/full_object_descriptor.hpp>
#include <steemit/object_builder/partial_object_descriptor.hpp>

namespace steemit { namespace object_builder {

using boost::algorithm::ends_with;

builder_state::builder_state()
{
   return;
}

full_object_descriptor& builder_state::core_descriptor_for( const std::string& name )
{
   std::string basename = fc::rsplit2( name, "::" ).second;
   auto it = name_to_fdesc.find( basename );
   if( it != name_to_fdesc.end() )
      return it->second;
   name_to_fdesc.emplace( basename, full_object_descriptor(basename) );
   full_object_descriptor& result = name_to_fdesc.find( basename )->second;
   result.add_index( std::pair< std::string, std::string >( "", "ordered_unique< tag< graphene::db::by_id >, member< object, object_id_type, &object::id > >" ) );
   return result;
}

void builder_state::add_partial_descriptor( const partial_object_descriptor& pdesc )
{
   full_object_descriptor& fdesc = core_descriptor_for( pdesc.name );
   fdesc.add_partial_descriptor( pdesc );
   return;
}

void builder_state::process_directory( const fc::path& dirname )
{
   fc::directory_iterator it = fc::directory_iterator( dirname );
   while( it != fc::directory_iterator() )
   {
      fc::path p = *it;
      if( !ends_with(p.generic_string(), ".object") )
         continue;

      fc::variant v;
      try
      {
         v = fc::json::from_file( p, fc::json::relaxed_parser );
      }
      catch( const fc::parse_error_exception& exc )
      {
         elog( "Error parsing JSON file ${p}", ("p", p) );
         throw;
      }
      partial_object_descriptor pdesc;
      fc::from_variant( v, pdesc );
      add_partial_descriptor( pdesc );
      ++it;
   }
   return;
}

void builder_state::generate( const fc::path& output_dirname )
{
   if( !fc::exists( output_dirname ) )
      fc::create_directories( output_dirname );

   for( const std::pair< std::string, full_object_descriptor >& name_fdesc : name_to_fdesc )
   {
      const full_object_descriptor& fdesc = name_fdesc.second;
      fc::path output_path = output_dirname / (fdesc.basename+".hpp");
      std::string content = fc::format_string( output_template, build_context_variant( fdesc ) );
      fc::ofstream of( output_path );
      of.write( content.c_str(), content.size() );
   }
}

} }
