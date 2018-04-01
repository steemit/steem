#include <golos/chain/database_exceptions.hpp>
#include <golos/chain/database.hpp>
#include <golos/plugins/chain/plugin.hpp>

#include <fc/io/json.hpp>
#include <fc/string.hpp>

#include <iostream>
#include <golos/protocol/protocol.hpp>
#include <golos/protocol/types.hpp>

namespace golos {
namespace plugins {
namespace chain {

    using fc::flat_map;
    using protocol::block_id_type;

    class plugin::plugin_impl {
    public:

        uint64_t shared_memory_size = 0;
        boost::filesystem::path shared_memory_dir;
        bool replay = false;
        bool resync = false;
        bool readonly = false;
        bool check_locks = false;
        bool validate_invariants = false;
        uint32_t flush_interval = 0;
        flat_map<uint32_t, protocol::block_id_type> loaded_checkpoints;

        uint32_t allow_future_time = 5;

        uint64_t read_wait_micro;
        uint32_t max_read_wait_retries;

        uint64_t write_wait_micro;
        uint32_t max_write_wait_retries;

        golos::chain::database db;

        plugin_impl() {
            // get default settings
            read_wait_micro = db.read_wait_micro();
            max_read_wait_retries = db.max_read_wait_retries();

            write_wait_micro = db.write_wait_micro();
            max_write_wait_retries = db.max_write_wait_retries();
        }

        // HELPERS
        golos::chain::database &database() {
            return db;
        }

        constexpr const static char *plugin_name = "chain_api";
        static const std::string &name() {
            static std::string name = plugin_name;
            return name;
        }

        void check_time_in_block(const protocol::signed_block &block);
        bool accept_block(const protocol::signed_block &block, bool currently_syncing, uint32_t skip);
        void accept_transaction(const protocol::signed_transaction &trx);
    };

    void plugin::plugin_impl::check_time_in_block(const protocol::signed_block &block) {
        time_point_sec now = fc::time_point::now();

        uint64_t max_accept_time = now.sec_since_epoch();
        max_accept_time += allow_future_time;
        FC_ASSERT(block.timestamp.sec_since_epoch() <= max_accept_time);
    }

    bool plugin::plugin_impl::accept_block(const protocol::signed_block &block, bool currently_syncing, uint32_t skip) {
        if (currently_syncing && block.block_num() % 10000 == 0) {
            ilog("Syncing Blockchain --- Got block: #${n} time: ${t} producer: ${p}",
                 ("t", block.timestamp)("n", block.block_num())("p", block.witness));
        }

        check_time_in_block(block);

        return db.push_block(block, skip);
    }

    void plugin::plugin_impl::accept_transaction(const protocol::signed_transaction &trx) {
        db.push_transaction(trx);
    }

    plugin::plugin() {
    }

    plugin::~plugin() {
    }

    golos::chain::database &plugin::db() {
        return my->db;
    }

    const golos::chain::database &plugin::db() const {
        return my->db;
    }

    void plugin::set_program_options(boost::program_options::options_description &cli,
                                     boost::program_options::options_description &cfg) {
        cfg.add_options()
            (
                "shared-file-dir",
                boost::program_options::value<boost::filesystem::path>()->default_value("blockchain"),
                "the location of the chain shared memory files (absolute path or relative to application data dir)"
            )
            (
                "shared-file-size",
                boost::program_options::value<std::string>()->default_value("64G"),
                "Size of the shared memory file. Default: 54G"
            )
            (
                "checkpoint,c",
                boost::program_options::value<std::vector<std::string>>()->composing(),
                "Pairs of [BLOCK_NUM,BLOCK_ID] that should be enforced as checkpoints."
            )
            (
                "flush-state-interval",
                boost::program_options::value<uint32_t>(),
                "flush shared memory changes to disk every N blocks"
            )
            (
                "read-wait-micro",
                boost::program_options::value<uint64_t>(),
                "maximum microseconds for trying to get read lock"
            )
            (
                "max-read-wait-retries",
                boost::program_options::value<uint32_t>(),
                "maximum number of retries to get read lock"
            )
            (
                "write-wait-micro",
                boost::program_options::value<uint64_t>(),
                "maximum microseconds for trying to get write lock"
            )
            (
                "max-write-wait-retries",
                boost::program_options::value<uint32_t>(),
                "maximum number of retries to get write lock"
            );
        cli.add_options()
            (
                "replay-blockchain",
                boost::program_options::bool_switch()->default_value(false),
                "clear chain database and replay all blocks"
            )
            (
                "resync-blockchain",
                boost::program_options::bool_switch()->default_value(false),
                "clear chain database and block log"
            )
            (
                "check-locks",
                boost::program_options::bool_switch()->default_value(false),
                "Check correctness of chainbase locking"
            )
            (
                "validate-database-invariants",
                boost::program_options::bool_switch()->default_value(false),
                "Validate all supply invariants check out"
            );
    }

