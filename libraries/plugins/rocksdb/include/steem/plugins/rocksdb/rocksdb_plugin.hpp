#pragma once

#include <steem/plugins/chain/chain_plugin.hpp>

#include <appbase/application.hpp>

#include <functional>
#include <memory>

namespace steem {

namespace plugins { namespace rocksdb {

namespace bfs = boost::filesystem;

typedef std::vector<char> serialize_buffer_t;

/** Dedicated definition is needed because of conflict of BIP allocator
 *  against usage of this class as temporary object.
 *  The conflict appears in original serialized_op container type definition,
 *  which in BIP version needs an allocator during constructor call.
 */
class rocksdb_operation_object
{
public:
   uint32_t             id = 0;

   chain::transaction_id_type  trx_id;
   uint32_t             block = 0;
   uint32_t             trx_in_block = 0;
   /// Seems that this member is never used
   //uint16_t             op_in_trx = 0;
   /// Seems that this member is never modified
   //uint64_t           virtual_op = 0;
   uint64_t             get_virtual_op() const { return 0;}
   time_point_sec       timestamp;
   serialize_buffer_t   serialized_op;

};

class rocksdb_plugin final : public appbase::plugin< rocksdb_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES((steem::plugins::chain::chain_plugin))

   rocksdb_plugin();
   virtual ~rocksdb_plugin();

   static const std::string& name() { static std::string name = "rocksdb_plugin"; return name; }

   virtual void set_program_options(
      boost::program_options::options_description &command_line_options,
      boost::program_options::options_description &config_file_options
      ) override;

   virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;

   void find_account_history_data(const protocol::account_name_type& name, uint64_t start, uint32_t limit,
      std::function<void(unsigned int, const rocksdb_operation_object&)> processor) const;
   bool find_operation_object(size_t opId, rocksdb_operation_object* data) const;
   void find_operations_by_block(size_t blockNum,
      std::function<void(const rocksdb_operation_object&)> processor) const;
   uint32_t enum_operations_from_block_range(uint32_t blockRangeBegin, uint32_t blockRangeEnd,
      std::function<void(const rocksdb_operation_object&)> processor) const;

private:
   class impl;

   std::unique_ptr<impl> _my;
   uint32_t              _blockLimit = 0;
   bool                  _doImmediateImport = false;
};


} } } // steem::plugins::rocksdb

FC_REFLECT( steem::plugins::rocksdb::rocksdb_operation_object, (id)(trx_id)(block)(trx_in_block)(timestamp)(serialized_op) )
