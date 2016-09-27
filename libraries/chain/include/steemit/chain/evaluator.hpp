#pragma once
#include <steemit/protocol/exceptions.hpp>
#include <steemit/protocol/operations.hpp>

namespace steemit { namespace chain {

class database;

template< typename OperationType=steemit::protocol::operation >
class evaluator
{
   public:
      virtual void apply(const OperationType& op) = 0;
      virtual int get_type()const = 0;
};

template< typename EvaluatorType, typename OperationType=steemit::protocol::operation >
class evaluator_impl : public evaluator<OperationType>
{
   public:
      typedef OperationType operation_sv_type;
      // typedef typename EvaluatorType::operation_type op_type;

      evaluator_impl( database& d )
         : _db(d) {}

      virtual void apply(const OperationType& o) final override
      {
         auto* eval = static_cast< EvaluatorType* >(this);
         const auto& op = o.template get< typename EvaluatorType::operation_type >();
         eval->do_apply(op);
      }

      virtual int get_type()const override { return OperationType::template tag< typename EvaluatorType::operation_type >::value; }

      database& db() { return _db; }

   protected:
      database& _db;
};

} }

#define DEFINE_EVALUATOR( X ) \
class X ## _evaluator : public steemit::chain::evaluator_impl< X ## _evaluator > \
{                                                                           \
   public:                                                                  \
      typedef X ## _operation operation_type;                               \
                                                                            \
      X ## _evaluator( database& db )                                       \
         : steemit::chain::evaluator_impl< X ## _evaluator >( db )          \
      {}                                                                    \
                                                                            \
      void do_apply( const X ## _operation& o );                            \
};
