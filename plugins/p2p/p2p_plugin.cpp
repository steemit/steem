#include <golos/plugins/p2p/p2p_plugin.hpp>

#include <golos/network/node.hpp>
#include <golos/network/exceptions.hpp>

#include <golos/chain/database_exceptions.hpp>

#include <fc/network/resolve.hpp>

#include <boost/range/algorithm/reverse.hpp>
#include <boost/range/adaptor/reversed.hpp>

using std::string;
using std::vector;

namespace golos {
    namespace plugins {
        namespace p2p {

            using appbase::app;

            using golos::network::item_hash_t;
            using golos::network::item_id;
            using golos::network::message;
            using golos::network::block_message;
            using golos::network::trx_message;

            using golos::protocol::block_header;
            using golos::protocol::signed_block_header;
            using golos::protocol::signed_block;
            using golos::protocol::block_id_type;
            using golos::chain::database;
            using golos::chain::chain_id_type;

            namespace detail {

                class p2p_plugin_impl : public golos::network::node_delegate {
                public:

                    p2p_plugin_impl(chain::plugin &c) : chain(c) {
                    }

                    virtual ~p2p_plugin_impl() {
                    }

                    bool is_included_block(const block_id_type &block_id);

                    chain_id_type get_chain_id() const;

                    // node_delegate interface
                    virtual bool has_item(const item_id &) override;

                    virtual bool handle_block(const block_message &, bool, std::vector<fc::uint160_t> &) override;

                    virtual void handle_transaction(const trx_message &) override;

                    virtual void handle_message(const message &) override;

                    virtual std::vector<item_hash_t> get_block_ids(const std::vector<item_hash_t> &, uint32_t &,
                                                                   uint32_t) override;

                    virtual message get_item(const item_id &) override;

                    virtual std::vector<item_hash_t> get_blockchain_synopsis(const item_hash_t &, uint32_t) override;

                    virtual void sync_status(uint32_t, uint32_t) override;

                    virtual void connection_count_changed(uint32_t) override;

                    virtual uint32_t get_block_number(const item_hash_t &) override;

                    virtual fc::time_point_sec get_block_time(const item_hash_t &) override;

                    virtual fc::time_point_sec get_blockchain_now() override;

                    virtual item_hash_t get_head_block_id() const override;

                    virtual uint32_t estimate_last_known_fork_from_git_revision_timestamp(uint32_t) const override;

                    virtual void error_encountered(const std::string &message, const fc::oexception &error) override;

                    //virtual uint8_t get_current_block_interval_in_seconds() const override {
                    //    return STEEMIT_BLOCK_INTERVAL;
                    //}


                    fc::optional<fc::ip::endpoint> endpoint;
                    vector<fc::ip::endpoint> seeds;
                    string user_agent;
                    uint32_t max_connections = 0;
                    bool force_validate = false;
                    bool block_producer = false;

                    std::unique_ptr<golos::network::node> node;

                    chain::plugin &chain;

                    fc::thread p2p_thread;
                };

                ////////////////////////////// Begin node_delegate Implementation //////////////////////////////
                bool p2p_plugin_impl::has_item(const item_id &id) {
                    return chain.db().with_weak_read_lock([&]() {
                        try {
                            if (id.item_type == network::block_message_type) {
                                return chain.db().is_known_block(id.item_hash);
                            } else {
                                return chain.db().is_known_transaction(id.item_hash);
                            }
                        } FC_CAPTURE_LOG_AND_RETHROW((id))
                    });
                }

