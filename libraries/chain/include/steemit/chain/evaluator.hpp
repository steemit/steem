#pragma once
#include <steemit/chain/exceptions.hpp>
#include <steemit/chain/protocol/operations.hpp>

namespace steemit { namespace chain {

   class database;
   struct signed_transaction;
   class generic_evaluator;
   class transaction_evaluation_state;

   class generic_evaluator
   {
   public:
      virtual ~generic_evaluator(){}

      virtual int get_type()const = 0;
      virtual void start_evaluate(transaction_evaluation_state& eval_state, const operation& op, bool apply);

      /**
       * @note derived classes should ASSUME that the default validation that is
       * indepenent of chain state should be performed by op.validate() and should
       * not perform these extra checks.
       */
      virtual void evaluate(const operation& op) = 0;
      virtual void  apply(const operation& op) = 0;

      database& db()const;

      //void check_required_authorities(const operation& op);
   protected:
      transaction_evaluation_state*    trx_state;
   };

   class op_evaluator
   {
   public:
      virtual ~op_evaluator(){}
      virtual void  evaluate(transaction_evaluation_state& eval_state, const operation& op, bool apply) = 0;
   };

   template<typename T>
   class op_evaluator_impl : public op_evaluator
   {
   public:
      virtual void  evaluate(transaction_evaluation_state& eval_state, const operation& op, bool apply = true) override
      {
         T eval;
         eval.start_evaluate(eval_state, op, apply);
      }
   };

   template<typename DerivedEvaluator>
   class evaluator : public generic_evaluator
   {
   public:
      virtual int get_type()const override { return operation::tag<typename DerivedEvaluator::operation_type>::value; }

      virtual void evaluate(const operation& o) final override
      {
         auto* eval = static_cast<DerivedEvaluator*>(this);
         const auto& op = o.get<typename DerivedEvaluator::operation_type>();
         eval->do_evaluate(op);
      }

      virtual void apply(const operation& o) final override
      {
         auto* eval = static_cast<DerivedEvaluator*>(this);
         const auto& op = o.get<typename DerivedEvaluator::operation_type>();
         eval->do_apply(op);
      }
   };
} }
