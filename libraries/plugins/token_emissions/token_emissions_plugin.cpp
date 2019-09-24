#include <steem/chain/steem_fwd.hpp>

#include <steem/plugins/token_emissions/token_emissions_plugin.hpp>
#include <steem/plugins/token_emissions/token_emissions_objects.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/index.hpp>
#include <steem/protocol/config.hpp>

#include <fc/io/json.hpp>

namespace steem { namespace plugins { namespace token_emissions {

namespace detail {

class token_emissions_impl
{
public:
   token_emissions_impl() : _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ) {}
   virtual ~token_emissions_impl() {}

   chain::database&              _db;
};

} // detail

token_emissions_plugin::token_emissions_plugin() {}
token_emissions_plugin::~token_emissions_plugin() {}

void token_emissions_plugin::set_program_options( boost::program_options::options_description& cli, boost::program_options::options_description& cfg )
{
}

void token_emissions_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   STEEM_ADD_PLUGIN_INDEX(my->_db, token_emissions_index);
}

void token_emissions_plugin::plugin_startup()
{

}

void token_emissions_plugin::plugin_shutdown()
{
}

} } } // steem::plugins::token_emissions

