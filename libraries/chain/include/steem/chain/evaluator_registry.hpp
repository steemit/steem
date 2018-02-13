#pragma once

#include <steem/chain/evaluator.hpp>

namespace steem { namespace chain {

template< typename OperationType >
class evaluator_registry
{
   public:
      evaluator_registry( database& d )
         : _db(d)
      {
         for( int i=0; i<OperationType::count(); i++ )
             _op_evaluators.emplace_back();
      }

      template< typename EvaluatorType, typename... Args >
      void register_evaluator( Args... args )
      {
         _op_evaluators[ OperationType::template tag< typename EvaluatorType::operation_type >::value ].reset( new EvaluatorType(_db, args...) );
      }

   private:
      template< bool CHECK >
      unique_ptr< evaluator<OperationType> >& get_evaluator_impl( const OperationType& op )
      {
         static unique_ptr< evaluator<OperationType> > empty;

         int i_which = op.which();
         uint64_t u_which = uint64_t( i_which );

         if( i_which < 0 )
         {
            if( CHECK )
               return empty;
            else
               assert( "Negative operation tag" && false );
         }

         if( u_which >= _op_evaluators.size() )
         {
            if( CHECK )
               return empty;
            else
               assert( "No registered evaluator for this operation" && false );
         }

         unique_ptr< evaluator<OperationType> >& eval = _op_evaluators[ u_which ];

         if( !eval )
         {
            if( CHECK )
               return empty;
            else
               assert( "No registered evaluator for this operation" && false );
         }

         return eval;
      }

   public:

      bool is_evaluator( const OperationType& op )
      {
         return get_evaluator_impl< true/*CHECK*/ >( op ).get() != nullptr;
      }

      evaluator<OperationType>& get_evaluator( const OperationType& op )
      {
         return *get_evaluator_impl< false/*CHECK*/ >( op );
      }

      std::vector< std::unique_ptr< evaluator<OperationType> > > _op_evaluators;
      database& _db;
};

} }
