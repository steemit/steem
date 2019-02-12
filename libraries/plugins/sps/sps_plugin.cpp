#include <steem/plugins/sps/sps_plugin.hpp>
#include <steem/plugins/sps/sps_objects.hpp>
#include <steem/plugins/sps/sps_operations.hpp>

#include <steem/chain/notifications.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/index.hpp>

#include <steem/chain/generic_custom_operation_interpreter.hpp>

namespace steem { namespace plugins { namespace sps {

using namespace steem::chain;
using namespace steem::protocol;

using steem::chain::block_notification;
using steem::chain::generic_custom_operation_interpreter;

namespace detail {

class sps_plugin_impl
{
   private:

      chain::database& _db;
      sps_plugin& _self;

      boost::signals2::connection _on_proposal_processing;
      std::shared_ptr< generic_custom_operation_interpreter< sps_plugin_operation > > _custom_operation_interpreter;

   public:

      sps_plugin_impl( sps_plugin& _plugin ) :
         _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ),
         _self( _plugin ) {}

      ~sps_plugin_impl() {}

      void initialize();

      void on_proposal_processing( const block_notification& note );

      void plugin_shutdown();
};

void sps_plugin_impl::initialize()
{
   _on_proposal_processing = _db.add_on_proposal_processing_handler(
         [&]( const block_notification& note ){ on_proposal_processing( note ); }, _self, 0 );

   // Each plugin needs its own evaluator registry.
   _custom_operation_interpreter = std::make_shared< generic_custom_operation_interpreter< sps_plugin_operation > >( _db, _self.name() );

   // Add each operation evaluator to the registry
   _custom_operation_interpreter->register_evaluator< create_proposal_evaluator >( &_self );
   _custom_operation_interpreter->register_evaluator< update_proposal_votes_evaluator >( &_self );

   // Add the registry to the database so the database can delegate custom ops to the plugin
   _db.register_custom_operation_interpreter( _custom_operation_interpreter );

   add_plugin_index< proposal_index      >( _db );
   add_plugin_index< proposal_vote_index >( _db );
}

void sps_plugin_impl::on_proposal_processing( const block_notification& note )
{
   
}

void sps_plugin_impl::plugin_shutdown()
{
   chain::util::disconnect_signal( _on_proposal_processing );
}

}; // detail

sps_plugin::sps_plugin() {}

sps_plugin::~sps_plugin() {}

void sps_plugin::set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
   )
{
}

void sps_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   try
   {
      ilog("Intializing sps plugin" );

      my = std::make_unique< detail::sps_plugin_impl >( *this );
      my->initialize();
   }
   FC_CAPTURE_AND_RETHROW()
}

void sps_plugin::plugin_startup() {}

void sps_plugin::plugin_shutdown()
{
   my->plugin_shutdown();
}

} } } // steem::plugins::sps
