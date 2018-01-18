#pragma once

#include <steem/plugins/chain/chain_plugin.hpp>

#include <appbase/application.hpp>

#include <functional>
#include <memory>

namespace steem {
   
namespace chain
{
class account_history_object;
class operation_object;
} /// namespace chain

namespace plugins { namespace rocksdb {

namespace bfs = boost::filesystem;

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

   bool find_account_history_data(const protocol::account_name_type& name, steem::chain::account_history_object* data) const;
   bool find_operation_object(size_t opId, steem::chain::operation_object* data) const;
   void find_operations_by_block(size_t blockNum,
      std::function<void(const steem::chain::operation_object&)> processor) const;

private:
   class impl;

   std::unique_ptr<impl> _my;
   bfs::path             _dbPath;
   uint32_t              _blockLimit = 0;
};


} } } // steem::plugins::rocksdb