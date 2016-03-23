#include <steemit/chain/database.hpp>
#include <steemit/chain/evaluator.hpp>
#include <steemit/chain/exceptions.hpp>
#include <steemit/chain/hardfork.hpp>
#include <steemit/chain/transaction_evaluation_state.hpp>

#include <fc/uint128.hpp>

namespace steemit { namespace chain {
database& generic_evaluator::db()const { return trx_state->db(); }

   void generic_evaluator::start_evaluate( transaction_evaluation_state& eval_state, const operation& op, bool apply )
   { try {
      trx_state   = &eval_state;
      //check_required_authorities(op);
      evaluate( op );
      if( apply ) this->apply( op );
   } FC_CAPTURE_AND_RETHROW() }


} }
