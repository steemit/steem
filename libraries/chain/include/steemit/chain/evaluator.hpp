#pragma once
#include <steemit/chain/exceptions.hpp>
#include <steemit/chain/transaction_evaluation_state.hpp>

#include <steemit/chain/protocol/operations.hpp>

namespace steemit { namespace chain {

   class database;
   struct signed_transaction;
   template< typename Operation > class generic_evaluator;
   class transaction_evaluation_state;

   template< typename Operation >
   class generic_evaluator
   {
   public:
      virtual ~generic_evaluator(){}

      virtual int get_type()const = 0;
      virtual void start_evaluate(transaction_evaluation_state& eval_state, const Operation& op, bool apply)
      { try {
         trx_state   = &eval_state;
         evaluate( op );
         if( apply ) this->apply( op );
      } FC_CAPTURE_AND_RETHROW() }

      /**
       * @note derived classes should ASSUME that the default validation that is
       * indepenent of chain state should be performed by op.validate() and should
       * not perform these extra checks.
       */
      virtual void evaluate(const Operation& op) = 0;
      virtual void  apply(const Operation& op) = 0;

      database& db()const { return trx_state->db(); }

      //void check_required_authorities(const Operation& op);
   protected:
      transaction_evaluation_state*    trx_state;
   };

   template< typename Operation=operation >
   class op_evaluator
   {
   public:
      virtual ~op_evaluator(){}
      virtual void  evaluate(transaction_evaluation_state& eval_state, const Operation& op, bool apply) = 0;
   };

   template<typename T, typename Operation=operation >
   class op_evaluator_impl : public op_evaluator< Operation >
   {
   public:
      virtual void  evaluate(transaction_evaluation_state& eval_state, const Operation& op, bool apply = true) override
      {
         T eval;
         eval.start_evaluate(eval_state, op, apply);
      }
   };

   template< typename DerivedEvaluator, typename Operation=operation >
   class evaluator : public generic_evaluator< Operation >
   {
   public:
      virtual int get_type()const override { return Operation::template tag<typename DerivedEvaluator::operation_type>::value; }

      virtual void evaluate(const Operation& o) final override
      {
         auto* eval = static_cast<DerivedEvaluator*>(this);
         const auto& op = o.template get<typename DerivedEvaluator::operation_type>();
         eval->do_evaluate(op);
      }

      virtual void apply(const Operation& o) final override
      {
         auto* eval = static_cast<DerivedEvaluator*>(this);
         const auto& op = o.template get<typename DerivedEvaluator::operation_type>();
         eval->do_apply(op);
      }
   };
} }
