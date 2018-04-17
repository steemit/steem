
#include <steem/utilities/storage_configuration_manager.hpp>

#include <boost/algorithm/string.hpp>

#include<iostream>

namespace steem { namespace utilities {

//************************************storage_configuration_plugin************************************

storage_configuration_plugin::storage_configuration_plugin( const std::string& _name )
: name( _name ), storage_path( bfs::path() ), config_file( bfs::path() )
{

}

storage_configuration_plugin::storage_configuration_plugin( const std::string& _name, const bfs::path& _storage_path, const bfs::path& _config_file )
: name( _name ), storage_path( _storage_path ), config_file( _config_file )
{

}

const std::string& storage_configuration_plugin::get_name() const
{
   return name;
}

void storage_configuration_plugin::set_config_file( const bfs::path& src )
{
   config_file = src;
}

const bfs::path& storage_configuration_plugin::get_config_file() const
{
   return config_file;
}

void storage_configuration_plugin::set_storage_path( const bfs::path& src )
{
   storage_path = src;
}

const bfs::path& storage_configuration_plugin::get_storage_path() const
{
   return storage_path;
}

void storage_configuration_plugin::correct_path( const bfs::path& src )
{
   storage_path = src / storage_path;
   config_file = src / config_file;
}

//************************************storage_configuration_manager************************************

storage_configuration_manager::storage_configuration_manager()
{
}

template< typename Callable >
const bfs::path storage_configuration_manager::get_any_path( const std::string& plugin_name, Callable&& call ) const
{
   static bfs::path empty;

   auto found = plugins.find( plugin_name );

   if( found == plugins.end() )
      return empty;
   else
      return call( found->second );
}

template< typename Callable >
void storage_configuration_manager::action( Callable&& call )
{
   for( auto& item : plugins )
      call( item.second );
}

template< typename Variable >
void storage_configuration_manager::find_plugin( Variable& v )
{
   /*
      --<plugin_name>-storage-path=<path> - allows to specify storage location for specific plugin
      --<plugin_name>-configuration-file=<file> - allows to specify configuration specific to given plugin’s storage 
   */
   std::vector< std::string > items;
   boost::split( items, v.first, boost::is_any_of( "-" ) );

   if( items.size() == 3 )
   {
      bool is_storage = items[1] =="storage" && items[2] == "path";
      bool is_config_file = items[1] == "configuration" && items[2] == "file";

      if( is_storage || is_config_file )
      {
         auto found = plugins.find( items[0] );

         if( found == plugins.end() )
         {
            if( is_storage )
               plugins.emplace( std::make_pair( items[0], storage_configuration_plugin( items[0], v.second. template as< bfs::path >(), bfs::path() ) ) );
            else
               plugins.emplace( std::make_pair( items[0], storage_configuration_plugin( items[0], bfs::path(), v.second. template as< bfs::path >() ) ) );
         }
         else
         {
            if( is_storage )
               found->second.set_storage_path( v.second. template as<bfs::path>() );
            else
               found->second.set_config_file( v.second. template as<bfs::path>() );
         }
      }
   }
}

void storage_configuration_manager::default_values( options_description& cli_opts, options_description& cfg_opts )
{
   cli_opts.add_options()
         ("storage-root-path", bpo::value<bfs::path>()->default_value( "witness_node_data_dir" ), "Root path for all storage databases")
         ("data-dir,d", bpo::value<bfs::path>()->default_value( "witness_node_data_dir" ), "Directory containing configuration file config.ini")
         ("config,c", bpo::value<bfs::path>()->default_value( "config.ini" ), "Configuration file name relative to data-dir");

   /*
      --<plugin_name>-storage-path
      --<plugin_name>-configuration-file
   */
   for( auto& item : plugins )
   {
      std::string sp = item.first + "-storage-path";
      std::string sp_default = item.first + "-data-dir";
      std::string cf = item.first + "-configuration-file";

      cfg_opts.add_options()
         ( sp.c_str(), bpo::value<bfs::path>()->default_value( sp_default.c_str() ), "Storage location for specific plugin")
         ( cf.c_str(), bpo::value<bfs::path>()->default_value( "config.ini" ), "Configuration specific to given plugin’s storage");
   }
}

void storage_configuration_manager::initialize_impl( int _argc, char** _argv )
{
   /*
      --storage-root-path=<path> - allows to specify root path for all storage databases (located in subdirectories)
   */
   try
   {
      options_description cli_opts;//command-line options
      options_description cfg_opts;//command-line options

      variables_map args;

      args.clear();
      storage_root_path = "";

      default_values( cli_opts, cfg_opts );

      bpo::store( bpo::command_line_parser( _argc, _argv ).options( cli_opts ).allow_unregistered().run(), args );

      bfs::path data_dir = "data-dir";

      bfs::path _config_file_name;
      bfs::path config_file_name;

      for( auto& item : args )
      {
         if( item.first == "data-dir" )
         {
            data_dir = item.second.as<bfs::path>();
            if( data_dir.is_relative() )
               data_dir = bfs::current_path() / data_dir;
         }
         else if( item.first == "config" )
         {
            _config_file_name = item.second.as<bfs::path>();
         }
      }

      if( _config_file_name.empty() )
         config_file_name = data_dir / "config.ini";
      else
      {
         if( _config_file_name.is_relative() )
            config_file_name = data_dir / _config_file_name;
         else
            config_file_name = _config_file_name;
      }

      bpo::store(bpo::parse_config_file< char >( config_file_name.make_preferred().string().c_str(),
                                                cfg_opts, true ), args );

      for( auto& item : args )
      {
         if( item.first == "storage-root-path" )
         {
            storage_root_path = item.second.as<bfs::path>();
            if( storage_root_path.is_relative() )
               storage_root_path = bfs::current_path() / storage_root_path;
         }
         else
            find_plugin( item );
      }
   }
   catch (const boost::program_options::error& e)
   {
      std::cerr << "Error parsing command line: " << e.what() << "\n";
   }
}

void storage_configuration_manager::correct_paths()
{
   action( [this]( storage_configuration_plugin& obj ){ obj.correct_path( storage_root_path ); } );
}

void storage_configuration_manager::initialize( int _argc, char** _argv )
{
   initialize_impl( _argc, _argv );

   correct_paths();
}

void storage_configuration_manager::add_plugin( const std::string& plugin_name )
{
   plugins.emplace( std::make_pair( plugin_name, storage_configuration_plugin( plugin_name ) ) );
}

const bfs::path& storage_configuration_manager::get_storage_root_path() const
{
   return storage_root_path;
}

const bfs::path storage_configuration_manager::get_config_file( const std::string& plugin_name ) const
{
   return get_any_path( plugin_name, []( const storage_configuration_plugin& obj ){ return obj.get_config_file(); } );
}

const bfs::path storage_configuration_manager::get_storage_path( const std::string& plugin_name ) const
{
   return get_any_path( plugin_name, []( const storage_configuration_plugin& obj ){ return obj.get_storage_path(); } );
}

} }
