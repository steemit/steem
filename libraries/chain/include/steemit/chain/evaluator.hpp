#pragma once

#include <steemit/chain/exceptions.hpp>
#include <steemit/chain/protocol/operations.hpp>

namespace steemit {
    namespace chain {

        class database;

        template<typename OperationType=steemit::chain::operation>
        class generic_evaluator {
        public:
            virtual void apply(const OperationType &op) = 0;

            virtual int get_type() const = 0;
        };

        template<typename EvaluatorType, typename OperationType=steemit::chain::operation>
        class evaluator : public generic_evaluator<OperationType> {
        public:
            typedef OperationType operation_sv_type;
            // typedef typename EvaluatorType::operation_type op_type;

            evaluator(database &d)
                    : _db(d) {
            }

            virtual void apply(const OperationType &o) final override {
                auto *eval = static_cast< EvaluatorType * >(this);
                const auto &op = o.template get<typename EvaluatorType::operation_type>();
                eval->do_apply(op);
            }

            virtual int get_type() const override {
                return OperationType::template tag<typename EvaluatorType::operation_type>::value;
            }

            database &db() {
                return _db;
            }

        protected:
            database &_db;
        };

        template<typename OperationType>
        class evaluator_registry {
        public:
            evaluator_registry(database &d)
                    : _db(d) {
                for (int i = 0; i < OperationType::count(); i++) {
                    _op_evaluators.emplace_back();
                }
            }

            template<typename EvaluatorType, typename... Args>
            void register_evaluator(Args... args) {
                _op_evaluators[OperationType::template tag<typename EvaluatorType::operation_type>::value].reset(new EvaluatorType(_db, args...));
            }

            generic_evaluator<OperationType> &get_evaluator(const OperationType &op) {
                int i_which = op.which();
                uint64_t u_which = uint64_t(i_which);
                if (i_which < 0)
                    assert("Negative operation tag" && false);
                if (u_which >= _op_evaluators.size())
                    assert("No registered evaluator for this operation" &&
                           false);
                unique_ptr<generic_evaluator<OperationType>> &eval = _op_evaluators[u_which];
                if (!eval)
                    assert("No registered evaluator for this operation" &&
                           false);
                return *eval;
            }

            std::vector<std::unique_ptr<generic_evaluator<OperationType>>> _op_evaluators;
            database &_db;
        };

    }
}

#define DEFINE_EVALUATOR(X) \
class X ## _evaluator : public steemit::chain::evaluator< X ## _evaluator > \
{                                                                           \
   public:                                                                  \
      typedef X ## _operation operation_type;                               \
                                                                            \
      X ## _evaluator( database& db )                                       \
         : steemit::chain::evaluator< X ## _evaluator >( db )               \
      {}                                                                    \
                                                                            \
      void do_apply( const X ## _operation& o );                            \
};
