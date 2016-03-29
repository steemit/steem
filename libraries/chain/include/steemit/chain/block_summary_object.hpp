#pragma once
#include <graphene/db/object.hpp>

namespace steemit { namespace chain {
   using namespace graphene::db;

   /**
    *  @brief tracks minimal information about past blocks to implement TaPOS
    *  @ingroup object
    *
    *  When attempting to calculate the validity of a transaction we need to
    *  lookup a past block and check its block hash and the time it occurred
    *  so we can calculate whether the current transaction is valid and at
    *  what time it should expire.
    */
   class block_summary_object : public abstract_object<block_summary_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_block_summary_object_type;

         block_id_type      block_id;
   };

} }

FC_REFLECT_DERIVED( steemit::chain::block_summary_object, (graphene::db::object), (block_id) )
