#pragma once

#ifdef DPN_ENABLE_SMT

#include <fc/optional.hpp>
#include <dpn/chain/database.hpp>
#include <dpn/chain/smt_objects/smt_token_object.hpp>
#include <dpn/protocol/asset_symbol.hpp>

namespace dpn { namespace chain { namespace util { namespace smt {

const smt_token_object* find_token( const database& db, uint32_t nai );
const smt_token_object* find_token( const database& db, asset_symbol_type symbol, bool precision_agnostic = false );
const smt_token_emissions_object* last_emission( const database& db, const asset_symbol_type& symbol );
fc::optional< time_point_sec > last_emission_time( const database& db, const asset_symbol_type& symbol );

} } } } // dpn::chain::util::smt

#endif
