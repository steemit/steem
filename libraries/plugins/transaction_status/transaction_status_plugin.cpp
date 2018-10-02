#include <steem/plugins/transaction_status/transaction_status_plugin.hpp>
#include <steem/plugins/transaction_status/transaction_status_objects.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/index.hpp>

#include <fc/io/json.hpp>

namespace steem { namespace plugins { namespace transaction_status {

namespace detail {

class transaction_status_impl
{
public:
   transaction_status_impl() :
   _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ) {}
   virtual ~transaction_status_impl() {}

   /**
     * This method is called as a callback after a block is applied
     * and will process/index all operations that were applied in the block.
     */
    //void on_post_apply_operation( const operation_notification& note );

   chain::database&     _db;
   boost::signals2::connection   _post_apply_operation_conn;
};

} // detail

transaction_status_plugin::transaction_status_plugin() {}
transaction_status_plugin::~transaction_status_plugin() {}

void transaction_status_plugin::set_program_options( boost::program_options::options_description& cli, boost::program_options::options_description& cfg ) {}

void transaction_status_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   try
   {
      ilog( "transaction_status: plugin_initialize() begin" );
      my = std::make_unique< detail::transaction_status_impl >();

      //my->_post_apply_operation_conn = my->_db.add_post_apply_operation_handler( [&]( const operation_notification& note ){ my->on_post_apply_operation( note ); }, *this, 0 );
      chain::add_plugin_index< transaction_status_index >( my->_db );
   } FC_CAPTURE_AND_RETHROW()
}

void transaction_status_plugin::plugin_startup() {}

void transaction_status_plugin::plugin_shutdown()
{
   //chain::util::disconnect_signal( my->_post_apply_operation_conn );
}

} } } // steem::plugins::transaction_status
