#pragma once

#include <fc/exception/exception.hpp>
#include <steem/protocol/protocol.hpp>

#define STEEM_ASSERT( expr, exc_type, FORMAT, ... )                \
   FC_MULTILINE_MACRO_BEGIN                                           \
   if( !(expr) )                                                      \
      FC_THROW_EXCEPTION( exc_type, FORMAT, __VA_ARGS__ );            \
   FC_MULTILINE_MACRO_END

namespace steem { namespace protocol {

   FC_DECLARE_EXCEPTION( transaction_exception, 3000000, "transaction exception" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_active_auth,            steem::protocol::transaction_exception, 3010000, "missing required active authority" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_owner_auth,             steem::protocol::transaction_exception, 3020000, "missing required owner authority" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_posting_auth,           steem::protocol::transaction_exception, 3030000, "missing required posting authority" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_other_auth,             steem::protocol::transaction_exception, 3040000, "missing required other authority" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_irrelevant_sig,                 steem::protocol::transaction_exception, 3050000, "irrelevant signature included" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate_sig,                  steem::protocol::transaction_exception, 3060000, "duplicate signature included" )

   #define STEEM_RECODE_EXC( cause_type, effect_type ) \
      catch( const cause_type& e ) \
      { throw( effect_type( e.what(), e.get_log() ) ); }

} } // steem::protocol
