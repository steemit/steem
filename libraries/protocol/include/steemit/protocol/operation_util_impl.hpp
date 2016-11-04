
#pragma once

#include <steemit/protocol/operation_util.hpp>

#include <fc/static_variant.hpp>

namespace fc
{
   using namespace steemit::protocol;

   std::string name_from_type( const std::string& type_name );

   struct from_operation
   {
      variant& var;
      from_operation( variant& dv )
         : var( dv ) {}

      typedef void result_type;
      template<typename T> void operator()( const T& v )const
      {
         auto name = name_from_type( fc::get_typename< T >::name() );
         var = variant( std::make_pair( name, v ) );
      }
   };

   struct get_operation_name
   {
      string& name;
      get_operation_name( string& dv )
         : name( dv ) {}

      typedef void result_type;
      template< typename T > void operator()( const T& v )const
      {
         name = name_from_type( fc::get_typename< T >::name() );
      }
   };
}

namespace steemit { namespace protocol {

struct operation_validate_visitor
{
   typedef void result_type;
   template<typename T>
   void operator()( const T& v )const { v.validate(); }
};

struct operation_get_required_auth_visitor
{
   typedef void result_type;

   flat_set< account_name_type >&        active;
   flat_set< account_name_type >&        owner;
   flat_set< account_name_type >&        posting;
   std::vector< authority >&  other;

   operation_get_required_auth_visitor(
         flat_set< account_name_type >& a,
         flat_set< account_name_type >& own,
         flat_set< account_name_type >& post,
         std::vector< authority >& oth )
      : active( a ), owner( own ), posting( post ), other( oth ) {}

   template< typename T >
   void operator()( const T& v )const
   {
      v.get_required_active_authorities( active );
      v.get_required_owner_authorities( owner );
      v.get_required_posting_authorities( posting );
      v.get_required_authorities( other );
   }
};

} } // steemit::protocol

//
// Place DEFINE_OPERATION_TYPE in a .cpp file to define
// functions related to your operation type
//
#define DEFINE_OPERATION_TYPE( OperationType )                             \
namespace fc {                                                             \
                                                                           \
void to_variant( const OperationType& var,  fc::variant& vo )              \
{                                                                          \
   var.visit( from_operation( vo ) );                                      \
}                                                                          \
                                                                           \
void from_variant( const fc::variant& var,  OperationType& vo )            \
{                                                                          \
   static std::map<string,uint32_t> to_tag = []()                          \
   {                                                                       \
      std::map<string,uint32_t> name_map;                                  \
      for( int i = 0; i < OperationType::count(); ++i )                    \
      {                                                                    \
         OperationType tmp;                                                \
         tmp.set_which(i);                                                 \
         string n;                                                         \
         tmp.visit( get_operation_name(n) );                               \
         name_map[n] = i;                                                  \
      }                                                                    \
      return name_map;                                                     \
   }();                                                                    \
                                                                           \
   auto ar = var.get_array();                                              \
   if( ar.size() < 2 ) return;                                             \
   if( ar[0].is_uint64() )                                                 \
      vo.set_which( ar[0].as_uint64() );                                   \
   else                                                                    \
   {                                                                       \
      auto itr = to_tag.find(ar[0].as_string());                           \
      FC_ASSERT( itr != to_tag.end(), "Invalid operation name: ${n}", ("n", ar[0]) ); \
      vo.set_which( to_tag[ar[0].as_string()] );                           \
   }                                                                       \
      vo.visit( fc::to_static_variant( ar[1] ) );                          \
   }                                                                       \
}                                                                          \
                                                                           \
namespace steemit { namespace protocol {                                      \
                                                                           \
void operation_validate( const OperationType& op )                         \
{                                                                          \
   op.visit( steemit::protocol::operation_validate_visitor() );               \
}                                                                          \
                                                                           \
void operation_get_required_authorities( const OperationType& op,          \
                                         flat_set< account_name_type >& active,         \
                                         flat_set< account_name_type >& owner,          \
                                         flat_set< account_name_type >& posting,        \
                                         std::vector< authority >& other )     \
{                                                                          \
   op.visit( steemit::protocol::operation_get_required_auth_visitor( active, owner, posting, other ) ); \
}                                                                          \
                                                                           \
} } /* steemit::protocol */