                bool p2p_plugin_impl::handle_block(const block_message &blk_msg, bool sync_mode, std::vector<fc::uint160_t> &) {
                    try {
                        uint32_t head_block_num;
                        chain.db().with_weak_read_lock([&]() {
                            head_block_num = chain.db().head_block_num();
                        });
                        if (sync_mode)
                            fc_ilog(fc::logger::get("sync"),
                                    "chain pushing sync block #${block_num} ${block_hash}, head is ${head}",
                                    ("block_num", blk_msg.block.block_num())("block_hash", blk_msg.block_id)("head",
                                                                                                             head_block_num));
                        else
                            fc_ilog(fc::logger::get("sync"),
                                    "chain pushing block #${block_num} ${block_hash}, head is ${head}",
                                    ("block_num", blk_msg.block.block_num())("block_hash", blk_msg.block_id)("head",
                                                                                                             head_block_num));

                        try {
                            // TODO: in the case where this block is valid but on a fork that's too old for us to switch to,
                            // you can help the network code out by throwing a block_older_than_undo_history exception.
                            // when the network code sees that, it will stop trying to push blocks from that chain, but
                            // leave that peer connected so that they can get sync blocks from us
                            bool result = chain.accept_block(blk_msg.block, sync_mode, (block_producer | force_validate)
                                                                                       ? database::skip_nothing
                                                                                       : database::skip_transaction_signatures);

                            if (!sync_mode) {
                                fc::microseconds latency = fc::time_point::now() - blk_msg.block.timestamp;
                                ilog("Got ${t} transactions on block ${b} by ${w} -- latency: ${l} ms",
                                     ("t", blk_msg.block.transactions.size())("b", blk_msg.block.block_num())("w", blk_msg.block.witness)("l", latency.count() / 1000));
                            }

                            return result;
                        } catch (const golos::network::unlinkable_block_exception &e) {
                            // translate to a golos::network exception
                            fc_elog(fc::logger::get("sync"), "Error when pushing block, current head block is ${head}:\n${e}", ("e", e.to_detail_string())("head", head_block_num));
                            elog("Error when pushing block:\n${e}", ("e", e.to_detail_string()));
                            FC_THROW_EXCEPTION(golos::network::unlinkable_block_exception, "Error when pushing block:\n${e}", ("e", e.to_detail_string()));
                        } catch (const fc::exception &e) {
                            fc_elog(fc::logger::get("sync"), "Error when pushing block, current head block is ${head}:\n${e}", ("e", e.to_detail_string())("head", head_block_num));
                            elog("Error when pushing block:\n${e}", ("e", e.to_detail_string()));
                            throw;
                        }

                        return false;
                    } FC_CAPTURE_AND_RETHROW((blk_msg)(sync_mode))
                }

                void p2p_plugin_impl::handle_transaction(const trx_message &trx_msg) {
                    try {
                        chain.accept_transaction(trx_msg.trx);
                    } FC_CAPTURE_AND_RETHROW((trx_msg))
                }

                void p2p_plugin_impl::handle_message(const message &message_to_process) {
                    // not a transaction, not a block
                    FC_THROW("Invalid Message Type");
                }

                std::vector<item_hash_t> p2p_plugin_impl::get_block_ids(
                        const std::vector<item_hash_t> &blockchain_synopsis, uint32_t &remaining_item_count,
                        uint32_t limit) {
                    try {
                        return chain.db().with_weak_read_lock([&]() {
                            vector<block_id_type> result;
                            remaining_item_count = 0;
                            if (chain.db().head_block_num() == 0) {
                                return result;
                            }

                            result.reserve(limit);
                            block_id_type last_known_block_id;

                            if (blockchain_synopsis.empty() ||
                                (blockchain_synopsis.size() == 1 && blockchain_synopsis[0] == block_id_type())) {
                                // peer has sent us an empty synopsis meaning they have no blocks.
                                // A bug in old versions would cause them to send a synopsis containing block 000000000
                                // when they had an empty blockchain, so pretend they sent the right thing here.
                                // do nothing, leave last_known_block_id set to zero
                            } else {
                                bool found_a_block_in_synopsis = false;

                                for (const item_hash_t &block_id_in_synopsis : boost::adaptors::reverse(
                                        blockchain_synopsis)) {
                                    if (block_id_in_synopsis == block_id_type() ||
                                        (chain.db().is_known_block(block_id_in_synopsis) &&
                                         is_included_block(block_id_in_synopsis))) {
                                        last_known_block_id = block_id_in_synopsis;
                                        found_a_block_in_synopsis = true;
                                        break;
                                    }
                                }

                                if (!found_a_block_in_synopsis)
                                    FC_THROW_EXCEPTION(golos::network::peer_is_on_an_unreachable_fork, "Unable to provide a list of blocks starting at any of the blocks in peer's synopsis");
                            }

                            for (uint32_t num = block_header::num_from_id(last_known_block_id);
                                 num <= chain.db().head_block_num() && result.size() < limit; ++num) {
                                if (num > 0) {
                                    result.push_back(chain.db().get_block_id_for_num(num));
                                }
                            }

                            if (!result.empty() &&
                                block_header::num_from_id(result.back()) < chain.db().head_block_num()) {
                                remaining_item_count =
                                        chain.db().head_block_num() - block_header::num_from_id(result.back());
                            }

                            return result;
                        });
                    } FC_CAPTURE_AND_RETHROW((blockchain_synopsis)(remaining_item_count)(limit))
                }