    void plugin::plugin_initialize(const boost::program_options::variables_map &options) {

        my.reset(new plugin_impl());
        my->shared_memory_dir = appbase::app().data_dir() / "blockchain";

        if (options.count("shared-file-dir")) {
            auto sfd = options.at("shared-file-dir").as<boost::filesystem::path>();
            if (sfd.is_relative()) {
                my->shared_memory_dir = appbase::app().data_dir() / sfd;
            } else {
                my->shared_memory_dir = sfd;
            }
        }

        if (options.count("read-wait-micro")) {
            my->read_wait_micro = options.at("read-wait-micro").as<uint64_t>();
        }

        if (options.count("max-read-wait-retries")) {
            my->max_read_wait_retries = options.at("max-read-wait-retries").as<uint32_t>();
        }

        if (options.count("write-wait-micro")) {
            my->write_wait_micro = options.at("write-wait-micro").as<uint64_t>();
        }

        if (options.count("max-write-wait-retries")) {
            my->max_write_wait_retries = options.at("max-write-wait-retries").as<uint32_t>();
        }

        my->shared_memory_size = fc::parse_size(options.at("shared-file-size").as<std::string>());

        my->replay = options.at("replay-blockchain").as<bool>();
        my->resync = options.at("resync-blockchain").as<bool>();
        my->check_locks = options.at("check-locks").as<bool>();
        my->validate_invariants = options.at("validate-database-invariants").as<bool>();
        if (options.count("flush-state-interval")) {
            my->flush_interval = options.at("flush-state-interval").as<uint32_t>();
        } else {
            my->flush_interval = 10000;
        }

        if (options.count("checkpoint")) {
            auto cps = options.at("checkpoint").as<std::vector<std::string>>();
            my->loaded_checkpoints.reserve(cps.size());
            for (const auto &cp : cps) {
                auto item = fc::json::from_string(cp).as<std::pair<uint32_t, protocol::block_id_type>>();
                my->loaded_checkpoints[item.first] = item.second;
            }
        }
    }

    void plugin::plugin_startup() {
        ilog("Starting chain with shared_file_size: ${n} bytes", ("n", my->shared_memory_size));

        if (my->resync) {
            wlog("resync requested: deleting block log and shared memory");
            my->db.wipe(appbase::app().data_dir() / "blockchain", my->shared_memory_dir, true);
        }

        my->db.set_flush_interval(my->flush_interval);
        my->db.add_checkpoints(my->loaded_checkpoints);
        my->db.set_require_locking(my->check_locks);

        my->db.read_wait_micro(my->read_wait_micro);
        my->db.max_read_wait_retries(my->max_read_wait_retries);
        my->db.write_wait_micro(my->write_wait_micro);
        my->db.max_write_wait_retries(my->max_write_wait_retries);

        if (my->replay) {
            ilog("Replaying blockchain on user request.");
            my->db.reindex(appbase::app().data_dir() / "blockchain", my->shared_memory_dir, my->shared_memory_size);
        } else {
            try {
                ilog("Opening shared memory from ${path}", ("path", my->shared_memory_dir.generic_string()));
                my->db.open(appbase::app().data_dir() / "blockchain", my->shared_memory_dir, STEEMIT_INIT_SUPPLY, my->shared_memory_size, chainbase::database::read_write/*, my->validate_invariants*/ );
            } catch (const fc::exception &e) {
                wlog("Error opening database, attempting to replay blockchain. Error: ${e}", ("e", e));

                try {
                    my->db.reindex(appbase::app().data_dir() / "blockchain", my->shared_memory_dir, my->shared_memory_size);
                } catch (golos::chain::block_log &) {
                    wlog("Error opening block log. Having to resync from network...");
                    my->db.open(appbase::app().data_dir() / "blockchain", my->shared_memory_dir, STEEMIT_INIT_SUPPLY, my->shared_memory_size, chainbase::database::read_write/*, my->validate_invariants*/ );
                }
            }
        }

        ilog("Started on blockchain with ${n} blocks", ("n", my->db.head_block_num()));
        on_sync();
    }

    void plugin::plugin_shutdown() {
        ilog("closing chain database");
        my->db.close();
        ilog("database closed successfully");
    }

    bool plugin::accept_block(const protocol::signed_block &block, bool currently_syncing, uint32_t skip) {
        return my->accept_block(block, currently_syncing, skip);
    }

    void plugin::accept_transaction(const protocol::signed_transaction &trx) {
        my->accept_transaction(trx);
    }

    bool plugin::block_is_on_preferred_chain(const protocol::block_id_type &block_id) {
        // If it's not known, it's not preferred.
        if (!db().is_known_block(block_id)) {
            return false;
        }

        // Extract the block number from block_id, and fetch that block number's ID from the database.
        // If the database's block ID matches block_id, then block_id is on the preferred chain. Otherwise, it's on a fork.
        return db().get_block_id_for_num(protocol::block_header::num_from_id(block_id)) == block_id;
    }

    void plugin::check_time_in_block(const protocol::signed_block &block) {
        my->check_time_in_block(block);
    }

}
}
} // namespace steem::plugis::chain::chain_apis
