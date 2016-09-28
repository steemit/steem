#pragma once
#include <steemit/chain/steem_object_types.hpp>

#include <graphene/db/object.hpp>

namespace steemit { namespace chain {
   using namespace graphene::db;

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
   class block_summary_object : public object< impl_block_summary_object_type, block_summary_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         block_summary_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         id_type        id;
         block_id_type  block_id;
   };

   typedef multi_index_container<
      block_summary_object,
      indexed_by<
         ordered_unique< member< block_summary_object, id_type, &block_summary_object::id > >
      >,
      bip::allocator< block_summary_object, bip::managed_mapped_file::segment_manager >
   > block_summary_index;

} }

FC_REFLECT( steemit::chain::block_summary_object, (id)(block_id) )
