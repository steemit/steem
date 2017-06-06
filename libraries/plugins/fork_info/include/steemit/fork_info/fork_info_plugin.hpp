#include <steemit/app/plugin.hpp>
#include <steemit/chain/steem_objects.hpp>


namespace steemit { namespace plugin { namespace fork_info {
namespace detail { struct fork_info_plugin_impl; }
using steemit::app::application;
class fork_info_plugin : public steemit::app::plugin
{
   public:
      fork_info_plugin( application* app );
      virtual ~fork_info_plugin();

      std::string plugin_name()const override { return "fork_info"; }

      virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
      virtual void plugin_startup() override;

   private:
      std::unique_ptr<steemit::plugin::fork_info::detail::fork_info_plugin_impl> _my;
};

} } } //steemit::plugin::fork_info_info

STEEMIT_DEFINE_PLUGIN( fork_info, steemit::plugin::fork_info::fork_info_plugin )
