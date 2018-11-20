
#pragma once

#include <memory>

#include <steem/protocol/types.hpp>

namespace steem { namespace schema {
   struct abstract_schema;
} }

namespace steem { namespace protocol {
   struct custom_json_operation;
} }

namespace steem { namespace chain {

using steem::protocol::custom_id_type;

class custom_operation_interpreter
{
   public:
      virtual void apply( const protocol::custom_json_operation& op ) = 0;
      virtual void apply( const protocol::custom_binary_operation & op ) = 0;
      virtual custom_id_type get_custom_id() = 0;
      virtual std::shared_ptr< steem::schema::abstract_schema > get_operation_schema() = 0;
};

class custom_operation_notification
{
   public:
      custom_operation_notification( const custom_id_type& cid ) : custom_id(cid) {}
      virtual ~custom_operation_notification() {}

      template< typename OpType >
      const OpType& get_op()const;
      template< typename OpType >
      const OpType* maybe_get_op() const;

      // const operation_notification& outer_note;
      // One day we might implement outer_note, but some core plumbing will need to be re-organized
      // to route operation_notification into core evaluators.  For now, a plugin that needs this
      // information can save it in the pre-eval handler for the custom operation.

      custom_id_type        custom_id;
      uint32_t              op_in_custom = 0;
};

} } // steem::chain
