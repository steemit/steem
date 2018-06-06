#pragma once

#include <golos/chain/database.hpp>
#include <appbase/plugin.hpp>
#include <golos/plugins/json_rpc/utility.hpp>
#include <golos/plugins/json_rpc/plugin.hpp>
#include <golos/plugins/chain/plugin.hpp>


namespace golos {
namespace plugins {
namespace mongo_db {

    class mongo_db_plugin final : public appbase::plugin<mongo_db_plugin> {
    public:

        APPBASE_PLUGIN_REQUIRES(
            (chain::plugin)
        )

        mongo_db_plugin();

        virtual ~mongo_db_plugin();

        void set_program_options(
            boost::program_options::options_description& cli,
            boost::program_options::options_description& cfg
        ) override;

        void plugin_initialize(const boost::program_options::variables_map& options) override;

        void plugin_startup() override;

        void plugin_shutdown() override;

        constexpr const static char *plugin_name = "mongo_db";

        static const std::string& name() {
            static std::string name = plugin_name;
            return name;
        }

    private:
        class mongo_db_plugin_impl;

        std::unique_ptr<mongo_db_plugin_impl> pimpl_;
    };

}}} // namespace golos::plugins::mongo_db

