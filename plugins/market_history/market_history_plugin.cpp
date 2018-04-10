#include <golos/plugins/market_history/market_history_plugin.hpp>

#include <golos/chain/index.hpp>
#include <golos/chain/operation_notification.hpp>
#include <golos/chain/steem_objects.hpp>
#include <golos/chain/account_object.hpp>



#define CHECK_ARG_SIZE(s) \
   FC_ASSERT( args.args->size() == s, "Expected #s argument(s), was ${n}", ("n", args.args->size()) );


namespace golos {
    namespace plugins {
        namespace market_history {

            using golos::protocol::fill_order_operation;
            using golos::chain::operation_notification;


            class market_history_plugin::market_history_plugin_impl {
            public:
                market_history_plugin_impl(market_history_plugin &plugin)
                        : _my(plugin),
                          _db(appbase::app().get_plugin<golos::plugins::chain::plugin>().db()){
                }

                ~market_history_plugin_impl() {
                }


                market_ticker get_ticker() const;
                market_volume get_volume() const;
                order_book get_order_book(uint32_t limit) const;
                order_book_extended get_order_book_extended(uint32_t limit) const;
                vector<market_trade> get_trade_history(time_point_sec start, time_point_sec end, uint32_t limit) const;
                vector<market_trade> get_recent_trades(uint32_t limit) const;
                vector<bucket_object> get_market_history(uint32_t bucket_seconds, time_point_sec start, time_point_sec end) const;
                flat_set<uint32_t> get_market_history_buckets() const;
                std::vector<limit_order> get_open_orders(std::string) const;


                void update_market_histories(const golos::chain::operation_notification &o);

                golos::chain::database &database() const {
                    return _db;
                }

                market_history_plugin &_my;
                flat_set<uint32_t> _tracked_buckets = flat_set<uint32_t>  {15, 60, 300, 3600, 86400 };

                int32_t _maximum_history_per_bucket_size = 1000;

                golos::chain::database &_db;
            };

