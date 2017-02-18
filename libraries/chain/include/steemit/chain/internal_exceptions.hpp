#pragma once

#include <fc/exception/exception.hpp>
#include <steemit/chain/exceptions.hpp>

#define STEEMIT_DECLARE_INTERNAL_EXCEPTION(exc_name, seqnum, msg)  \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      internal_ ## exc_name,                                          \
      steemit::chain::internal_exception,                            \
      3990000 + seqnum,                                               \
      msg                                                             \
      )

namespace steemit {
    namespace chain {

        FC_DECLARE_DERIVED_EXCEPTION(internal_exception, steemit::chain::chain_exception, 3990000, "internal exception")

        STEEMIT_DECLARE_INTERNAL_EXCEPTION(verify_auth_max_auth_exceeded, 1, "Exceeds max authority fan-out")

        STEEMIT_DECLARE_INTERNAL_EXCEPTION(verify_auth_account_not_found, 2, "Auth account not found")

    }
} // steemit::chain
