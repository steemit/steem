#include <golos/plugins/market_history/market_history_plugin.hpp>

#include <golos/chain/index.hpp>
#include <golos/chain/operation_notification.hpp>

namespace golos {
    namespace plugins {
        namespace market_history {

            using golos::protocol::fill_order_operation;

            class market_history_plugin::market_history_plugin_impl {
            public:
                market_history_plugin_impl(market_history_plugin &plugin)
                        : _my(plugin),
                          _db(appbase::app().get_plugin<golos::plugins::chain::plugin>().db()){
                }

                ~market_history_plugin_impl() {
                }

                market_ticker_r get_ticker() const;
                market_volume_r get_volume() const;
                order_book_r get_order_book(const order_book_a &args) const;
                trade_history_r get_trade_history(const trade_history_a &args) const;
                recent_trades_r get_recent_trades(const recent_trades_a &args) const;
                market_history_r get_market_history(const market_history_a &args) const;
                market_history_buckets_r get_market_history_buckets() const;

                void update_market_histories(const golos::chain::operation_notification &o);

                golos::chain::database &database() const {
                    return _db;
                }

                market_history_plugin &_my;
                flat_set<uint32_t> _tracked_buckets = flat_set<uint32_t>  {15, 60, 300, 3600, 86400 };

                int32_t _maximum_history_per_bucket_size = 1000;

                golos::chain::database &_db;
            };

            void market_history_plugin::market_history_plugin_impl::update_market_histories(const golos::chain::operation_notification &o) {
                if (o.op.which() ==
                    operation::tag<fill_order_operation>::value) {
                    fill_order_operation op = o.op.get<fill_order_operation>();

                    const auto &bucket_idx = _db.get_index<bucket_index>().indices().get<by_bucket>();

                    _db.create<order_history_object>([&](order_history_object &ho) {
                        ho.time = _db.head_block_time();
                        ho.op = op;
                    });

                    if (!_maximum_history_per_bucket_size) {
                        return;
                    }
                    if (!_tracked_buckets.size()) {
                        return;
                    }

                    for (auto bucket : _tracked_buckets) {
                        auto cutoff = _db.head_block_time() - fc::seconds(
                                bucket * _maximum_history_per_bucket_size);

                        auto open = fc::time_point_sec(
                                (_db.head_block_time().sec_since_epoch() /
                                 bucket) * bucket);
                        auto seconds = bucket;

                        auto itr = bucket_idx.find(boost::make_tuple(seconds, open));
                        if (itr == bucket_idx.end()) {
                            _db.create<bucket_object>([&](bucket_object &b) {
                                b.open = open;
                                b.seconds = bucket;

                                if (op.open_pays.symbol == STEEM_SYMBOL) {
                                    b.high_steem = op.open_pays.amount;
                                    b.high_sbd = op.current_pays.amount;
                                    b.low_steem = op.open_pays.amount;
                                    b.low_sbd = op.current_pays.amount;
                                    b.open_steem = op.open_pays.amount;
                                    b.open_sbd = op.current_pays.amount;
                                    b.close_steem = op.open_pays.amount;
                                    b.close_sbd = op.current_pays.amount;
                                    b.steem_volume = op.open_pays.amount;
                                    b.sbd_volume = op.current_pays.amount;
                                } else {
                                    b.high_steem = op.current_pays.amount;
                                    b.high_sbd = op.open_pays.amount;
                                    b.low_steem = op.current_pays.amount;
                                    b.low_sbd = op.open_pays.amount;
                                    b.open_steem = op.current_pays.amount;
                                    b.open_sbd = op.open_pays.amount;
                                    b.close_steem = op.current_pays.amount;
                                    b.close_sbd = op.open_pays.amount;
                                    b.steem_volume = op.current_pays.amount;
                                    b.sbd_volume = op.open_pays.amount;
                                }
                            });
                        } else {
                            _db.modify(*itr, [&](bucket_object &b) {
                                if (op.open_pays.symbol == STEEM_SYMBOL) {
                                    b.steem_volume += op.open_pays.amount;
                                    b.sbd_volume += op.current_pays.amount;
                                    b.close_steem = op.open_pays.amount;
                                    b.close_sbd = op.current_pays.amount;

                                    if (b.high() <
                                        golos::protocol::price(op.current_pays, op.open_pays)) {
                                        b.high_steem = op.open_pays.amount;
                                        b.high_sbd = op.current_pays.amount;
                                    }

                                    if (b.low() >
                                        golos::protocol::price(op.current_pays, op.open_pays)) {
                                        b.low_steem = op.open_pays.amount;
                                        b.low_sbd = op.current_pays.amount;
                                    }
                                } else {
                                    b.steem_volume += op.current_pays.amount;
                                    b.sbd_volume += op.open_pays.amount;
                                    b.close_steem = op.current_pays.amount;
                                    b.close_sbd = op.open_pays.amount;

                                    if (b.high() <
                                        golos::protocol::price(op.open_pays, op.current_pays)) {
                                        b.high_steem = op.current_pays.amount;
                                        b.high_sbd = op.open_pays.amount;
                                    }

                                    if (b.low() >
                                        golos::protocol::price(op.open_pays, op.current_pays)) {
                                        b.low_steem = op.current_pays.amount;
                                        b.low_sbd = op.open_pays.amount;
                                    }
                                }
                            });

                            if (_maximum_history_per_bucket_size > 0) {
                                open = fc::time_point_sec();
                                itr = bucket_idx.lower_bound(boost::make_tuple(seconds, open));

                                while (itr->seconds == seconds &&
                                       itr->open < cutoff) {
                                    auto old_itr = itr;
                                    ++itr;
                                    _db.remove(*old_itr);
                                }
                            }
                        }
                    }
                }
            }

