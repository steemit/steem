
#include <boost/algorithm/string.hpp>

#include <boost/program_options.hpp>

#include <fc/exception/exception.hpp>
#include <fc/io/fstream.hpp>

#include <steemit/object_builder/builder_state.hpp>

#include <fstream>
#include <iostream>

int main( int argc, char** argv, char** envp )
{
   try
   {
      boost::program_options::options_description odesc("Steem object builder");
      odesc.add_options()
         ("help,h"    , "Print this help message and exit")
         ("outdir,o"  , boost::program_options::value<std::string>(), "Output directory (created if it does not exist)")
         ("list,l"    , boost::program_options::value<std::string>(), "Input listing of directories containing .object files")
         ("template,t", boost::program_options::value<std::string>(), "Template file")
         ;

      boost::program_options::variables_map options;
      boost::program_options::store( boost::program_options::parse_command_line( argc, argv, odesc ), options );

      if( options.count("help") )
      {
         std::cout << odesc << std::endl;
         return 0;
      }

      FC_ASSERT( options.count( "outdir" ) );
      FC_ASSERT( options.count( "list" ) );
      FC_ASSERT( options.count( "template" ) );

      steemit::object_builder::builder_state s;
      fc::read_file_contents( options.at("template").as<std::string>(), s.output_template );

      std::ifstream listfile( options.at("list").as<std::string>() );
      while( !listfile.eof() )
      {
         std::string line;
         listfile >> line;
         boost::trim( line );
         if( line.size() == 0 )
            continue;
         s.process_directory( line );
      }

      s.generate( options.at("outdir").as<std::string>() );
   }
   catch( const fc::exception& e )
   {
      ilog( "caught exception:\n${e}", ("e", e.to_detail_string()) );
      throw;
   }
   return 0;
}