                message p2p_plugin_impl::get_item(const item_id &id) {
                    try {
                        if (id.item_type == network::block_message_type) {
                            return chain.db().with_weak_read_lock([&]() {
                                auto opt_block = chain.db().fetch_block_by_id(id.item_hash);
                                if (!opt_block)
                                    elog("Couldn't find block ${id} -- corresponding ID in our chain is ${id2}",
                                         ("id", id.item_hash)("id2", chain.db().get_block_id_for_num(
                                                 block_header::num_from_id(id.item_hash))));
                                FC_ASSERT(opt_block.valid());
                                // ilog("Serving up block #${num}", ("num", opt_block->block_num()));
                                return block_message(std::move(*opt_block));
                            });
                        }
                        return chain.db().with_weak_read_lock([&]() {
                            return trx_message(chain.db().get_recent_transaction(id.item_hash));
                        });
                    } FC_CAPTURE_AND_RETHROW((id))
                }

                chain_id_type p2p_plugin_impl::get_chain_id() const {
                    return STEEMIT_CHAIN_ID;
                }

                std::vector<item_hash_t> p2p_plugin_impl::get_blockchain_synopsis(const item_hash_t &reference_point,
                                                                                  uint32_t number_of_blocks_after_reference_point) {
                    try {
                        std::vector<item_hash_t> synopsis;
                        chain.db().with_weak_read_lock([&]() {
                            synopsis.reserve(30);
                            uint32_t high_block_num;
                            uint32_t non_fork_high_block_num;
                            uint32_t low_block_num = chain.db().last_non_undoable_block_num();
                            std::vector<block_id_type> fork_history;

                            if (reference_point != item_hash_t()) {
                                // the node is asking for a summary of the block chain up to a specified
                                // block, which may or may not be on a fork
                                // for now, assume it's not on a fork
                                if (is_included_block(reference_point)) {
                                    // reference_point is a block we know about and is on the main chain
                                    uint32_t reference_point_block_num = block_header::num_from_id(reference_point);
                                    assert(reference_point_block_num > 0);
                                    high_block_num = reference_point_block_num;
                                    non_fork_high_block_num = high_block_num;

                                    if (reference_point_block_num < low_block_num) {
                                        // we're on the same fork (at least as far as reference_point) but we've passed
                                        // reference point and could no longer undo that far if we diverged after that
                                        // block.  This should probably only happen due to a race condition where
                                        // the network thread calls this function, and then immediately pushes a bunch of blocks,
                                        // then the main thread finally processes this function.
                                        // with the current framework, there's not much we can do to tell the network
                                        // thread what our current head block is, so we'll just pretend that
                                        // our head is actually the reference point.
                                        // this *may* enable us to fetch blocks that we're unable to push, but that should
                                        // be a rare case (and correctly handled)
                                        low_block_num = reference_point_block_num;
                                    }
                                } else {
                                    // block is a block we know about, but it is on a fork
                                    try {
                                        fork_history = chain.db().get_block_ids_on_fork(reference_point);
                                        // returns a vector where the last element is the common ancestor with the preferred chain,
                                        // and the first element is the reference point you passed in
                                        assert(fork_history.size() >= 2);

                                        if (fork_history.front() != reference_point) {
                                            edump((fork_history)(reference_point));
                                            assert(fork_history.front() == reference_point);
                                        }
                                        block_id_type last_non_fork_block = fork_history.back();
                                        fork_history.pop_back();  // remove the common ancestor
                                        boost::reverse(fork_history);

                                        if (last_non_fork_block ==
                                            block_id_type()) { // if the fork goes all the way back to genesis (does golos's fork db allow this?)
                                            non_fork_high_block_num = 0;
                                        } else {
                                            non_fork_high_block_num = block_header::num_from_id(last_non_fork_block);
                                        }

                                        high_block_num = non_fork_high_block_num + fork_history.size();
                                        assert(high_block_num == block_header::num_from_id(fork_history.back()));
                                    } catch (const fc::exception &e) {
                                        // unable to get fork history for some reason.  maybe not linked?
                                        // we can't return a synopsis of its chain
                                        elog("Unable to construct a blockchain synopsis for reference hash ${hash}: ${exception}",
                                             ("hash", reference_point)("exception", e));
                                        throw;
                                    }
                                    if (non_fork_high_block_num < low_block_num) {
                                        wlog("Unable to generate a usable synopsis because the peer we're generating it for forked too long ago "
                                                     "(our chains diverge after block #${non_fork_high_block_num} but only undoable to block #${low_block_num})",
                                             ("low_block_num", low_block_num)("non_fork_high_block_num",
                                                                              non_fork_high_block_num));
                                        FC_THROW_EXCEPTION(golos::network::block_older_than_undo_history, "Peer is are on a fork I'm unable to switch to");
                                    }
                                }
                            } else {
                                // no reference point specified, summarize the whole block chain
                                high_block_num = chain.db().head_block_num();
                                non_fork_high_block_num = high_block_num;
                                if (high_block_num == 0) {
                                    return;
                                } // we have no blocks
                            }

                            if (low_block_num == 0) {
                                low_block_num = 1;
                            }

                            // at this point:
                            // low_block_num is the block before the first block we can undo,
                            // non_fork_high_block_num is the block before the fork (if the peer is on a fork, or otherwise it is the same as high_block_num)
                            // high_block_num is the block number of the reference block, or the end of the chain if no reference provided

                            // true_high_block_num is the ending block number after the network code appends any item ids it
                            // knows about that we don't
                            uint32_t true_high_block_num = high_block_num + number_of_blocks_after_reference_point;
                            do {
                                // for each block in the synopsis, figure out where to pull the block id from.
                                // if it's <= non_fork_high_block_num, we grab it from the main blockchain;
                                // if it's not, we pull it from the fork history
                                if (low_block_num <= non_fork_high_block_num) {
                                    synopsis.push_back(chain.db().get_block_id_for_num(low_block_num));
                                } else {
                                    synopsis.push_back(fork_history[low_block_num - non_fork_high_block_num - 1]);
                                }
                                low_block_num += (true_high_block_num - low_block_num + 2) / 2;
                            } while (low_block_num <= high_block_num);

                            //idump((synopsis));
                            return;
                        });

                        return synopsis;
                    } FC_LOG_AND_RETHROW()
                }

