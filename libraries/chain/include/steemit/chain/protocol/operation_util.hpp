
#pragma once

#include <steemit/chain/protocol/authority.hpp>

#include <fc/variant.hpp>

#include <boost/container/flat_set.hpp>

#include <string>
#include <vector>

//
// Place DECLARE_OPERATION_TYPE in a .hpp file to declare
// functions related to your operation type
//
#define DECLARE_OPERATION_TYPE( OperationType )                       \
namespace fc {                                                        \
                                                                      \
void to_variant( const OperationType&, fc::variant& );                \
void from_variant( const fc::variant&, OperationType& );              \
                                                                      \
} /* fc */                                                            \
                                                                      \
namespace steemit { namespace chain {                                 \
                                                                      \
void operation_validate( const OperationType& o );                    \
void operation_get_required_authorities( const OperationType& op,     \
                                         flat_set<string>& active,    \
                                         flat_set<string>& owner,     \
                                         flat_set<string>& posting,   \
                                         vector<authority>& other );  \
                                                                      \
} } /* steemit::chain */
