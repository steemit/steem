
#pragma once

#include <steemit/protocol/authority.hpp>
#include <steemit/protocol/auth_rule.hpp>

#include <fc/variant.hpp>

#include <boost/container/flat_set.hpp>

#include <string>
#include <vector>

//
// Place DECLARE_OPERATION_TYPE in a .hpp file to declare
// functions related to your operation type
//
#define DECLARE_OPERATION_TYPE( OperationType )                                  \
namespace fc {                                                                   \
                                                                                 \
void to_variant( const OperationType&, fc::variant& );                           \
void from_variant( const fc::variant&, OperationType& );                         \
                                                                                 \
} /* fc */                                                                       \
                                                                                 \
namespace steemit { namespace protocol {                                         \
                                                                                 \
void operation_validate( const OperationType& o );                               \
void operation_get_required_authorities(                                         \
   const OperationType& op,                                                      \
   flat_set< account_name_type >& active,                                        \
   flat_set< account_name_type >& owner,                                         \
   flat_set< account_name_type >& posting,                                       \
   vector< authority >& other );                                                 \
void operation_get_required_authorizations(                                      \
   const OperationType& op,                                                      \
   get_req_authorizations_context& ctx );                                        \
                                                                                 \
} } /* steemit::protocol */

namespace steemit { namespace protocol {

/**
 * This class produces unique temporary `authorization` objects.
 * The authorization produced has an empty account name, and
 * a role name which combines the specified prefix with an
 * incrementing counter.
 */

struct local_authorization_factory
{
   local_authorization_factory() {}
   void get_next_auth( const std::string& prefix, authorization& auth )
   {
      auth.account = account_name_type();
      auth.role = role_name_type( prefix + std::to_string( next_auth_num++ ) );
   }

   uint32_t next_auth_num = 0;
};

struct get_req_authorizations_context
{
   vector< auth_rule >            rules;
   local_authorization_factory    auth_factory;
};

} }