            market_ticker_r market_history_plugin::market_history_plugin_impl::get_ticker() const {
                market_ticker_r result;

                const auto &bucket_idx = _db.get_index<bucket_index>().indices().get<by_bucket>();
                auto itr = bucket_idx.lower_bound(boost::make_tuple(86400,
                                                                    _db.head_block_time() - 86400));

                if (itr != bucket_idx.end()) {
                    auto open = (golos::protocol::asset(itr->open_sbd, SBD_SYMBOL) /
                                 golos::protocol::asset(itr->open_steem, STEEM_SYMBOL)).to_real();
                    result.ticker.latest = (golos::protocol::asset(itr->close_sbd, SBD_SYMBOL) /
                                            golos::protocol::asset(itr->close_steem, STEEM_SYMBOL)).to_real();
                    result.ticker.percent_change =
                            ((result.ticker.latest - open) / open) * 100;
                } else {
                    result.ticker.latest = 0;
                    result.ticker.percent_change = 0;
                }

                order_book_a tmp;
                tmp.limit = 1;
                auto orders = get_order_book(tmp);
                if (orders.orders.bids.size()) {
                    result.ticker.highest_bid = orders.orders.bids[0].price;
                }
                if (orders.orders.asks.size()) {
                    result.ticker.lowest_ask = orders.orders.asks[0].price;
                }

                auto volume = get_volume();
                result.ticker.steem_volume = volume.volume.steem_volume;
                result.ticker.sbd_volume = volume.volume.sbd_volume;

                return result;
            }

            market_volume_r market_history_plugin::market_history_plugin_impl::get_volume() const {

                const auto &bucket_idx = _db.get_index<bucket_index>().indices().get<by_bucket>();
                auto itr = bucket_idx.lower_bound(boost::make_tuple(0,
                                                                    _db.head_block_time() - 86400));
                market_volume_r result;

                if (itr == bucket_idx.end()) {
                    return result;
                }

                uint32_t bucket_size = itr->seconds;
                do {
                    result.volume.steem_volume.amount += itr->steem_volume;
                    result.volume.sbd_volume.amount += itr->sbd_volume;

                    ++itr;
                } while (itr != bucket_idx.end() &&
                         itr->seconds == bucket_size);

                return result;
            }

