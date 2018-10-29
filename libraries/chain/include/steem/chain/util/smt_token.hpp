#pragma once

#ifdef STEEM_ENABLE_SMT

#include <steem/chain/database.hpp>
#include <steem/chain/smt_objects/smt_token_object.hpp>
#include <steem/protocol/asset_symbol.hpp>

namespace steem { namespace chain { namespace util {

const smt_token_object* smt_token_lookup( database& db, uint32_t nai );
const smt_token_object* smt_token_lookup( database& db, asset_symbol_type symbol, bool precision_agnostic = false );

} } } // steem::chain::util

#endif
