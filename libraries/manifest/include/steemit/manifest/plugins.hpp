
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace steemit { namespace app {

class abstract_plugin;

} }

namespace steemit { namespace plugin {

void initialize_plugin_factories();
std::shared_ptr< steemit::app::abstract_plugin > create_plugin( const std::string& name );
std::vector< std::string > get_available_plugins();

} }
