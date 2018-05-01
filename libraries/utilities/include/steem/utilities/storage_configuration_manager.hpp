#pragma once

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

#include<string>
#include<map>

namespace steem { namespace utilities {

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

using bpo::options_description;
using bpo::variables_map;

class storage_configuration_plugin
{
   private:
  
      const std::string name;

      bfs::path storage_path;
      bfs::path config_file;

   public:

      storage_configuration_plugin( const std::string& _name );
      storage_configuration_plugin( const std::string& _name, const bfs::path& _storage_path, const bfs::path& _config_file );

      const std::string& get_name() const;

      void set_config_file( const bfs::path& src );
      const bfs::path& get_config_file() const;
      const bool exist_config_file() const;

      void set_storage_path( const bfs::path& src );
      const bfs::path& get_storage_path() const;

      void correct_path( const bfs::path& src );
};

class storage_configuration_manager
{
   private:

      using t_plugins = std::map< std::string, storage_configuration_plugin >;

      bfs::path storage_root_path;

      t_plugins plugins;

      template< typename Result, typename Callable >
      const Result get_any_info( const std::string& plugin_name, Callable&& call ) const;

      template< typename Callable >
      void action( Callable&& call );

      template< typename Variable >
      void find_plugin( Variable& v );

      void default_values( options_description& cli_opts, options_description& cfg_opts );
      void create_path( bfs::path& dst, const bfs::path& src );
      void initialize_impl( int _argc, char** _argv );
      void correct_paths();

   public:

      storage_configuration_manager();

      void initialize( int _argc, char** _argv );
      void add_plugin( const std::string& plugin_name );

      const bfs::path& get_storage_root_path() const;

      const bfs::path get_config_file( const std::string& plugin_name ) const;
      const bool exist_config_file( const std::string& plugin_name ) const;

      const bfs::path get_storage_path( const std::string& plugin_name ) const;
};

} } // steem::utilities
