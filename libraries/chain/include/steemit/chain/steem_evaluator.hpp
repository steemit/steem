#pragma once
#include <steemit/chain/evaluator.hpp>
#include <steemit/chain/protocol/steem_operations.hpp>

namespace steemit{ namespace chain {

#define DEFINE_EVALUATOR( X ) \
class X ## _evaluator : public evaluator< X ## _evaluator > { public: \
   typedef X ## _operation operation_type; \
   void do_evaluate( const X ## _operation& o ){};  \
   void do_apply( const X ## _operation& o ); \
};

DEFINE_EVALUATOR( account_create )
DEFINE_EVALUATOR( account_update )
DEFINE_EVALUATOR( transfer )
DEFINE_EVALUATOR( transfer_to_vesting )
DEFINE_EVALUATOR( witness_update )
DEFINE_EVALUATOR( account_witness_vote )
DEFINE_EVALUATOR( account_witness_proxy )
DEFINE_EVALUATOR( withdraw_vesting )
DEFINE_EVALUATOR( comment )
DEFINE_EVALUATOR( delete_comment )
DEFINE_EVALUATOR( vote )
DEFINE_EVALUATOR( custom )
DEFINE_EVALUATOR( custom_json )
DEFINE_EVALUATOR( pow )
DEFINE_EVALUATOR( feed_publish )
DEFINE_EVALUATOR( convert )
DEFINE_EVALUATOR( limit_order_create )
DEFINE_EVALUATOR( limit_order_cancel )
DEFINE_EVALUATOR( report_over_production )

} } // steemit::chain
