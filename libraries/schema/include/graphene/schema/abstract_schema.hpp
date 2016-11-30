
#pragma once

#include <memory>
#include <string>

#include <fc/reflect/reflect.hpp>

namespace graphene { namespace schema {

struct abstract_schema
{
   virtual void get_deps( std::vector< std::shared_ptr< abstract_schema > >& deps ) = 0;
   virtual void get_name( std::string& name ) = 0;
   virtual void get_str_schema( std::string& s ) = 0;
   virtual int64_t get_id() = 0;
};

namespace detail {

template<typename ObjectType >
std::shared_ptr< abstract_schema > create_schema( int64_t id );

int64_t _next_schema_id();

// This class is an implementation detail, don't worry about it.
template<
   typename ObjectType,
   bool is_reflected = fc::reflector<ObjectType>::is_defined::value,
   bool is_enum = fc::reflector<ObjectType>::is_enum::value
   >
struct schema_impl;

}

template< typename ObjectType >
struct schema_reflect
{
   typedef detail::schema_impl< ObjectType >        schema_impl_type;
};

template< typename ObjectType >
std::shared_ptr< abstract_schema > get_schema_for_type()
{
   static std::shared_ptr< abstract_schema > sch = std::shared_ptr< abstract_schema >();
   if( !sch )
   {
      sch = detail::create_schema<ObjectType>( detail::_next_schema_id() );
   }
   return sch;
}

void add_dependent_schemas( std::vector< std::shared_ptr< abstract_schema > >& schema_list );

} } // graphene::schema
