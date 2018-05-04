#pragma once

#include <steem/chain/steem_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>

#ifndef STEEM_ACCOUNT_HISTORY_ROCKSDB_SPACE_ID
#define STEEM_ACCOUNT_HISTORY_ROCKSDB_SPACE_ID 15
#endif

namespace steem { namespace plugins { namespace account_history_rocksdb {

using namespace steem::chain;

typedef std::vector<char> serialize_buffer_t;

enum account_history_rocksdb_object_types
{
   volatile_operation_object_type = ( STEEM_ACCOUNT_HISTORY_ROCKSDB_SPACE_ID << 8 )
};

class volatile_operation_object : public object< volatile_operation_object_type, volatile_operation_object >
{
   volatile_operation_object() = delete;

   public:
      template< typename Constructor, typename Allocator >
      volatile_operation_object( Constructor&& c, allocator< Allocator > a )
         :serialized_op( a ), impacted( a )
      {
         c( *this );
      }

      id_type                    id;

      chain::transaction_id_type trx_id;
      uint32_t                   block = 0;
      uint32_t                   trx_in_block = 0;
      uint32_t                   op_in_trx = 0;
      uint32_t                   virtual_op = 0;
      time_point_sec             timestamp;
      chain::buffer_type         serialized_op;
      chainbase::t_vector< account_name_type > impacted;
};

typedef volatile_operation_object::id_type volatile_operation_id_type;

/** Dedicated definition is needed because of conflict of BIP allocator
 *  against usage of this class as temporary object.
 *  The conflict appears in original serialized_op container type definition,
 *  which in BIP version needs an allocator during constructor call.
 */
class rocksdb_operation_object
{
   public:
      rocksdb_operation_object() {}
      rocksdb_operation_object( const volatile_operation_object& o ) :
         trx_id( o.trx_id ),
         block( o.block ),
         trx_in_block( o.trx_in_block ),
         op_in_trx( o.op_in_trx ),
         virtual_op( o.virtual_op ),
         timestamp( o.timestamp )
      {
         serialized_op.insert( serialized_op.end(), o.serialized_op.begin(), o.serialized_op.end() );
      }

      int64_t                    id = 0;

      chain::transaction_id_type trx_id;
      uint32_t                   block = 0;
      uint32_t                   trx_in_block = 0;
      uint16_t                   op_in_trx = 0;
      uint16_t                   virtual_op = 0;
      time_point_sec             timestamp;
      serialize_buffer_t         serialized_op;
};

struct by_block;

typedef multi_index_container<
      volatile_operation_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< volatile_operation_object, volatile_operation_id_type, &volatile_operation_object::id > >,
         ordered_unique< tag< by_block >,
            composite_key< volatile_operation_object,
               member< volatile_operation_object, uint32_t, &volatile_operation_object::block>,
               member< volatile_operation_object, volatile_operation_id_type, &volatile_operation_object::id>
            >
         >
      >,
      allocator< volatile_operation_object >
   > volatile_operation_index;

} } } // steem::plugins::account_history_rocksdb

FC_REFLECT( steem::plugins::account_history_rocksdb::volatile_operation_object, (id)(trx_id)(block)(trx_in_block)(op_in_trx)(virtual_op)(timestamp)(serialized_op)(impacted) )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::account_history_rocksdb::volatile_operation_object, steem::plugins::account_history_rocksdb::volatile_operation_index )

FC_REFLECT( steem::plugins::account_history_rocksdb::rocksdb_operation_object, (id)(trx_id)(block)(trx_in_block)(op_in_trx)(virtual_op)(timestamp)(serialized_op) )
