#include <appbase/application.hpp>
#include <iostream>
#include <boost/exception/diagnostic_information.hpp>

struct database { };

namespace bpo = boost::program_options;

using bpo::options_description;
using bpo::variables_map;
using std::string;
using std::vector;

class plugin_a : public appbase::plugin<plugin_a>
{
   public:
     APPBASE_PLUGIN_REQUIRES();

     static const std::string& name() { static std::string name = "plugin_a"; return name; }

     virtual void set_program_options( options_description& cli, options_description& cfg ) override
     {
         cfg.add_options()
               ("dbsize", bpo::value<uint64_t>()->default_value( 8*1024 ), "Minimum size MB of database shared memory file")
               ;
         cli.add_options()
               ("replay", "clear plugin_a database and replay all blocks" )
               ("reset", "clear plugin_a database and block log" )
               ;
     }

    virtual void plugin_initialize( const variables_map& options ) override
    {
        std::cout << "initialize plugin_a plugin\n";
    }
    virtual void plugin_startup() override
    {
       std::cout << "starting plugin_a plugin \n";
    }
    
    virtual void plugin_shutdown() override
    {
      std::cout << "shutdown plugin_a plugin \n";
    }

     database& db() { return _db; }

   private:
     database _db;
};

class plugin_b : public appbase::plugin<plugin_b>
{
   public:
     plugin_b(){};
     ~plugin_b(){};

     APPBASE_PLUGIN_REQUIRES( (plugin_a) );

     static const std::string& name() { static std::string name = "plugin_b"; return name; }

     virtual void set_program_options( options_description& cli, options_description& cfg ) override
     {
        cfg.add_options()
              ("listen-endpoint", bpo::value<string>()->default_value( "127.0.0.1:9876" ), "The local IP address and port to listen for incoming connections.")
              ("remote-endpoint", bpo::value< vector<string> >()->composing(), "The IP address and port of a remote peer to sync with.")
              ("public-endpoint", bpo::value<string>()->default_value( "0.0.0.0:9876" ), "The public IP address and port that should be advertized to peers.")
              ;
     }

    protected:
      virtual void plugin_initialize( const variables_map& options ) override
      {
      std::cout << "initialize plugin_b plugin\n";
      }
     
      virtual void plugin_startup() override
      {
         std::cout << "starting plugin_b plugin \n";
      }

      virtual void plugin_shutdown() override
      {
         std::cout << "shutdown plugin_b plugin \n";
      }

};



int main( int argc, char** argv ) {
   try {
      appbase::app().register_plugin<plugin_b>();
      if( !appbase::app().initialize( argc, argv ) )
         return -1;
      appbase::app().startup();
      appbase::app().exec();
   } catch ( const boost::exception& e ) {
      std::cerr << boost::diagnostic_information(e) << "\n";
   } catch ( const std::exception& e ) {
      std::cerr << e.what() << "\n";
   } catch ( ... ) {
      std::cerr << "unknown exception\n";
   }
   std::cout << "exited cleanly\n";
   return 0;
}
