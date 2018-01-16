#pragma once
#include <steem/plugins/json_rpc/utility.hpp>

#include <steem/chain/history_object.hpp>

#include <steem/protocol/operations.hpp>
#include <steem/protocol/types.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>

namespace steem { namespace plugins {

namespace rocksdb {

class rocksdb_plugin;

namespace detail {class api_impl;}

struct api_operation_object
{
   api_operation_object() = default;
   api_operation_object( const steem::chain::operation_object& op_obj ) :
      trx_id( op_obj.trx_id ),
      block( op_obj.block ),
      trx_in_block( op_obj.trx_in_block ),
      virtual_op( op_obj.virtual_op() ),
      timestamp( op_obj.timestamp )
   {
      op = fc::raw::unpack< steem::protocol::operation >( op_obj.serialized_op );
   }

   steem::protocol::transaction_id_type trx_id;
   uint32_t                             block = 0;
   uint32_t                             trx_in_block = 0;
   uint16_t                             op_in_trx = 0;
   uint64_t                             virtual_op = 0;
   fc::time_point_sec                   timestamp;
   steem::protocol::operation           op;
};

struct get_ops_in_block_args
{
   uint32_t block_num;
   bool     only_virtual;
};

struct get_ops_in_block_return
{
   vector< api_operation_object > ops;
};

struct get_account_history_args
{
   steem::protocol::account_name_type   account;
   uint64_t                             start = -1;
   uint32_t                             limit = 1000;
};

struct get_account_history_return
{
   std::map< uint32_t, api_operation_object > history;
};

class rocksdb_api final
{
   public:
      explicit rocksdb_api(const rocksdb::rocksdb_plugin& dataSource);
      ~rocksdb_api();

      DECLARE_API(
         (get_ops_in_block)
         (get_account_history)
      )

   private:
      std::unique_ptr<detail::api_impl> my;
};

} } } // steem::plugins::rocksdb

FC_REFLECT( steem::plugins::rocksdb::api_operation_object,
   (trx_id)(block)(trx_in_block)(op_in_trx)(virtual_op)(timestamp)(op) )

FC_REFLECT( steem::plugins::rocksdb::get_ops_in_block_args,
   (block_num)(only_virtual) )

FC_REFLECT( steem::plugins::rocksdb::get_ops_in_block_return,
   (ops) )

FC_REFLECT( steem::plugins::rocksdb::get_account_history_args,
   (account)(start)(limit) )

FC_REFLECT( steem::plugins::rocksdb::get_account_history_return,
   (history) )
