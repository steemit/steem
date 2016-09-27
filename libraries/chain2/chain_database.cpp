#include <steemit/chain2/chain_database.hpp>

namespace steemit { namespace chain2 {

   void apply( database& db, const signed_block& b, const options_type& opts ) {
      auto undo_session = db.start_undo_session( !(opts & skip_undo_block) );
         db.pre_apply_block( b );
         for( const auto& trx : b.transactions ) {
            apply( db, b, opts );
         }
         db.post_apply_block( b );
      undo_session.push();
   }

   void apply( database& db, const signed_transaction& t, const options_type& opts  ){
      auto undo_session = db.start_undo_session( !(opts & skip_undo_transaction) );
         db.pre_apply_transaction( t );
         for( const auto& op : t.operations ) {
            apply( db, op, opts );
         }
         db.post_apply_transaction( t );
      undo_session.squash();
   }

   struct apply_operation_visitor {
      apply_operation_visitor( database& db, const options_type& opts ):_db(db),_opts(opts){}

      template<typename T>
      void operator()( T&& op )const {
         apply( _db, std::forward<T>(op), _opts );
      }

      const options_type&  _opts;
      database&            _db;
   };

   void apply( database& db, const operation& o, const options_type& opts ) {
      auto undo_session = db.start_undo_session( !(opts & skip_undo_operation) );
         db.pre_apply_operation( o ); 
         o.visit( apply_operation_visitor( db, opts ) );
         db.post_apply_operation( o ); 
      undo_session.squash();
   }

   struct validate_operation_visitor {
      validate_operation_visitor( const options_type& opts )_opts(opts){}

      template<typename T>
      void operator()( T&& op )const {
         validate( std::forward<T>(op), _opts );
      }
      const options_type&  _opts;
   };

   void validate( const operation& o, const options_type& opts ) {
     o.visit( validate_operation_visitor( opts ) );
   }

} } // steemit::chain2
