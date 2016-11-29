
#include <graphene/schema/schema.hpp>

#include <set>

namespace graphene { namespace schema {

namespace detail {

int64_t _next_schema_id()
{
   static int64_t _next_id = 1;
   return _next_id++;
}

}

void add_dependent_schemas( std::vector< std::shared_ptr< abstract_schema > >& schema_list )
{
   std::vector< std::shared_ptr< abstract_schema > > to_process;
   std::vector< std::shared_ptr< abstract_schema > > result;
   std::set< int64_t > has_types;

   for( std::shared_ptr< abstract_schema > s : schema_list )
      to_process.push_back( s );

   size_t i = 0;
   while( i < to_process.size() )
   {
      std::shared_ptr< abstract_schema > s = to_process[i++];
      std::string s_name;
      s->get_name( s_name );

      int64_t id = s->get_id();
      if( has_types.find( id ) != has_types.end() )
         continue;

      has_types.insert( id );
      result.push_back( s );
      s->get_deps( to_process );
   }

   schema_list.swap(result);
}

} }
