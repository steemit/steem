#pragma once
#include <graphene/db2/database.hpp>
#include <steemit/protocol/block.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/interprocess/containers/vector.hpp>

#include <fc/signals.hpp>

namespace steemit { namespace chain2 {

   using namespace steemit::protocol;
   using namespace boost::multi_index;

   namespace db2 = graphene::db2;
   using db2::allocator;

   typedef uint64_t options_type;

   typedef bip::vector<char, allocator<char> >                                                  buffer_type; 
   typedef buffer_type                                                                          packed_operation_type;
   typedef bip::vector<packed_operation_type, typename packed_operation_type::allocator_type>   packed_operations_type;

   struct by_id;
   struct by_ref_prefix;
   struct by_trx_id;
   struct by_block_num;
   struct by_timestamp;

   enum skip_flags {
      skip_undo_block,
      skip_undo_transaction,
      skip_undo_operation,
      skip_validation ///< apply changes without validation checks
   };

   class block_object : public db2::object<0, block_object> {
      public:

      template<typename Constructor, typename Allocator>                   
      block_object( Constructor&& c, allocator<Allocator> a )
      :transactions( allocator< transaction_id_type >( a.get_segment_manager() ) )
      { c(*this); }  

      id_type                            id;
      uint32_t                           ref_prefix = 0; ///< used for TaPoS checks
      uint32_t                           block_num = 0; ///< used for TaPoS checks
      block_id_type                      block_id;
      block_id_type                      previous;
      account_name_type                  witness;
      time_point_sec                     timestamp;
      checksum_type                      transaction_merkle_root;
      signature_type                     witness_signature;
      bip::vector<transaction_id_type, allocator<transaction_id_type> > transactions;
   };

   typedef multi_index_container <
     block_object,
     indexed_by<
        ordered_unique< tag< by_id >, member<block_object,block_object::id_type, &block_object::id> >,
        ordered_unique< tag< by_block_num >, member<block_object,uint32_t, &block_object::block_num> >,
        ordered_unique< tag< by_timestamp >, member<block_object,time_point_sec, &block_object::timestamp>, std::greater<time_point_sec> >,
        ordered_unique< tag< by_ref_prefix >,
           composite_key< block_object,
              member< block_object, uint32_t, &block_object::ref_prefix >,
              member< block_object, uint32_t, &block_object::block_num >
           >,
           composite_key_compare< std::less<uint32_t>, std::greater<uint32_t> >
        >
     >,
     bip::allocator<block_object,bip::managed_mapped_file::segment_manager>
   > block_index;


   class transaction_object : public db2::object<1, transaction_object> {
      public:
         template<typename Constructor, typename Allocator> 
         transaction_object( Constructor&& c, allocator<Allocator> a )
         :packed_transaction( buffer_type::allocator_type( a.get_segment_manager() ) ) 
         {
            c(*this);
         };

         id_type                  id;
         uint32_t                 block_num = 0;  ///< block the transaction was included in
         transaction_id_type      trx_id;
         buffer_type              packed_transaction;
   };


   typedef multi_index_container <
     transaction_object,
     indexed_by<
        ordered_unique< tag< by_id >, 
            member<transaction_object,transaction_object::id_type, &transaction_object::id> >,
        ordered_unique< tag< by_trx_id >, 
            member< transaction_object, transaction_id_type, &transaction_object::trx_id > >
     >,
     bip::allocator<transaction_object,bip::managed_mapped_file::segment_manager>
   > transaction_index;


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

GRAPHENE_DB2_SET_INDEX_TYPE( steemit::chain2::block_object, steemit::chain2::block_index );
GRAPHENE_DB2_SET_INDEX_TYPE( steemit::chain2::transaction_object, steemit::chain2::transaction_index );


