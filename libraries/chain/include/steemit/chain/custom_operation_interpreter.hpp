
#pragma once

#include <memory>

namespace graphene { namespace schema {
   struct abstract_schema;
} }

namespace steemit { namespace protocol {
   struct custom_json_operation;
} }

namespace steemit { namespace chain {

class custom_operation_interpreter
{
   public:
      virtual void apply( const protocol::custom_json_operation& op ) = 0;
      virtual void apply( const protocol::custom_binary_operation & op ) = 0;
      virtual std::shared_ptr< graphene::schema::abstract_schema > get_operation_schema() = 0;
};

} } // steemit::chain
