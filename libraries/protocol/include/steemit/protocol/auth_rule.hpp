
#pragma once

#include <steemit/protocol/types.hpp>

#include <vector>

// macros to help with boilerplate comparison operators for types

#define STEEMIT_COMPARISON_TIE2_EQ( OP, T, FIELD_1, FIELD_2 )          \
   bool operator OP( const T& b )const                                 \
   {                                                                   \
      return std::tie(   FIELD_1,   FIELD_2 ) OP                       \
             std::tie( b.FIELD_1, b.FIELD_2 );                         \
   }

#define STEEMIT_COMPARISON_TIE2_GL( OP, T, FIELD_1, FIELD_2 )          \
   friend bool operator OP( const T& a, const T& b )                   \
   {                                                                   \
      return std::tie( a.FIELD_1, a.FIELD_2 ) OP                       \
             std::tie( b.FIELD_1, b.FIELD_2 );                         \
   }

#define STEEMIT_COMPARISON_TIE2( T, FIELD_1, FIELD_2 )                 \
   STEEMIT_COMPARISON_TIE2_EQ( ==, T, FIELD_1, FIELD_2 )               \
   STEEMIT_COMPARISON_TIE2_EQ( !=, T, FIELD_1, FIELD_2 )               \
   STEEMIT_COMPARISON_TIE2_GL( > , T, FIELD_1, FIELD_2 )               \
   STEEMIT_COMPARISON_TIE2_GL( >=, T, FIELD_1, FIELD_2 )               \
   STEEMIT_COMPARISON_TIE2_GL( < , T, FIELD_1, FIELD_2 )               \
   STEEMIT_COMPARISON_TIE2_GL( <=, T, FIELD_1, FIELD_2 )

#define STEEMIT_COMPARISON_TIE3_EQ( OP, T, FIELD_1, FIELD_2, FIELD_3 ) \
   bool operator OP( const T& b )const                                 \
   {                                                                   \
      return std::tie(   FIELD_1,   FIELD_2,   FIELD_3 ) OP            \
             std::tie( b.FIELD_1, b.FIELD_2, b.FIELD_3 );              \
   }

#define STEEMIT_COMPARISON_TIE3_GL( OP, T, FIELD_1, FIELD_2, FIELD_3 ) \
   friend bool operator OP( const T& a, const T& b )                   \
   {                                                                   \
      return std::tie( a.FIELD_1, a.FIELD_2, a.FIELD_3 ) OP            \
             std::tie( b.FIELD_1, b.FIELD_2, b.FIELD_3 );              \
   }

#define STEEMIT_COMPARISON_TIE3( T, FIELD_1, FIELD_2, FIELD_3 )        \
   STEEMIT_COMPARISON_TIE3_EQ( ==, T, FIELD_1, FIELD_2, FIELD_3 )      \
   STEEMIT_COMPARISON_TIE3_EQ( !=, T, FIELD_1, FIELD_2, FIELD_3 )      \
   STEEMIT_COMPARISON_TIE3_GL( > , T, FIELD_1, FIELD_2, FIELD_3 )      \
   STEEMIT_COMPARISON_TIE3_GL( >=, T, FIELD_1, FIELD_2, FIELD_3 )      \
   STEEMIT_COMPARISON_TIE3_GL( < , T, FIELD_1, FIELD_2, FIELD_3 )      \
   STEEMIT_COMPARISON_TIE3_GL( <=, T, FIELD_1, FIELD_2, FIELD_3 )

namespace steemit { namespace protocol {

/**
 * An authorization represents an (account, role) pair.
 */
struct authorization
{
   authorization() {}
   authorization( const account_name_type& a, const role_name_type& r )
      : account(a), role(r) {}

   account_name_type      account;
   role_name_type         role;

   STEEMIT_COMPARISON_TIE2( authorization, account, role )
};

struct auth_rule_auth_component
{
   auth_rule_auth_component( weight_type w, const account_name_type& account, const role_name_type& role )
      : weight(w), auth( account, role ) {}
   auth_rule_auth_component( weight_type w, const authorization& a )
      : weight(w), auth(a) {}

   weight_type            weight;
   authorization          auth;

   STEEMIT_COMPARISON_TIE2( auth_rule_auth_component, weight, auth )
};

struct auth_rule_key_component
{
   auth_rule_key_component( weight_type w, const public_key_type& k )
      : weight(w), key(k) {}

   weight_type            weight;
   public_key_type        key;

   STEEMIT_COMPARISON_TIE2( auth_rule_key_component, weight, key )
};

struct auth_rule_lhs
{
   template< typename AuthorityType >
   auth_rule_lhs( const AuthorityType& lhs_auth );
   auth_rule_lhs() {}

   auth_rule_lhs( const account_name_type& account, const role_name_type& role )
      : weight_threshold(1)
   { component_authorizations.emplace_back( 1, account, role ); }
   auth_rule_lhs( const authorization& auth )
      : weight_threshold(1)
   { component_authorizations.emplace_back( 1, auth );          }
   auth_rule_lhs( const public_key_type& k )
      : weight_threshold(1)
   { component_keys.emplace_back( 1, k );                       }

   std::vector< auth_rule_auth_component >        component_authorizations;
   std::vector< auth_rule_key_component >         component_keys;
   uint32_t                                       weight_threshold = 0;

   STEEMIT_COMPARISON_TIE3( auth_rule_lhs, component_authorizations, component_keys, weight_threshold )
};

/**
 * An auth_rule is a rule of the form:
 *
 * TMS( threshold, (w0, k0|auth0), (w1, k1|auth1), ... ) -> auth
 *
 * Recall an authorization (auth) is simply an (account, role) pair.
 *
 * An auth_rule simply states that a threshold multi-sig of some
 * weighted components grants a new auth.  Each component consists of
 * a weight, and either a key or an auth.  The "key components" and "auth
 * components" are tracked by this class in separate vectors.
 */
struct auth_rule
{
   auth_rule( const auth_rule_lhs& lh, const authorization& rh )
      : lhs(lh), rhs(rh) {}
   auth_rule( const auth_rule_lhs& lh )
      : lhs(lh) {}
   auth_rule( const authorization& rh )
      : rhs(rh) {}
   auth_rule() {}

   // shortcut for auth_rule( auth_rule_lhs(lhs_auth), authorization(rhs_account, rhs_role) )
   template< typename AuthorityType >
   auth_rule( const AuthorityType& lhs_auth, const account_name_type& rhs_account, const role_name_type& rhs_role )
      : lhs(lhs_auth), rhs(rhs_account, rhs_role) {}

   // constructor for rules of the form (acct, owner) -> (acct, active)
   auth_rule( const account_name_type& account, const role_name_type& lhs_role, const role_name_type& rhs_role )
      : lhs( account, lhs_role ), rhs( account, rhs_role ) {}

   auth_rule_lhs                                  lhs;
   authorization                                  rhs;

   STEEMIT_COMPARISON_TIE2( auth_rule, lhs, rhs )
};

template< typename AuthorityType >
auth_rule_lhs::auth_rule_lhs( const AuthorityType& auth )
{
   component_authorizations.reserve( auth.account_auths.size() );
   component_keys.reserve( auth.key_auths.size() );

   for( const std::pair< account_name_type, weight_type >& e : auth.account_auths )
      component_authorizations.emplace_back( e.second, e.first, STEEMIT_ACTIVE_ROLE_NAME );
   for( const std::pair< public_key_type, weight_type >& e : auth.key_auths )
      component_keys.emplace_back( e.second, e.first );
   weight_threshold = auth.weight_threshold;
}

} }