                void p2p_plugin_impl::sync_status(uint32_t item_type, uint32_t item_count) {
                    // any status reports to GUI go here
                }

                void p2p_plugin_impl::connection_count_changed(uint32_t c) {
                    // any status reports to GUI go here
                }

                uint32_t p2p_plugin_impl::get_block_number(const item_hash_t &block_id) {
                    try {
                        return block_header::num_from_id(block_id);
                    } FC_CAPTURE_AND_RETHROW((block_id))
                }

                fc::time_point_sec p2p_plugin_impl::get_block_time(const item_hash_t &block_id) {
                    try {
                        return chain.db().with_weak_read_lock([&]() {
                            auto opt_block = chain.db().fetch_block_by_id(block_id);
                            if (opt_block.valid()) {
                                return opt_block->timestamp;
                            }
                            return fc::time_point_sec::min();
                        });
                    } FC_CAPTURE_AND_RETHROW((block_id))
                }

                item_hash_t p2p_plugin_impl::get_head_block_id() const {
                    try {
                        return chain.db().with_weak_read_lock([&]() {
                            return chain.db().head_block_id();
                        });
                    } FC_CAPTURE_AND_RETHROW()
                }

                uint32_t p2p_plugin_impl::estimate_last_known_fork_from_git_revision_timestamp(uint32_t) const {
                    return 0; // there are no forks in golos
                }

                void p2p_plugin_impl::error_encountered(const string &message, const fc::oexception &error) {
                    // notify GUI or something cool
                }

                fc::time_point_sec p2p_plugin_impl::get_blockchain_now() {
                    try {
                        return fc::time_point::now();
                    } FC_CAPTURE_AND_RETHROW()
                }

                bool p2p_plugin_impl::is_included_block(const block_id_type &block_id) {
                    try {
                        return chain.db().with_weak_read_lock([&]() {
                            uint32_t block_num = block_header::num_from_id(block_id);
                            block_id_type block_id_in_preferred_chain = chain.db().get_block_id_for_num(block_num);
                            return block_id == block_id_in_preferred_chain;
                        });
                    } FC_CAPTURE_AND_RETHROW()
                }

                ////////////////////////////// End node_delegate Implementation //////////////////////////////

            } // detail

            p2p_plugin::p2p_plugin() {
            }

            p2p_plugin::~p2p_plugin() {
            }

            void p2p_plugin::set_program_options(boost::program_options::options_description &cli, boost::program_options::options_description &cfg) {
                cfg.add_options()
                    ("p2p-endpoint", boost::program_options::value<string>()->implicit_value("127.0.0.1:9876"),
                        "The local IP address and port to listen for incoming connections.")
                    ("p2p-max-connections", boost::program_options::value<uint32_t>(),
                        "Maxmimum number of incoming connections on P2P endpoint.")
                    ("seed-node", boost::program_options::value<vector<string>>()->composing(),
                        "The IP address and port of a remote peer to sync with. Deprecated in favor of p2p-seed-node.")
                    ("p2p-seed-node", boost::program_options::value<vector<string>>()->composing(),
                        "The IP address and port of a remote peer to sync with.");
                cli.add_options()
                    ("force-validate", boost::program_options::bool_switch()->default_value(false),
                        "Force validation of all transactions. Deprecated in favor of p2p-force-validate")
                    ("p2p-force-validate", boost::program_options::bool_switch()->default_value(false),
                        "Force validation of all transactions.");
            }

