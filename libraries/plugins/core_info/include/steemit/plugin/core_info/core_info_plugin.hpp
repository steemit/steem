
#include <steemit/app/plugin.hpp>

namespace steemit { namespace plugin { namespace core_info {

class core_info_plugin : public steemit::app::plugin
{
   public:
      core_info_plugin();
      virtual ~core_info_plugin();

      virtual std::string plugin_name()const override;
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;
};

} } }
