
#pragma once

#include <memory>

#include <dpn/protocol/types.hpp>

namespace dpn { namespace schema {
   struct abstract_schema;
} }

namespace dpn { namespace protocol {
   struct custom_json_operation;
} }

namespace dpn { namespace chain {

class custom_operation_interpreter
{
   public:
      virtual void apply( const protocol::custom_json_operation& op ) = 0;
      virtual void apply( const protocol::custom_binary_operation & op ) = 0;
      virtual dpn::protocol::custom_id_type get_custom_id() = 0;
      virtual std::shared_ptr< dpn::schema::abstract_schema > get_operation_schema() = 0;
};

} } // dpn::chain