            order_book_r market_history_plugin::market_history_plugin_impl::get_order_book(const order_book_a& args) const {
                FC_ASSERT(args.limit <= 500);

                const auto &order_idx = _db.get_index<golos::chain::limit_order_index>().indices().get<golos::chain::by_price>();
                auto itr = order_idx.lower_bound(golos::protocol::price::max(SBD_SYMBOL, STEEM_SYMBOL));

                order_book_r result;

                while (itr != order_idx.end() &&
                       itr->sell_price.base.symbol == SBD_SYMBOL &&
                       result.orders.bids.size() < args.limit) {
                    order cur;
                    cur.price = itr->sell_price.base.to_real() /
                                itr->sell_price.quote.to_real();
                    cur.steem = (asset(itr->for_sale, SBD_SYMBOL) *
                                 itr->sell_price).amount;
                    cur.sbd = itr->for_sale;
                    result.orders.bids.push_back(cur);
                    ++itr;
                }

                itr = order_idx.lower_bound(golos::protocol::price::max(STEEM_SYMBOL, SBD_SYMBOL));

                while (itr != order_idx.end() &&
                       itr->sell_price.base.symbol == STEEM_SYMBOL &&
                       result.orders.asks.size() < args.limit) {
                    order cur;
                    cur.price = itr->sell_price.quote.to_real() /
                                itr->sell_price.base.to_real();
                    cur.steem = itr->for_sale;
                    cur.sbd = (asset(itr->for_sale, STEEM_SYMBOL) *
                               itr->sell_price).amount;
                    result.orders.asks.push_back(cur);
                    ++itr;
                }

                return result;
            }

            trade_history_r market_history_plugin::market_history_plugin_impl::get_trade_history(const trade_history_a& args) const {
                FC_ASSERT(args.limit <= 1000);
                const auto &bucket_idx = _db.get_index<order_history_index>().indices().get<by_time>();
                auto itr = bucket_idx.lower_bound(args.start);

                trade_history_r result;

                while (itr != bucket_idx.end() && itr->time <= args.end &&
                       result.history.size() < args.limit) {
                    market_trade trade;
                    trade.date = itr->time;
                    trade.current_pays = itr->op.current_pays;
                    trade.open_pays = itr->op.open_pays;
                    result.history.push_back(trade);
                    ++itr;
                }

                return result;
            }

            recent_trades_r market_history_plugin::market_history_plugin_impl::get_recent_trades(const recent_trades_a& args) const {
                FC_ASSERT(args.limit <= 1000);
                const auto &order_idx = _db.get_index<order_history_index>().indices().get<by_time>();
                auto itr = order_idx.rbegin();

                recent_trades_r result;

                while (itr != order_idx.rend() && result.trades.size() < args.limit) {
                    market_trade trade;
                    trade.date = itr->time;
                    trade.current_pays = itr->op.current_pays;
                    trade.open_pays = itr->op.open_pays;
                    result.trades.push_back(trade);
                    ++itr;
                }

                return result;
            }

            market_history_r market_history_plugin::market_history_plugin_impl::get_market_history(const market_history_a& args) const {
                const auto &bucket_idx = _db.get_index<bucket_index>().indices().get<by_bucket>();
                auto itr = bucket_idx.lower_bound(boost::make_tuple(args.bucket_seconds, args.start));

                market_history_r result;

                while (itr != bucket_idx.end() &&
                       itr->seconds == args.bucket_seconds && itr->open < args.end) {
                    result.history.push_back(*itr);

                    ++itr;
                }

                return result;
            }

            market_history_buckets_r market_history_plugin::market_history_plugin_impl::get_market_history_buckets() const {
                market_history_buckets_r retval;
                retval.buckets = appbase::app().get_plugin<market_history_plugin>().get_tracked_buckets();
                return retval;
            }

            market_history_plugin::market_history_plugin()
                    : _my(new market_history_plugin_impl(*this)){
            }

            market_history_plugin::~market_history_plugin() {
            }

