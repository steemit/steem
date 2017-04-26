
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
   std::vector< authority >&             other;

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

/**
 * This class implements a visitor to translate the required authorizations
 * for an operation from the legacy `authority` struct to its replacement,
 * the `auth_rule` struct.
 *
 * The effect of the visitor is to save one or more rules in the `rules` member
 * of the `get_req_authorizations_context` structure passed to the constructor.
 *
 * The first rule created is a rule for when the operation is authorized.  This
 * rule is of the form:
 *
 * ```
 * auth_1 & auth_2 & ... & auth_n -> !op
 * ```
 *
 * where `auth_1, auth_2, ..., auth_n` are the authorizations corresponding to
 * the required authorities.  (The conjunction, i.e. ANDing, of the authorizations
 * is expressed by components of weight `1` and a weight threshold of `n`).
 *
 * In case `auth_i` is an `other` authority, we introduce an additional rule:
 *
 * ```
 * other_authority -> !other_j
 * ```
 *
 * and then set `auth_i` to `!other_j`.
 *
 * Authorizations starting with `!` are "temporary" identifiers produced by the
 * `authorization_factory`.  The factory is a member of the context to allow
 * multiple visits to the same context with different visitor objects.
 */
// TODO Change special identifier from ! to %

struct operation_get_req_authorizations_vtor
{
   typedef size_t result_type;

   steemit::protocol::get_req_authorizations_context& ctx;

   operation_get_req_authorizations_vtor( steemit::protocol::get_req_authorizations_context& c )
      : ctx( c ) {}

   template< typename T >
   size_t operator()( const T& v )const
   {
      using boost::container::flat_set;
      using steemit::protocol::account_name_type;
      using steemit::protocol::authority;

      flat_set< account_name_type > active;
      flat_set< account_name_type > owner;
      flat_set< account_name_type > posting;
      std::vector< authority > other;

      v.get_required_active_authorities( active );
      v.get_required_owner_authorities( owner );
      v.get_required_posting_authorities( posting );
      v.get_required_authorities( other );

      size_t n_active = active.size();
      size_t n_owner = owner.size();
      size_t n_posting = posting.size();
      size_t n_other = other.size();

      size_t n_total = n_active + n_owner + n_posting + n_other;

      size_t i_op = ctx.rules.size();

      // Build local auth rules
      // The first local rule is active & owner & posting & other -> !op
      // The second local rule is (other_auth) -> !other
      ctx.rules.emplace_back();
      ctx.auth_factory.get_next_auth( "!op", ctx.rules[i_op].rhs );
      ctx.rules[i_op].lhs.weight_threshold = n_total;
      for( const account_name_type& a : active )
         ctx.rules[i_op].lhs.component_authorizations.emplace_back( 1, a, STEEMIT_ACTIVE_ROLE_NAME );
      for( const account_name_type& a : owner )
         ctx.rules[i_op].lhs.component_authorizations.emplace_back( 1, a, STEEMIT_OWNER_ROLE_NAME );
      for( const account_name_type& a : posting )
         ctx.rules[i_op].lhs.component_authorizations.emplace_back( 1, a, STEEMIT_POSTING_ROLE_NAME );
      for( const authority& auth : other )
      {
         size_t i_other = ctx.rules.size();
         ctx.rules.emplace_back( auth_rule_lhs( auth ) );
         ctx.auth_factory.get_next_auth( "!other", ctx.rules[i_other].rhs );
         ctx.rules[i_op].lhs.component_authorizations.emplace_back( 1, ctx.rules[i_other].rhs );
      }

      return i_op;
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
      FC_ASSERT( itr != to_tag.end(),                                      \
         "Invalid operation name: ${n}", ("n", ar[0]) );                   \
      vo.set_which( to_tag[ar[0].as_string()] );                           \
   }                                                                       \
   vo.visit( fc::to_static_variant( ar[1] ) );                             \
}                                                                          \
} /* namespace fc */                                                       \
                                                                           \
namespace steemit { namespace protocol {                                   \
                                                                           \
void operation_validate( const OperationType& op )                         \
{                                                                          \
   op.visit( steemit::protocol::operation_validate_visitor() );            \
}                                                                          \
                                                                           \
void operation_get_required_authorities(                                   \
   const OperationType& op,                                                \
   flat_set< account_name_type >& active,                                  \
   flat_set< account_name_type >& owner,                                   \
   flat_set< account_name_type >& posting,                                 \
   std::vector< authority >& other )                                       \
{                                                                          \
   op.visit( steemit::protocol::operation_get_required_auth_visitor(       \
      active, owner, posting, other ) );                                   \
}                                                                          \
                                                                           \
void operation_get_required_authorizations(                                \
   const OperationType& op,                                                \
   get_req_authorizations_context& ctx )                                   \
{                                                                          \
   op.visit( steemit::protocol::operation_get_req_authorizations_vtor(     \
      ctx ) );                                                             \
}                                                                          \
                                                                           \
} } /* namespace steemit::protocol */
