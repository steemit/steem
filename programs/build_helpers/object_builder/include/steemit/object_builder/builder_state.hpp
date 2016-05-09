#pragma once

#include <map>
#include <string>

#include <steemit/object_builder/full_object_descriptor.hpp>

namespace fc {
class path;
}

namespace steemit { namespace object_builder {

struct partial_object_descriptor;

class builder_state
{
   public:
      builder_state();
      void process_directory( const fc::path& dirname );
      full_object_descriptor& core_descriptor_for( const std::string& name );
      void add_partial_descriptor( const partial_object_descriptor& pdesc );

      void generate( const fc::path& output_dirname );

      std::map< std::string, full_object_descriptor > name_to_fdesc;
      std::string output_template;
};

} }
