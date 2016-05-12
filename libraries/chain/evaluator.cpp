#include <steemit/chain/database.hpp>
#include <steemit/chain/evaluator.hpp>
#include <steemit/chain/exceptions.hpp>
#include <steemit/chain/hardfork.hpp>
#include <steemit/chain/transaction_evaluation_state.hpp>

#include <fc/uint128.hpp>

namespace steemit { namespace chain {
   template< typename Operation >
   database& generic_evaluator< Operation >::db()const { return trx_state->db(); }

   template< typename Operation >
   void generic_evaluator< Operation >::start_evaluate( transaction_evaluation_state& eval_state, const Operation& op, bool apply )
   { try {
      trx_state   = &eval_state;
      //check_required_authorities(op);
      evaluate( op );
      if( apply ) this->apply( op );
   } FC_CAPTURE_AND_RETHROW() }

} } // steemit::chain
