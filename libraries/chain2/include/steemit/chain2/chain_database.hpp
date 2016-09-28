#pragma once
#include <steemit/protocol/block.hpp>
#include <steemit/chain2/object_types.hpp>

#include <fc/signals.hpp>

namespace steemit { namespace chain2 {

   enum skip_flags {
      skip_undo_block,
      skip_undo_transaction,
      skip_undo_operation,
      skip_validation ///< apply changes without validation checks
   };

   class block_object;

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

      private:

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



