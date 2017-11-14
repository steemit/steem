#include <steem/plugins/witness/witness_operations.hpp>

#include <steem/protocol/operation_util_impl.hpp>

namespace steem { namespace plugins { namespace witness {

void enable_content_editing_operation::validate()const
{
   protocol::validate_account_name( account );
}

uint32_t egg_pow_input::calculate_pow_summary()const
{
   // TODO:  For now this is sha256, it should be modified to scrypt before release
   return (fc::sha256::hash( *this )).approx_log_32();
}

void egg_pow::validate()const
{
   FC_ASSERT( pow_summary == input.calculate_pow_summary() );
}

void create_egg_operation::validate()const
{
   work.validate();
   FC_ASSERT( work.input.h_egg_auth == fc::sha256::hash( egg_auth ) );
}

} } } // steem::plugins::witness

STEEM_DEFINE_OPERATION_TYPE( steem::plugins::witness::witness_plugin_operation )
