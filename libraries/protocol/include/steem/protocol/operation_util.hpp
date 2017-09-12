
#pragma once

#include <steem/protocol/authority.hpp>

#include <fc/variant.hpp>

#include <boost/container/flat_set.hpp>

#include <string>
#include <vector>

//
// Place STEEM_DECLARE_OPERATION_TYPE in a .hpp file to declare
// functions related to your operation type
//
#define STEEM_DECLARE_OPERATION_TYPE( OperationType )                            \
namespace fc {                                                                   \
                                                                                 \
void to_variant( const OperationType&, fc::variant& );                           \
void from_variant( const fc::variant&, OperationType& );                         \
                                                                                 \
} /* fc */                                                                       \
                                                                                 \
namespace steem { namespace protocol {                                         \
                                                                                 \
void operation_validate( const OperationType& o );                               \
void operation_get_required_authorities( const OperationType& op,                \
                                         flat_set< account_name_type >& active,  \
                                         flat_set< account_name_type >& owner,   \
                                         flat_set< account_name_type >& posting, \
                                         vector< authority >& other );           \
                                                                                 \
} } /* steem::protocol */