            void market_history_plugin::set_program_options(
                    boost::program_options::options_description &cli,
                    boost::program_options::options_description &cfg
            ) {
                cli.add_options()
                        ("market-history-bucket-size",
                         boost::program_options::value<string>()->default_value("[15,60,300,3600,86400]"),
                         "Track market history by grouping orders into buckets of equal size measured in seconds specified as a JSON array of numbers")
                        ("market-history-buckets-per-size",
                         boost::program_options::value<uint32_t>()->default_value(5760),
                         "How far back in time to track history for each bucket size, measured in the number of buckets (default: 5760)");
                cfg.add(cli);
            }

            void market_history_plugin::plugin_initialize(const boost::program_options::variables_map &options) {
                try {
                    ilog("market_history: plugin_initialize() begin");

                    golos::chain::database& db = _my->database();

                    db.post_apply_operation.connect(
                            [&](const golos::chain::operation_notification &o) { _my->update_market_histories(o); });
                    golos::chain::add_plugin_index<bucket_index>(db);
                    golos::chain::add_plugin_index<order_history_index>(db);

                    if (options.count("bucket-size")) {
                        std::string buckets = options["bucket-size"].as<string>();
                        _my->_tracked_buckets = fc::json::from_string(buckets).as<flat_set<uint32_t>>();
                    }
                    if (options.count("history-per-size")) {
                        _my->_maximum_history_per_bucket_size = options["history-per-size"].as<uint32_t>();
                    }

                    wlog("bucket-size ${b}", ("b", _my->_tracked_buckets));
                    wlog("history-per-size ${h}", ("h", _my->_maximum_history_per_bucket_size));

                    ilog("market_history: plugin_initialize() end");
                } FC_CAPTURE_AND_RETHROW()
            }

            void market_history_plugin::plugin_startup() {
                ilog("market_history plugin: plugin_startup() begin");

                ilog("market_history plugin: plugin_startup() end");
            }

            void market_history_plugin::plugin_shutdown() {
                ilog("market_history plugin: plugin_shutdown() begin");

                ilog("market_history plugin: plugin_shutdown() end");
            }

            flat_set<uint32_t> market_history_plugin::get_tracked_buckets() const {
                return _my->_tracked_buckets;
            }

            uint32_t market_history_plugin::get_max_history_per_bucket() const {
                return _my->_maximum_history_per_bucket_size;
            }


            // Api Defines

            DEFINE_API(market_history_plugin, get_ticker) {
                auto &db = _my->database();
                return db.with_read_lock([&]() {
                    market_ticker_r result;
                    return _my->get_ticker();
                });
            }

            DEFINE_API(market_history_plugin, get_volume) {
                auto &db = _my->database();
                return db.with_read_lock([&]() {
                    market_volume_r result;
                    return _my->get_volume();
                });
            }

            DEFINE_API(market_history_plugin, get_order_book) {
                auto tmp = args.args->at(0).as<order_book_a>();
                auto &db = _my->database();
                return db.with_read_lock([&]() {
                    order_book_r result;
                    result = _my->get_order_book(tmp);
                    return result;
                });
            }

            DEFINE_API(market_history_plugin, get_trade_history) {
                auto tmp = args.args->at(0).as<trade_history_a>();
                auto &db = _my->database();
                return db.with_read_lock([&]() {
                    trade_history_r result;
                    result = _my->get_trade_history(tmp);
                    return result;
                });
            }

            DEFINE_API(market_history_plugin, get_recent_trades) {
                auto tmp = args.args->at(0).as<recent_trades_a>();
                auto &db = _my->database();
                return db.with_read_lock([&]() {
                    recent_trades_r result;
                    result = _my->get_recent_trades(tmp);
                    return result;
                });
            }

            DEFINE_API(market_history_plugin, get_market_history) {
                auto tmp = args.args->at(0).as<market_history_a>();
                auto &db = _my->database();
                return db.with_read_lock([&]() {
                    market_history_r result;
                    result = _my->get_market_history(tmp);
                    return result;
                });
            }

            DEFINE_API(market_history_plugin, get_market_history_buckets) {
                auto &db = _my->database();
                return db.with_read_lock([&]() {
                    market_history_buckets_r result;
                    return _my->get_market_history_buckets();
                });
            }

        }
    }
} // golos::plugins::market_history

//STEEMIT_DEFINE_PLUGIN(market_history, golos::market_history::market_history_plugin)
