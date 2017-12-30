#pragma once

#include <steem/plugins/chain/chain_plugin.hpp>

#include <appbase/application.hpp>

#include <memory>

namespace steem { namespace plugins { namespace rocksdb {

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

private:
   class impl;

   std::unique_ptr<impl> _my;
   bfs::path             _dbPath;
};


} } } // steem::plugins::rocksdb