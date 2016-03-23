#pragma once
#include <steemit/chain/protocol/operations.hpp>
#include <steemit/chain/evaluator.hpp>
#include <steemit/chain/database.hpp>

namespace steemit { namespace chain {

   class custom_evaluator : public evaluator<custom_evaluator>
   {
      public:
         typedef custom_operation operation_type;

         void do_evaluate( const custom_operation& o ){}
         void do_apply( const custom_operation& o ){}
   };
} }
