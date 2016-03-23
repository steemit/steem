#pragma once
#include <graphene/db/object.hpp>

namespace steemit { namespace chain {
   using namespace graphene::db;

   class block_summary_object : public abstract_object<block_summary_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_block_summary_object_type;

         block_id_type      block_id;
   };

} }

FC_REFLECT_DERIVED( steemit::chain::block_summary_object, (graphene::db::object), (block_id) )
