#pragma once
#include <steemit/chain/protocol/operations.hpp>
#include <steemit/chain/evaluator.hpp>
#include <steemit/chain/database.hpp>

namespace steemit { namespace chain {

   class assert_evaluator : public evaluator<assert_evaluator>
   {
      public:
         typedef assert_operation operation_type;

         void_result do_evaluate( const assert_operation& o );
         void_result do_apply( const assert_operation& o );
   };

} } // steemit::chain
