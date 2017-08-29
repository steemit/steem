
#include <steemit/chain/steem_evaluator.hpp>
#include <steemit/chain/database.hpp>
#include <steemit/chain/steem_objects.hpp>
#include <steemit/chain/smt_objects.hpp>

#include <steemit/chain/util/reward.hpp>

#include <steemit/protocol/smt_operations.hpp>

namespace steemit { namespace chain {

void smt_setup_evaluator::do_apply( const smt_setup_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEMIT_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEMIT_SMT_HARDFORK) );
}

void smt_cap_reveal_evaluator::do_apply( const smt_cap_reveal_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEMIT_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEMIT_SMT_HARDFORK) );
}

void smt_refund_evaluator::do_apply( const smt_refund_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEMIT_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEMIT_SMT_HARDFORK) );
}

void smt_setup_inflation_evaluator::do_apply( const smt_setup_inflation_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEMIT_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEMIT_SMT_HARDFORK) );
}

void smt_set_setup_parameters_evaluator::do_apply( const smt_set_setup_parameters_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEMIT_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEMIT_SMT_HARDFORK) );
}

void smt_set_runtime_parameters_evaluator::do_apply( const smt_set_runtime_parameters_operation& o )
{
   FC_ASSERT( _db.has_hardfork( STEEMIT_SMT_HARDFORK ), "SMT functionality not enabled until hardfork ${hf}", ("hf", STEEMIT_SMT_HARDFORK) );
}

} }
