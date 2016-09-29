#pragma once
#include <steemit/protocol/block.hpp>
#include <steemit/chain2/block_objects.hpp>
#include <steemit/chain/fork_database.hpp>

#include <fc/signals.hpp>
#include <fc/scoped_exit.hpp>

namespace steemit { namespace chain2 {

   enum skip_flags {
      skip_undo_block,
      skip_undo_transaction,
      skip_undo_operation,
      skip_validation ///< apply changes without validation checks
   };

   class database : public db2::database {
      public:
         database();
         ~database();

         void open( const fc::path& dir );

         const block_object& head_block()const;
         uint32_t            head_block_num()const;

         fc::signal<void( const operation& )>          pre_apply_operation; 
         fc::signal<void( const operation& )>          post_apply_operation; 
         fc::signal<void( const signed_transaction& )> pre_apply_transaction;
         fc::signal<void( const signed_transaction& )> post_apply_transaction;
         fc::signal<void( const signed_block& )>       pre_apply_block;
         fc::signal<void( const signed_block& )>       post_apply_block;

         void           push_block( const signed_block& b );
         void           push_transaction( const signed_transaction& trx );
         signed_block   generate_block( time_point_sec time, const account_name_type& witness, const fc::ecc::private_key& block_signing_key );

         auto clear_pending() {
            _pending_tx_session.reset();
            return fc::make_scoped_exit( [pending=std::move(_pending_transactions),this]()mutable {
               for( const auto& t : pending ) {
                  try {
                     push_transaction( t );
                  } catch ( ... ){}
               }
            });
         }

      private:

         chain::fork_database         _fork_db;
         vector< signed_transaction > _pending_transactions;
         optional<session>            _pending_tx_session;
   };

   void apply( database& db, const signed_block& b, const options_type& opts = options_type() );
   void apply( database& db, const signed_transaction& t, const options_type& opts = options_type() );
   void apply( database& db, const operation& o, const options_type& opts = options_type() ); 
   
   void validate( const signed_block& o, const options_type& opts  = options_type() );
   void validate( const signed_transaction& o, const options_type& opts  = options_type() );
   void validate( const operation& o, const options_type& opts = options_type() );

   /** provide a default implementation of validate for types that don't have any specific 
    *  validation logic
    */
   template<typename T>
   void validate( const T&, const options_type& opts = options_type() ){} 

} } // namespace steemit::chain2



