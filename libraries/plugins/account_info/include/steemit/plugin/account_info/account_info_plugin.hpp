
#include <steemit/app/plugin.hpp>

namespace steemit { namespace plugin { namespace account_info {

class account_info_plugin : public steemit::app::plugin
{
   public:
      account_info_plugin();
      virtual ~account_info_plugin();

      virtual std::string plugin_name()const override;
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;
};

} } }
