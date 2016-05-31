
#pragma once

#include <steemit/chain/protocol/authority.hpp>

#include <fc/variant.hpp>

#include <boost/container/flat_set.hpp>

#include <string>
#include <vector>

namespace steemit { namespace chain {

std::string name_from_type( const std::string& type_name );

} }

//
// Place DECLARE_OPERATION_TYPE in a .hpp file to declare
// functions related to your operation type
//
#define DECLARE_OPERATION_TYPE( OperationType )                       \
namespace fc {                                                        \
                                                                      \
void to_variant( const OperationType& o, fc::variant var );           \
void from_variant( const fc::variant& var, OperationType& o );        \
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
