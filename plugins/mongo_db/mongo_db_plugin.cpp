#include <golos/plugins/mongo_db/mongo_db_plugin.hpp>
#include <golos/plugins/json_rpc/plugin.hpp>
#include <golos/plugins/chain/plugin.hpp>
#include <golos/protocol/block.hpp>

#include <golos/plugins/mongo_db/mongo_db_writer.hpp>

namespace golos {
namespace plugins {
namespace mongo_db {

    using golos::protocol::signed_block;

    class mongo_db_plugin::mongo_db_plugin_impl {
    public:
        mongo_db_plugin_impl(mongo_db_plugin &plugin)
            : pimpl_(plugin),
              db_(appbase::app().get_plugin<golos::plugins::chain::plugin>().db()) {
        }

        bool initialize(const std::string& uri, const bool write_raw, const std::vector<std::string>& op) {
            return writer.initialize(uri, write_raw, op);
        }

        ~mongo_db_plugin_impl() = default;

        void on_block(const signed_block& block) {
            writer.on_block(block);
        }

        void on_operation(const golos::chain::operation_notification& note) {
            if (is_virtual_operation(note.op)) {
               writer.on_operation(note);
            }
        }

        golos::chain::database &database() const {
            return db_;
        }

        mongo_db_writer writer;
        mongo_db_plugin &pimpl_;

        golos::chain::database &db_;
    };

    // Plugin
    mongo_db_plugin::mongo_db_plugin() {
    }

    mongo_db_plugin::~mongo_db_plugin() {
    }

    void mongo_db_plugin::set_program_options(
        boost::program_options::options_description &cli,
        boost::program_options::options_description &cfg
    ) {
        cli.add_options()
            ("mongodb-uri",
             boost::program_options::value<string>()->default_value("mongodb://127.0.0.1:27017/Golos"),
             "Mongo DB connection string")
            ("mongodb-write-raw-blocks",
             boost::program_options::value<bool>()->default_value(true),
             "Write raw blocks into mongo or not")
            ("mongodb-write-operations",
             boost::program_options::value<std::vector<std::string>>()->multitoken()->zero_tokens()->composing(),
             "List of operations to write into mongo");
        cfg.add(cli);
    }

    void mongo_db_plugin::plugin_initialize(const boost::program_options::variables_map &options) {
        try {
            ilog("mongo_db plugin: plugin_initialize() begin");

            bool raw_blocks = true;
            if (options.count("mongodb-write-raw-blocks")) {
                raw_blocks = options.at("mongodb-write-raw-blocks").as<bool>();
            }
            std::vector<std::string> write_operations;
            if (options.count("mongodb-write-operations")) {
                write_operations = options.at("mongodb-write-operations").as<std::vector<std::string>>();
            }

            // First init mongo db
            if (options.count("mongodb-uri")) {
                std::string uri_str = options.at("mongodb-uri").as<std::string>();
                ilog("Connecting MongoDB to ${u}", ("u", uri_str));

                pimpl_ = std::make_unique<mongo_db_plugin_impl>(*this);

                if (!pimpl_->initialize(uri_str, raw_blocks, write_operations)) {
                    ilog("Cannot initialize MongoDB plugin. Plugin disabled.");
                    pimpl_.reset();
                    return;
                }
                // Set applied block listener
                auto &db = pimpl_->database();

                db.applied_block.connect([&](const signed_block &b) {
                    pimpl_->on_block(b);
                });

                db.post_apply_operation.connect([&](const operation_notification &o) {
                    pimpl_->on_operation(o);
                });

            } else {
                ilog("Mongo plugin configured, but no mongodb-uri specified. Plugin disabled.");
            }

            ilog("mongo_db plugin: plugin_initialize() end");
        } FC_CAPTURE_AND_RETHROW()
    } 

    void mongo_db_plugin::plugin_startup() {
        ilog("mongo_db plugin: plugin_startup() begin");

        ilog("mongo_db plugin: plugin_startup() end");
    }

    void mongo_db_plugin::plugin_shutdown() {
        ilog("mongo_db plugin: plugin_shutdown() begin");

        ilog("mongo_db plugin: plugin_shutdown() end");
    }

 }}} // namespace golos::plugins::mongo_db
