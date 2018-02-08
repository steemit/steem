#include <golos/plugins/network_broadcast_api/network_broadcast_api_plugin.hpp>
#include <golos/plugins/json_rpc/utility.hpp>

#include <appbase/application.hpp>

#include <boost/thread/future.hpp>
#include <boost/thread/lock_guard.hpp>

namespace golos {
    namespace plugins {
        namespace network_broadcast_api {


            using std::vector;
            using fc::variant;
            using fc::optional;
            using golos::protocol::signed_transaction;


            struct broadcast_transaction_t {
                signed_transaction trx;
                int32_t max_block_age = -1;
            };

            struct broadcast_block_t {
                signed_block block;
            };

            using confirmation_callback = std::function<void(const broadcast_transaction_synchronous_return &)>;

            struct network_broadcast_api_plugin::impl final {
            public:
                impl() : _p2p(appbase::app().get_plugin<p2p::p2p_plugin>()),
                        _chain(appbase::app().get_plugin<chain::plugin>()) {
                }

                p2p::p2p_plugin &_p2p;
                chain::plugin &_chain;
                map<transaction_id_type, confirmation_callback> _callbacks;
                map<time_point_sec, vector<transaction_id_type> > _callback_expirations;
                boost::mutex _mtx;
            };

            network_broadcast_api_plugin::network_broadcast_api_plugin() {

            }

            network_broadcast_api_plugin::~network_broadcast_api_plugin() {
            }

            DEFINE_API(network_broadcast_api_plugin, broadcast_transaction) {
                //auto tmp = args.args->at(0).as<broadcast_transaction_t>();
                auto n_args = args.args->size();
                FC_ASSERT(args.args->size() >= 1, "Expected at least 1 argument, got 0");
                broadcast_transaction_t tmp;
                tmp.trx = args.args->at(0).as<signed_transaction>();
                if (n_args > 1)
                    tmp.max_block_age = args.args->at(1).as<uint32_t>();
                FC_ASSERT(!check_max_block_age(tmp.max_block_age));
                pimpl->_chain.db().push_transaction(tmp.trx);
                pimpl->_p2p.broadcast_transaction(tmp.trx);

                return broadcast_transaction_return();
            }

            DEFINE_API(network_broadcast_api_plugin, broadcast_transaction_synchronous) {
                //auto tmp = args.args->at(0).as<broadcast_transaction_t>();
                auto n_args = args.args->size();
                FC_ASSERT(args.args->size() >= 1, "Expected at least 1 argument, got 0");
                broadcast_transaction_t tmp;
                tmp.trx = args.args->at(0).as<signed_transaction>();
                if (n_args > 1)
                    tmp.max_block_age = args.args->at(1).as<uint32_t>();
                FC_ASSERT(!check_max_block_age(tmp.max_block_age));
                boost::promise<broadcast_transaction_synchronous_return> p;
                {
                    boost::lock_guard<boost::mutex> guard(pimpl->_mtx);
                    pimpl->_callbacks[tmp.trx.id()] = [&p](const broadcast_transaction_synchronous_return &r) {
                        p.set_value(r);
                    };
                    pimpl->_callback_expirations[tmp.trx.expiration].push_back(tmp.trx.id());
                }

                pimpl->_chain.db().push_transaction(tmp.trx);
                pimpl->_p2p.broadcast_transaction(tmp.trx);

                return p.get_future().get();
            }

            DEFINE_API(network_broadcast_api_plugin, broadcast_block) {
                auto tmp = args.args->at(0).as<broadcast_block_t>();
                pimpl->_chain.db().push_block(tmp.block);
                pimpl->_p2p.broadcast_block(tmp.block);
                return broadcast_block_return();
            }

            bool network_broadcast_api_plugin::check_max_block_age(int32_t max_block_age) const {
                return pimpl->_chain.db().with_read_lock([&]() {
                    if (max_block_age < 0) {
                        return false;
                    }

                    fc::time_point_sec now = fc::time_point::now();
                    const auto &dgpo = pimpl->_chain.db().get_dynamic_global_properties();

                    return (dgpo.time < now - fc::seconds(max_block_age));
                });
            }

            void network_broadcast_api_plugin::set_program_options(boost::program_options::options_description &cli, boost::program_options::options_description &cfg) {
            }

            void network_broadcast_api_plugin::plugin_initialize(const boost::program_options::variables_map &options) {
                pimpl.reset(new impl);
                JSON_RPC_REGISTER_API(STEEM_NETWORK_BROADCAST_API_PLUGIN_NAME);
                on_applied_block_connection = appbase::app().get_plugin<chain::plugin>().db().applied_block.connect(
                        [&](const signed_block &b) { on_applied_block(b);
                        });
            }

            void network_broadcast_api_plugin::plugin_startup() {
            }

            void network_broadcast_api_plugin::plugin_shutdown() {
            }

            void network_broadcast_api_plugin::on_applied_block(const signed_block &b) { try {
                    boost::lock_guard< boost::mutex > guard( pimpl->_mtx );
                    int32_t block_num = int32_t(b.block_num());
                    if( pimpl->_callbacks.size() ) {
                        for( size_t trx_num = 0; trx_num < b.transactions.size(); ++trx_num ) {
                            const auto& trx = b.transactions[trx_num];
                            auto id = trx.id();
                            auto itr = pimpl->_callbacks.find( id );
                            if( itr ==pimpl-> _callbacks.end() ) continue;
                            itr->second( broadcast_transaction_synchronous_return( id, block_num, int32_t( trx_num ), false ) );
                            pimpl->_callbacks.erase( itr );
                        }
                    }

                    /// clear all expirations
                    while( true ) {
                        auto exp_it = pimpl->_callback_expirations.begin();
                        if( exp_it == pimpl->_callback_expirations.end() )
                            break;
                        if( exp_it->first >= b.timestamp )
                            break;
                        for( const transaction_id_type& txid : exp_it->second ) {
                            auto cb_it = pimpl->_callbacks.find( txid );
                            // If it's empty, that means the transaction has been confirmed and has been deleted by the above check.
                            if( cb_it == pimpl->_callbacks.end() )
                                continue;

                            confirmation_callback callback = cb_it->second;
                            transaction_id_type txid_byval = txid;    // can't pass in by reference as it's going to be deleted
                            callback( broadcast_transaction_synchronous_return( txid_byval, block_num, -1, true ) );

                            pimpl->_callbacks.erase( cb_it );
                        }
                        pimpl->_callback_expirations.erase( exp_it );
                    }
                } FC_LOG_AND_RETHROW() }
                #pragma message( "Remove FC_LOG_AND_RETHROW here before appbase release. It exists to help debug a rare lock exception" )

        }
    }
} // steem::plugins::network_broadcast_api


FC_REFLECT((golos::plugins::network_broadcast_api::broadcast_transaction_t), (trx)(max_block_age))

FC_REFLECT((golos::plugins::network_broadcast_api::broadcast_block_t), (block))
