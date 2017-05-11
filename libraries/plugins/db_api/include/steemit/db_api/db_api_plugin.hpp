#pragma once
#include <steemit/app/plugin.hpp>
#include <steemit/chain/database.hpp>

namespace steemit { namespace db_api {

using steemit::app::application;

#define DB_API_PLUGIN_NAME "db_api_plugin"

class db_api_plugin : public steemit::app::plugin
{
   public:
      db_api_plugin( application* app ) : plugin( app ) {}

      std::string plugin_name()const override { return DB_API_PLUGIN_NAME; }
      virtual void plugin_set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg ) override {}
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override {}
      virtual void plugin_startup() override;
};

} } // steemit::db_api
