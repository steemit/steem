#include <steemit/chain/transfer_evaluator.hpp>
#include <steemit/chain/account_object.hpp>
#include <steemit/chain/exceptions.hpp>
#include <steemit/chain/hardfork.hpp>

namespace steemit { namespace chain {
void_result transfer_evaluator::do_evaluate( const transfer_operation& op )
{ try {
   const database& d = db();
}  FC_CAPTURE_AND_RETHROW( (op) ) }

void_result transfer_evaluator::do_apply( const transfer_operation& o )
{ try {
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

} } // steemit::chain
