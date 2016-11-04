#pragma once
#include <steemit/chain/steem_object_types.hpp>

namespace steemit { namespace chain {

   using steemit::protocol::block_id_type;

   /**
    *  @brief tracks minimal information about past blocks to implement TaPOS
    *  @ingroup object
    *
    *  When attempting to calculate the validity of a transaction we need to
    *  lookup a past block and check its block hash and the time it occurred
    *  so we can calculate whether the current transaction is valid and at
    *  what time it should expire.
    */
   class block_summary_object : public object< block_summary_object_type, block_summary_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         block_summary_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         block_summary_object(){};

         id_type        id;
         block_id_type  block_id;
   };

   typedef multi_index_container<
      block_summary_object,
      indexed_by<
         ordered_unique< tag< by_id >,
            member< block_summary_object, block_summary_object::id_type, &block_summary_object::id > >
      >,
      allocator< block_summary_object >
   > block_summary_index;

   class block_stats_object : public object< block_stats_object_type, block_stats_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         block_stats_object( Constructor&& c, allocator< Allocator > a )
            :packed_block( a )
         {
            c( *this );
         }

         id_type        id;
         block_id_type  block_id;
         uint64_t       pos = -1;
         bip::vector< char, allocator< char > > packed_block;

         uint64_t block_num()const { return id._id + 1; }
   };

   struct by_block_id;

   typedef multi_index_container<
      block_stats_object,
      indexed_by<
         ordered_unique< tag< by_id >,
            member< block_stats_object, block_stats_id_type, &block_stats_object::id > >,
         ordered_unique< tag< by_block_id >,
            member< block_stats_object, block_id_type, &block_stats_object::block_id > >
      >,
      allocator< block_stats_object >
   > block_stats_index;

} } // steemit::chain

FC_REFLECT( steemit::chain::block_summary_object, (id)(block_id) )
CHAINBASE_SET_INDEX_TYPE( steemit::chain::block_summary_object, steemit::chain::block_summary_index )

FC_REFLECT( steemit::chain::block_stats_object, (id)(block_id)(pos)(packed_block) )
CHAINBASE_SET_INDEX_TYPE( steemit::chain::block_stats_object, steemit::chain::block_stats_index )
