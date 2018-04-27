
#include <steem/utilities/storage_configuration_manager.hpp>

#include <boost/algorithm/string.hpp>

#include<iostream>
#include <sys/stat.h>
#include <stdlib.h>

namespace steem { namespace utilities {

namespace storage_configuration_helper
{
   void remove_directory( const bfs::path& path )
   {
      if( bfs::exists( path ) )
      {
         std::string command = "rm -r " + std::string( path.c_str() );
         if( !system( command.c_str() ) )
            ilog( "Problem with removing directory" );
      }
   }

   void create_directory( const bfs::path& path )
   {
      if( !bfs::exists( path ) )
      {
         mkdir( path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#ifdef _DEBUG
         struct stat sb;
         assert( stat(path.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode) );
#endif
      }
   }
}

//************************************storage_configuration_plugin************************************

storage_configuration_plugin::storage_configuration_plugin( const std::string& _name,
                                                            const rocksdb_types::column_definitions_preparer& _col_def_preparer,
                                                            const rocksdb_types::key_value_items& _sequences,
                                                            const rocksdb_types::key_value_items& _version )
:  name( _name ), storage_path( _name ), config_file( "config.ini" ),
   col_def_preparer( _col_def_preparer ), sequences( _sequences ), version( _version )
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

const bool storage_configuration_plugin::exist_config_file() const
{
   return bfs::exists( config_file );
}

void storage_configuration_plugin::set_storage_path( const bfs::path& src )
{
   storage_path = src;
}

const bfs::path& storage_configuration_plugin::get_storage_path() const
{
   return storage_path;
}

const rocksdb_types::column_definitions_preparer& storage_configuration_plugin::get_column_definitions_preparer() const
{
   return col_def_preparer;
}

const rocksdb_types::key_value_items& storage_configuration_plugin::get_sequences() const
{
   return sequences;
}

const rocksdb_types::key_value_items& storage_configuration_plugin::get_version() const
{
   return version;
}

void storage_configuration_plugin::correct_path( const bfs::path& src )
{
   storage_path = src / storage_path;
   config_file = storage_path / config_file;

   storage_configuration_helper::create_directory( storage_path );
}

//************************************storage_configuration_manager************************************

storage_configuration_manager::storage_configuration_manager()
{
}

void storage_configuration_manager::reset()
{
   storage_root_path = bfs::path();
   plugins.clear();
}

template< typename Result, typename Callable >
const Result storage_configuration_manager::get_any_info( const std::string& plugin_name, Callable&& call ) const
{
   static Result empty;

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
            bfs::path _path = v.second. template as<bfs::path>();
            if( is_storage )
               found->second.set_storage_path( _path );
            else
               found->second.set_config_file( _path );
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

   cli_opts.add( cfg_opts );
}

void storage_configuration_manager::create_path( bfs::path& dst, const bfs::path& src )
{
   if( src.is_relative() )
      dst = bfs::current_path() / src;
   else
      dst = src;
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
      bpo::notify( args );

      bfs::path data_dir = "data-dir";

      bfs::path _config_file_name;
      bfs::path _storage_root_path;
      bfs::path config_file_name;

      for( auto& item : args )
      {
         if( item.first == "data-dir" )
            create_path( data_dir, item.second.as<bfs::path>() );
         else if( item.first == "config" )
            _config_file_name = item.second.as<bfs::path>();
         else if( item.first == "storage-root-path" )
            create_path( _storage_root_path, item.second.as<bfs::path>() );
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

      //config.ini for steemd has to exist.
      if( !bfs::exists( config_file_name ) )
      {
         storage_root_path = _storage_root_path;
         return;
      }

      bpo::store(bpo::parse_config_file< char >( config_file_name.make_preferred().string().c_str(),
                                                cfg_opts, true ), args );
      bpo::notify( args );

      for( auto& item : args )
      {
         if( item.first == "storage-root-path" )
            create_path( storage_root_path, item.second.as<bfs::path>() );
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
   storage_configuration_helper::create_directory( storage_root_path );

   action( [this]( storage_configuration_plugin& obj ){ obj.correct_path( storage_root_path ); } );
}

void storage_configuration_manager::initialize( int _argc, char** _argv )
{
   initialize_impl( _argc, _argv );

   correct_paths();
}

void storage_configuration_manager::add_plugin( const std::string& plugin_name,
                                                const rocksdb_types::column_definitions_preparer& _col_def_preparer,
                                                const rocksdb_types::key_value_items& _sequences,
                                                const rocksdb_types::key_value_items& _version )
{
   plugins.emplace( std::make_pair( plugin_name, storage_configuration_plugin( plugin_name, _col_def_preparer, _sequences, _version ) ) );
}

const bfs::path& storage_configuration_manager::get_storage_root_path() const
{
   return storage_root_path;
}

const bfs::path storage_configuration_manager::get_config_file( const std::string& plugin_name ) const
{
   return get_any_info< bfs::path >( plugin_name, []( const storage_configuration_plugin& obj ){ return obj.get_config_file(); } );
}

const bool storage_configuration_manager::exist_config_file( const std::string& plugin_name ) const
{
   return get_any_info< bool >( plugin_name, []( const storage_configuration_plugin& obj ){ return obj.exist_config_file(); } );
}

const bfs::path storage_configuration_manager::get_storage_path( const std::string& plugin_name ) const
{
   return get_any_info< bfs::path >( plugin_name, []( const storage_configuration_plugin& obj ){ return obj.get_storage_path(); } );
}

rocksdb_types::column_definitions_preparer storage_configuration_manager::get_column_definitions_preparer( const std::string& plugin_name ) const
{
   return get_any_info< rocksdb_types::column_definitions_preparer >( plugin_name, []( const storage_configuration_plugin& obj ){ return obj.get_column_definitions_preparer(); } );
}

rocksdb_types::key_value_items storage_configuration_manager::get_sequences( const std::string& plugin_name ) const
{
   return get_any_info< rocksdb_types::key_value_items >( plugin_name, []( const storage_configuration_plugin& obj ){ return obj.get_sequences(); } );
}

rocksdb_types::key_value_items storage_configuration_manager::get_version( const std::string& plugin_name ) const
{
   return get_any_info< rocksdb_types::key_value_items >( plugin_name, []( const storage_configuration_plugin& obj ){ return obj.get_version(); } );
}

} }
