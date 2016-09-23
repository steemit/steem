
#pragma once

#include <memory>

namespace graphene { namespace db {
struct abstract_schema;
} }

namespace steemit { namespace chain {

struct custom_json_operation;

class custom_operation_interpreter
{
   public:
      virtual void apply( const custom_json_operation& op ) = 0;
      virtual void apply( const custom_binary_operation & op ) = 0;
      virtual std::shared_ptr< graphene::db::abstract_schema > get_operation_schema() = 0;
};

} } // steemit::chain
