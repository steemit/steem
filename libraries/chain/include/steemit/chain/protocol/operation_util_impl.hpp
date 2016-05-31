
#pragma once

#include <steemit/chain/protocol/operation_util.hpp>

#include <fc/static_variant.hpp>

namespace steemit { namespace chain {

std::string name_from_type( const std::string& type_name );

struct operation_to_variant_visitor
{
   variant& var;
   operation_to_variant_visitor( variant& dv ):var(dv){}

   typedef void result_type;
   template<typename T> void operator()( const T& v )const
   {
      auto name = name_from_type( fc::get_typename<T>::name() );
      var = variant( std::make_pair(name,v) );
   }
};

struct get_operation_name_visitor
{
   string& name;
   get_operation_name_visitor( string& dv ):name(dv){}

   typedef void result_type;
   template<typename T> void operator()( const T& v )const
   {
      name = name_from_type( fc::get_typename<T>::name() );
   }
};

template< typename OperationType >
void abstract_operation_to_variant( const OperationType& o,  fc::variant& var )
{
   o.visit( operation_to_variant_visitor( var ) );
}

template< typename OperationType >
void abstract_operation_from_variant( const fc::variant& var,  OperationType& o )
{
   static std::map<string,uint32_t> to_tag = []()
   {
      std::map<string,uint32_t> name_map;
      for( int i = 0; i < OperationType::count(); ++i )
      {
         OperationType tmp;
         tmp.set_which(i);
         string n;
         tmp.visit( get_operation_name_visitor(n) );
         name_map[n] = i;
      }
      return name_map;
   }();

   auto ar = var.get_array();
   if( ar.size() < 2 ) return;
   if( ar[0].is_uint64() )
      o.set_which( ar[0].as_uint64() );
   else
   {
      auto itr = to_tag.find(ar[0].as_string());
      FC_ASSERT( itr != to_tag.end(), "Invalid operation name: ${n}", ("n", ar[0]) );
      o.set_which( to_tag[ar[0].as_string()] );
   }
   o.visit( fc::to_static_variant( ar[1] ) );
}

struct operation_validate_visitor
{
   typedef void result_type;
   template<typename T>
   void operator()( const T& v )const { v.validate(); }
};

struct operation_get_required_auth_visitor
{
   typedef void result_type;

   flat_set<string>&    active;
   flat_set<string>&    owner;
   flat_set<string>&    posting;
   std::vector<authority>&   other;

   operation_get_required_auth_visitor(
     flat_set<string>& a,
     flat_set<string>& own,
     flat_set<string>& post,
     std::vector<authority>&  oth ):active(a),owner(own),posting(post),other(oth){}

   template<typename T>
   void operator()( const T& v )const
   {
      v.get_required_active_authorities( active );
      v.get_required_owner_authorities( owner );
      v.get_required_posting_authorities( posting );
      v.get_required_authorities( other );
   }
};

} } // steemit::chain

//
// Place DEFINE_OPERATION_TYPE in a .cpp file to define
// functions related to your operation type
//
#define DEFINE_OPERATION_TYPE( OperationType )                        \
namespace fc {                                                        \
                                                                      \
void to_variant( const OperationType& o, fc::variant var )            \
{                                                                     \
   steemit::chain::abstract_operation_to_variant< OperationType >( o, var ); \
}                                                                     \
                                                                      \
void from_variant( const fc::variant& var, OperationType& o )         \
{                                                                     \
   steemit::chain::abstract_operation_from_variant< OperationType >( var, o ); \
}                                                                     \
                                                                      \
} /* fc */                                                            \
                                                                      \
namespace steemit { namespace chain {                                 \
                                                                      \
void operation_validate( const OperationType& op )                    \
{                                                                     \
   op.visit( steemit::chain::operation_validate_visitor() );          \
}                                                                     \
                                                                      \
void operation_get_required_authorities( const OperationType& op,     \
                                         flat_set<string>& active,    \
                                         flat_set<string>& owner,     \
                                         flat_set<string>& posting,   \
                                         std::vector<authority>& other ) \
{                                                                     \
   op.visit( steemit::chain::operation_get_required_auth_visitor( active, owner, posting, other ) ); \
}                                                                     \
                                                                      \
} } /* steemit::chain */
