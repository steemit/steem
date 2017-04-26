
#pragma once

#include <steemit/protocol/auth_rule.hpp>

#include <vector>

#define STEEMIT_GET_AUTH_RULES_FOR_ACCOUNT_BODY                                                 \
   result.emplace_back( account.owner  , account_name            , STEEMIT_OWNER_ROLE_NAME   ); \
   result.emplace_back( account.active , account_name            , STEEMIT_ACTIVE_ROLE_NAME  ); \
   result.emplace_back( account.posting, account_name            , STEEMIT_POSTING_ROLE_NAME ); \
   result.emplace_back( account_name   , STEEMIT_OWNER_ROLE_NAME , STEEMIT_ACTIVE_ROLE_NAME  ); \
   result.emplace_back( account_name   , STEEMIT_ACTIVE_ROLE_NAME, STEEMIT_POSTING_ROLE_NAME );

namespace steemit { namespace chain {

void get_required_authorizations(
   std::vector< authorization >& authorizations,
   const steemit::protocol::transaction& tx );

} }
