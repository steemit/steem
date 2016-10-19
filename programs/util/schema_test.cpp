#include <graphene/db/schema.hpp>
#include <graphene/db/schema_impl.hpp>
#include <graphene/db/schema_types.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <steemit/chain/account_object.hpp>
#include <steemit/chain/steem_objects.hpp>

using namespace graphene::db;

struct mystruct
{
   std::string alpha;
   uint64_t beta;
};

FC_REFLECT( mystruct,
   (alpha)
   (beta)
   );

void process( std::shared_ptr< abstract_schema > s )
{
   std::string name;
   s->get_name(name);
   std::cout << "   name = " << name;
   std::vector< std::shared_ptr< abstract_schema > > deps;
   s->get_deps(deps);
   std::vector< std::string > dep_names;
   for( const std::shared_ptr< abstract_schema >& s : deps )
   {
      std::string s_name;
      s->get_name( s_name );
      dep_names.push_back( s_name );
   }
   std::cout << "   deps = " << fc::json::to_string(dep_names);
   std::string str_schema;
   s->get_str_schema( str_schema );
   std::cout << "   s = " << str_schema;
   std::cout << std::endl;
}

int main( int argc, char** argv, char** envp )
{
   std::vector< std::shared_ptr< abstract_schema > > schemas;

   schemas.push_back( get_schema_for_type< steemit::chain::account_object >() );
   schemas.push_back( get_schema_for_type< steemit::chain::comment_object >() );
   add_dependent_schemas( schemas );

   for( const std::shared_ptr< abstract_schema >& s : schemas )
   {
      process( s );
   }

   return 0;
}