            void p2p_plugin::plugin_initialize(const boost::program_options::variables_map &options) {
                my.reset(new detail::p2p_plugin_impl(appbase::app().get_plugin<chain::plugin>()));

                if (options.count("p2p-endpoint")) {
                    my->endpoint = fc::ip::endpoint::from_string(options.at("p2p-endpoint").as<string>());
                }

                my->user_agent = "Steem Reference Implementation";

                if (options.count("p2p-max-connections")) {
                    my->max_connections = options.at("p2p-max-connections").as<uint32_t>();
                }

                if (options.count("seed-node") || options.count("p2p-seed-node")) {
                    vector<string> seeds;
                    if (options.count("seed-node")) {
                        wlog("Option seed-node is deprecated in favor of p2p-seed-node");
                        auto s = options.at("seed-node").as<vector<string>>();
                        seeds.insert(seeds.end(), s.begin(), s.end());
                    }

                    if (options.count("p2p-seed-node")) {
                        auto s = options.at("p2p-seed-node").as<vector<string>>();
                        seeds.insert(seeds.end(), s.begin(), s.end());
                    }

                    for (const string &endpoint_string : seeds) {
                        try {
                            auto eps = appbase::app().resolve_string_to_ip_endpoints(endpoint_string);
                            for (auto& ep: eps) {
                                my->seeds.push_back(fc::ip::endpoint(ep.address().to_string(), ep.port()));
                            }
                        } catch (const fc::exception &e) {
                            wlog("caught exception ${e} while adding seed node ${endpoint}",
                                 ("e", e.to_detail_string())("endpoint", endpoint_string));
                        }
                    }
                }

                my->force_validate = options.at("p2p-force-validate").as<bool>();

                if (!my->force_validate && options.at("force-validate").as<bool>()) {
                    wlog("Option force-validate is deprecated in favor of p2p-force-validate");
                    my->force_validate = true;
                }
            }

            void p2p_plugin::plugin_startup() {
                my->p2p_thread.async([this] {
                    my->node.reset(new golos::network::node(my->user_agent));
                    my->node->load_configuration(app().data_dir() / "p2p");
                    my->node->set_node_delegate(&(*my));

                    if (my->endpoint) {
                        ilog("Configuring P2P to listen at ${ep}", ("ep", my->endpoint));
                        my->node->listen_on_endpoint(*my->endpoint, true);
                    }

                    for (const auto &seed : my->seeds) {
                        ilog("P2P adding seed node ${s}", ("s", seed));
                        my->node->add_node(seed);
                        my->node->connect_to_endpoint(seed);
                    }

                    if (my->max_connections) {
                        ilog("Setting p2p max connections to ${n}", ("n", my->max_connections));
                        fc::variant_object node_param = fc::variant_object("maximum_number_of_connections",
                                                                           fc::variant(my->max_connections));
                        my->node->set_advanced_node_parameters(node_param);
                    }

                    my->node->listen_to_p2p_network();
                    my->node->connect_to_p2p_network();
                    block_id_type block_id;
                    my->chain.db().with_weak_read_lock([&]() {
                        block_id = my->chain.db().head_block_id();
                    });
                    my->node->sync_from(item_id(golos::network::block_message_type, block_id),
                                        std::vector<uint32_t>());
                    ilog("P2P node listening at ${ep}", ("ep", my->node->get_actual_listening_endpoint()));
                }).wait();
                ilog("P2P Plugin started");
            }

            void p2p_plugin::plugin_shutdown() {
                ilog("Shutting down P2P Plugin");
                my->node->close();
                my->p2p_thread.quit();
                my->node.reset();
            }

            void p2p_plugin::broadcast_block(const protocol::signed_block &block) {
                ulog("Broadcasting block #${n}", ("n", block.block_num()));
                my->node->broadcast(block_message(block));
            }

            void p2p_plugin::broadcast_transaction(const protocol::signed_transaction &tx) {
                ulog("Broadcasting tx #${n}", ("id", tx.id()));
                my->node->broadcast(trx_message(tx));
            }

            void p2p_plugin::set_block_production(bool producing_blocks) {
                my->block_producer = producing_blocks;
            }

        }
    }
} // namespace steem::plugins::p2p