            void market_history_plugin::market_history_plugin_impl::update_market_histories(const operation_notification &o) {
                if (o.op.which() ==
                    operation::tag<fill_order_operation>::value) {
                    fill_order_operation op = o.op.get<fill_order_operation>();

                    auto &db = database();
                    const auto &bucket_idx = db.get_index<bucket_index>().indices().get<by_bucket>();

                    db.create<order_history_object>([&](order_history_object &ho) {
                        ho.time = db.head_block_time();
                        ho.op = op;
                    });

                    if (!_maximum_history_per_bucket_size) {
                        return;
                    }
                    if (!_tracked_buckets.size()) {
                        return;
                    }

                    for (auto bucket : _tracked_buckets) {
                        auto cutoff = db.head_block_time() - fc::seconds(
                                bucket * _maximum_history_per_bucket_size);

                        auto open = fc::time_point_sec(
                                (db.head_block_time().sec_since_epoch() /
                                 bucket) * bucket);
                        auto seconds = bucket;

                        auto itr = bucket_idx.find(boost::make_tuple(seconds, open));
                        if (itr == bucket_idx.end()) {
                            db.create<bucket_object>([&](bucket_object &b) {
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
                            db.modify(*itr, [&](bucket_object &b) {
                                if (op.open_pays.symbol == STEEM_SYMBOL) {
                                    b.steem_volume += op.open_pays.amount;
                                    b.sbd_volume += op.current_pays.amount;
                                    b.close_steem = op.open_pays.amount;
                                    b.close_sbd = op.current_pays.amount;

                                    if (b.high() <
                                        price(op.current_pays, op.open_pays)) {
                                        b.high_steem = op.open_pays.amount;
                                        b.high_sbd = op.current_pays.amount;
                                    }

                                    if (b.low() >
                                        price(op.current_pays, op.open_pays)) {
                                        b.low_steem = op.open_pays.amount;
                                        b.low_sbd = op.current_pays.amount;
                                    }
                                } else {
                                    b.steem_volume += op.current_pays.amount;
                                    b.sbd_volume += op.open_pays.amount;
                                    b.close_steem = op.current_pays.amount;
                                    b.close_sbd = op.open_pays.amount;

                                    if (b.high() <
                                        price(op.open_pays, op.current_pays)) {
                                        b.high_steem = op.current_pays.amount;
                                        b.high_sbd = op.open_pays.amount;
                                    }

                                    if (b.low() >
                                        price(op.open_pays, op.current_pays)) {
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
                                    db.remove(*old_itr);
                                }
                            }
                        }
                    }
                }
            }

            market_ticker market_history_plugin::market_history_plugin_impl::get_ticker() const {
                market_ticker result;
                const auto &bucket_idx = database().get_index<bucket_index>().indices().get<by_bucket>();
                auto itr = bucket_idx.lower_bound(boost::make_tuple(86400, database().head_block_time() - 86400));

                if (itr != bucket_idx.end()) {
                    auto open = (asset(itr->open_sbd, SBD_SYMBOL) /
                                 asset(itr->open_steem, STEEM_SYMBOL)).to_real();
                    result.latest = (asset(itr->close_sbd, SBD_SYMBOL) /
                                     asset(itr->close_steem, STEEM_SYMBOL)).to_real();
                    result.percent_change =
                            ((result.latest - open) / open) * 100;
                } else {
                    result.latest = 0;
                    result.percent_change = 0;
                }

                auto orders = get_order_book(1);
                if (orders.bids.size()) {
                    result.highest_bid = orders.bids[0].price;
                }
                if (orders.asks.size()) {
                    result.lowest_ask = orders.asks[0].price;
                }

                auto volume = get_volume();
                result.steem_volume = volume.steem_volume;
                result.sbd_volume = volume.sbd_volume;

                return result;
            }

            market_volume market_history_plugin::market_history_plugin_impl::get_volume() const {
                const auto &bucket_idx = database().get_index<bucket_index>().indices().get<by_bucket>();
                auto itr = bucket_idx.lower_bound(boost::make_tuple(0, database().head_block_time() - 86400));
                market_volume result;

                if (itr == bucket_idx.end()) {
                    return result;
                }

                uint32_t bucket_size = itr->seconds;
                do {
                    result.steem_volume.amount += itr->steem_volume;
                    result.sbd_volume.amount += itr->sbd_volume;

                    ++itr;
                } while (itr != bucket_idx.end() &&
                         itr->seconds == bucket_size);

                return result;
            }

            order_book market_history_plugin::market_history_plugin_impl::get_order_book(uint32_t limit) const {
                FC_ASSERT(limit <= 500);

                const auto &order_idx = database().get_index<golos::chain::limit_order_index>().indices().get<golos::chain::by_price>();
                auto itr = order_idx.lower_bound(price::max(SBD_SYMBOL, STEEM_SYMBOL));

                order_book result;

                while (itr != order_idx.end() &&
                       itr->sell_price.base.symbol == SBD_SYMBOL &&
                       result.bids.size() < limit) {
                    order cur;
                    cur.price = itr->sell_price.base.to_real() / itr->sell_price.quote.to_real();
                    cur.steem = (asset(itr->for_sale, SBD_SYMBOL) * itr->sell_price).amount;
                    cur.sbd = itr->for_sale;
                    result.bids.push_back(cur);
                    ++itr;
                }

                itr = order_idx.lower_bound(price::max(STEEM_SYMBOL, SBD_SYMBOL));

                while (itr != order_idx.end() &&
                       itr->sell_price.base.symbol == STEEM_SYMBOL &&
                       result.asks.size() < limit) {
                    order cur;
                    cur.price = itr->sell_price.quote.to_real() / itr->sell_price.base.to_real();
                    cur.steem = itr->for_sale;
                    cur.sbd = (asset(itr->for_sale, STEEM_SYMBOL) * itr->sell_price).amount;
                    result.asks.push_back(cur);
                    ++itr;
                }

                return result;
            }

            order_book_extended market_history_plugin::market_history_plugin_impl::get_order_book_extended(uint32_t limit) const {
                FC_ASSERT(limit <= 1000);
                order_book_extended result;

                auto max_sell = price::max(SBD_SYMBOL, STEEM_SYMBOL);
                auto max_buy = price::max(STEEM_SYMBOL, SBD_SYMBOL);
                const auto &limit_price_idx = database().get_index<golos::chain::limit_order_index>().indices().get<golos::chain::by_price>();
                auto sell_itr = limit_price_idx.lower_bound(max_sell);
                auto buy_itr = limit_price_idx.lower_bound(max_buy);
                auto end = limit_price_idx.end();

                while (sell_itr != end &&
                       sell_itr->sell_price.base.symbol == SBD_SYMBOL &&
                       result.bids.size() < limit) {
                    auto itr = sell_itr;
                    order_extended cur;
                    cur.order_price = itr->sell_price;
                    cur.real_price = (cur.order_price).to_real();
                    cur.sbd = itr->for_sale;
                    cur.steem = (asset(itr->for_sale, SBD_SYMBOL) * cur.order_price).amount;
                    cur.created = itr->created;
                    result.bids.push_back(cur);
                    ++sell_itr;
                }
                while (buy_itr != end &&
                       buy_itr->sell_price.base.symbol == STEEM_SYMBOL &&
                       result.asks.size() < limit) {
                    auto itr = buy_itr;
                    order_extended cur;
                    cur.order_price = itr->sell_price;
                    cur.real_price = (~cur.order_price).to_real();
                    cur.steem = itr->for_sale;
                    cur.sbd = (asset(itr->for_sale, STEEM_SYMBOL) * cur.order_price).amount;
                    cur.created = itr->created;
                    result.asks.push_back(cur);
                    ++buy_itr;
                }

                return result;
            }


            vector<market_trade> market_history_plugin::market_history_plugin_impl::get_trade_history(
                    time_point_sec start, time_point_sec end, uint32_t limit) const {
                FC_ASSERT(limit <= 1000);
                const auto &bucket_idx = database().get_index<order_history_index>().indices().get<by_time>();
                auto itr = bucket_idx.lower_bound(start);

                std::vector<market_trade> result;

                while (itr != bucket_idx.end() && itr->time <= end &&
                       result.size() < limit) {
                    market_trade trade;
                    trade.date = itr->time;
                    trade.current_pays = itr->op.current_pays;
                    trade.open_pays = itr->op.open_pays;
                    result.push_back(trade);
                    ++itr;
                }

                return result;
            }

            vector<market_trade> market_history_plugin::market_history_plugin_impl::get_recent_trades(uint32_t limit) const {
                FC_ASSERT(limit <= 1000);
                const auto &order_idx = database().get_index<order_history_index>().indices().get<by_time>();
                auto itr = order_idx.rbegin();

                vector<market_trade> result;

                while (itr != order_idx.rend() && result.size() < limit) {
                    market_trade trade;
                    trade.date = itr->time;
                    trade.current_pays = itr->op.current_pays;
                    trade.open_pays = itr->op.open_pays;
                    result.push_back(trade);
                    ++itr;
                }

                return result;
            }

            vector<bucket_object> market_history_plugin::market_history_plugin_impl::get_market_history(
                    uint32_t bucket_seconds, time_point_sec start, time_point_sec end) const {
                const auto &bucket_idx = database().get_index<bucket_index>().indices().get<by_bucket>();
                auto itr = bucket_idx.lower_bound(boost::make_tuple(bucket_seconds, start));

                std::vector<bucket_object> result;

                while (itr != bucket_idx.end() &&
                       itr->seconds == bucket_seconds && itr->open < end) {
                    result.push_back(*itr);

                    ++itr;
                }

                return result;
            }

            flat_set<uint32_t> market_history_plugin::market_history_plugin_impl::get_market_history_buckets() const {
                return appbase::app().get_plugin<market_history_plugin>().get_tracked_buckets();
            }

            std::vector<limit_order> market_history_plugin::market_history_plugin_impl::get_open_orders(string owner) const {
                return _db.with_read_lock([&]() {
                    std::vector<limit_order> result;
                    const auto &idx = _db.get_index<golos::chain::limit_order_index>().indices().get<golos::chain::by_account>();
                    auto itr = idx.lower_bound(owner);
                    while (itr != idx.end() && itr->seller == owner) {
                        result.push_back(*itr);

                        if (itr->sell_price.base.symbol == STEEM_SYMBOL) {
                            result.back().real_price = (~result.back().sell_price).to_real();
                        } else {
                            result.back().real_price = (result.back().sell_price).to_real();
                        }
                        ++itr;
                    }
                    return result;
                });
            }

            market_history_plugin::market_history_plugin() {
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
                    ilog("market_history plugin: plugin_initialize() begin");
                    _my.reset(new market_history_plugin_impl(*this));
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

                    ilog("market_history plugin: plugin_initialize() end");
                    JSON_RPC_REGISTER_API ( name() ) ;
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
                    return _my->get_ticker();
                });
            }

            DEFINE_API(market_history_plugin, get_volume) {
                auto &db = _my->database();
                return db.with_read_lock([&]() {
                    return _my->get_volume();
                });
            }

            DEFINE_API(market_history_plugin, get_order_book) {
                CHECK_ARG_SIZE(1)
                auto limit = args.args->at(0).as<uint32_t>();
                auto &db = _my->database();
                return db.with_read_lock([&]() {
                    return _my->get_order_book(limit);
                });
            }

            DEFINE_API(market_history_plugin, get_order_book_extended) {
                CHECK_ARG_SIZE(1)
                auto limit = args.args->at(0).as<uint32_t>();
                auto &db = _my->database();
                return db.with_read_lock([&]() {
                    return _my->get_order_book_extended(limit);
                });
            }


            DEFINE_API(market_history_plugin, get_trade_history) {
                CHECK_ARG_SIZE(3)
                auto start = args.args->at(0).as<time_point_sec>();
                auto end = args.args->at(1).as<time_point_sec>();
                auto limit = args.args->at(2).as<uint32_t>();
                auto &db = _my->database();
                return db.with_read_lock([&]() {
                    return _my->get_trade_history(start, end, limit);
                });
            }

            DEFINE_API(market_history_plugin, get_recent_trades) {
                CHECK_ARG_SIZE(1)
                auto limit = args.args->at(0).as<uint32_t>();
                auto &db = _my->database();
                return db.with_read_lock([&]() {
                    return _my->get_recent_trades(limit);
                });
            }

            DEFINE_API(market_history_plugin, get_market_history) {
                CHECK_ARG_SIZE(3)
                auto bucket_seconds = args.args->at(0).as<uint32_t>();
                auto start = args.args->at(1).as<time_point_sec>();
                auto end = args.args->at(2).as<time_point_sec>();
                auto &db = _my->database();
                return db.with_read_lock([&]() {
                    return _my->get_market_history(bucket_seconds, start, end);
                });
            }

            DEFINE_API(market_history_plugin, get_market_history_buckets) {
                auto &db = _my->database();
                return db.with_read_lock([&]() {
                    return _my->get_market_history_buckets();
                });
            }

            DEFINE_API(market_history_plugin, get_open_orders) {
                auto tmp = args.args->at(0).as<string>();
                auto &db = _my->database();
                return db.with_read_lock([&]() {
                    return _my->get_open_orders(tmp);
                });
            }

        }
    }
} // golos::plugins::market_history
