#pragma once
#include <graphene/db2/database.hpp>

namespace steemit { namespace chain2 {

   namespace db2 = graphene::db2;

   typedef uint64_t options_type;


   struct block_header : {
      static uint32_t num_from_id(const block_id_type& id);
      uint32_t        block_num()const { return num_from_id(previous) + 1; }

      block_id_type                 previous;
      fc::time_point_sec            timestamp;
      account_name_type             witness;
      checksum_type                 transaction_merkle_root;
      block_header_extensions_type  extensions;
   }

   struct signed_block_header : public block_header
   {
      block_id_type        id()const;
      fc::ecc::public_key  signee()const;
      void                 sign( const fc::ecc::private_key& signer );
      bool                 validate_signee( const fc::ecc::public_key& expected_signee )const;


      signature_type       witness_signature;
   };


   struct signed_block : public signed_block_header
   {
      checksum_type calculate_merkle_root()const;

      vector<signed_transaction> transactions;
   };



   class block_object : public db2::object<0, block_object> {
      public:

      template<typename Constructor, typename Allocator>                   
      block_object( Constructor&& c, allocator<Allocator> a )
      :transactions( transactions_type::allocator_type( a.get_segment_manager() ) )
      { c(*this); }  

      id_type                            id;
      block_id_type                      previous;
      account_name_type                  witness;
      time_point_sec                     timestamp;
      checksum_type                      transaction_merkle_root;
      bip::vector<transaction_id_type, allocator<transaction_id_type> > transactions;

   };

   class transaction_object : public db2::object<1, transaction_object> {
      public:
         template<typename Constructor, typename Allocator> 
         test_object( Constructor&& c, allocator<Allocator> a ){
            c(*this);
         };

   };


   class database : public db2::database {
      public:
         database();
         ~database();

         enum skip_flags {
            skip_undo_block,
            skip_undo_transaction,
            skip_undo_operation,
            skip_validation ///< apply changes without validation checks
         };

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
