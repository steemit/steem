
#pragma once

#include <steem/protocol/asset_symbol.hpp>

#ifdef STEEM_ENABLE_SMT

// Functions in this header file break apart the NAI data and check digits.
// Most code should not need to include this header.

namespace steem { namespace protocol {

asset_symbol_type asset_symbol_from_nai_data( uint32_t nai_data_digits, uint8_t decimal_places );
uint32_t asset_symbol_to_nai_data( asset_symbol_type sym );

} }

#endif
