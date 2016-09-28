#pragma once
#include <steemit/chain2/object_types.hpp>
#include <steemit/protocol/block.hpp>

namespace steemit { namespace chain2 {

   using namespace steemit::protocol;

   struct by_id;
   struct by_ref_prefix;
   struct by_trx_id;
   struct by_block_num;
   struct by_timestamp;

   class block_object : public db2::object<block_object_type, block_object> {
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


   class transaction_object : public db2::object<transaction_object_type, transaction_object> {
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

} } /// namespace steemit::chain2

FC_REFLECT( steemit::chain2::block_object, 
            (id)(ref_prefix)(block_num)(block_id)(previous)(witness)(timestamp)
            (transaction_merkle_root)(witness_signature)(transactions) )
GRAPHENE_DB2_SET_INDEX_TYPE( steemit::chain2::block_object, steemit::chain2::block_index );


FC_REFLECT( steemit::chain2::transaction_object, (id)(block_num)(trx_id)(packed_transaction) );
GRAPHENE_DB2_SET_INDEX_TYPE( steemit::chain2::transaction_object, steemit::chain2::transaction_index );
