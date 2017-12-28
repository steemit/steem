#include <golos/plugins/database_api/plugin.hpp>

//#include <golos/plugins/tags/tags_plugin.hpp>

#include <golos/protocol/get_config.hpp>

#include <fc/bloom_filter.hpp>
#include <fc/smart_ref_impl.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>
#include <memory>
#include <golos/plugins/json_rpc/plugin.hpp>

#define GET_REQUIRED_FEES_MAX_RECURSION 4

#define CHECK_ARG_SIZE(s) \
   FC_ASSERT( args.args->size() == s, "Expected #s argument(s), was ${n}", ("n", args.args->size()) );

namespace golos {
    namespace plugins {
        namespace database_api {

            template<class C, typename... Args>
            boost::signals2::scoped_connection connect_signal(boost::signals2::signal<void(Args...)> &sig, C &c, void(C::* f)(Args...)) {
                std::weak_ptr<C> weak_c = c.shared_from_this();
                return sig.connect([weak_c, f](Args... args) {
                    std::shared_ptr<C> shared_c = weak_c.lock();
                    if (!shared_c) {
                        return;
                    }
                    ((*shared_c).*f)(args...);
                });
            }
/*
            std::string get_language(const comment_api_object &c) {
                languages::comment_metadata meta;
                std::string language("");
                if (!c.json_metadata.empty()) {
                    try {
                        meta = fc::json::from_string(c.json_metadata).as<languages::comment_metadata>();
                        language = meta.language;
                    } catch (...) {
                        // Do nothing on malformed json_metadata
                    }
                }

                return language;
            }
*/
            bool tags_filter(const discussion_query &query, const comment_api_object &c,
                             const std::function<bool(const comment_api_object &)> &condition) {
                if (query.select_authors.size()) {
                    if (query.select_authors.find(c.author) == query.select_authors.end()) {
                        return true;
                    }
                }

                tags::comment_metadata meta;

                if (!c.json_metadata.empty()) {
                    try {
                        meta = fc::json::from_string(c.json_metadata).as<tags::comment_metadata>();
                    } catch (const fc::exception &e) {
                        // Do nothing on malformed json_metadata
                    }
                }

                for (const std::set<std::string>::value_type &iterator : query.filter_tags) {
                    if (meta.tags.find(iterator) != meta.tags.end()) {
                        return true;
                    }
                }

                return condition(c) || query.filter_tags.find(c.category) != query.filter_tags.end();
            }

/*
            bool languages_filter(const discussion_query &query, const comment_api_object &c,
                                  const std::function<bool(const comment_api_object &)> &condition) {
                std::string language = get_language(c);

                if (query.filter_languages.size()) {
                    if (language.empty()) {
                        return true;
                    }
                }

                if (query.filter_languages.count(language)) {
                    return true;
                }

                return false || condition(c);
            }
*/
            struct plugin::api_impl final: public std::enable_shared_from_this<api_impl> {
            public:
                api_impl();

                ~api_impl();

                // Subscriptions
                void set_subscribe_callback(std::function<void(const variant &)> cb, bool clear_filter);

                void set_pending_transaction_callback(std::function<void(const variant &)> cb);

                void set_block_applied_callback(std::function<void(const variant &block_id)> cb);

                void cancel_all_subscriptions();

                // Blocks and transactions
                optional<block_header> get_block_header(uint32_t block_num) const;

                optional<signed_block> get_block(uint32_t block_num) const;

                std::vector<applied_operation> get_ops_in_block(uint32_t block_num, bool only_virtual) const;

                // Globals
                fc::variant_object get_config() const;

                dynamic_global_property_api_object get_dynamic_global_properties() const;

                // Accounts
                std::vector<extended_account> get_accounts(std::vector<std::string> names) const;

                std::vector<optional<account_api_object>> lookup_account_names(
                        const std::vector<std::string> &account_names) const;

                std::set<std::string> lookup_accounts(const std::string &lower_bound_name, uint32_t limit) const;

                uint64_t get_account_count() const;

                // Witnesses
                std::vector<optional<witness_api_object>> get_witnesses(
                        const std::vector<witness_object::id_type> &witness_ids) const;

                fc::optional<witness_api_object> get_witness_by_account(std::string account_name) const;

                std::set<account_name_type> lookup_witness_accounts(const std::string &lower_bound_name,
                                                                    uint32_t limit) const;

                uint64_t get_witness_count() const;

                // Balances

                // Authority / validation
                std::string get_transaction_hex(const signed_transaction &trx) const;

                std::set<public_key_type> get_required_signatures(const signed_transaction &trx, const flat_set<
                        public_key_type> &available_keys) const;

                std::set<public_key_type> get_potential_signatures(const signed_transaction &trx) const;

                std::vector<vote_state> get_active_votes(std::string author, std::string permlink) const;

                bool verify_authority(const signed_transaction &trx) const;

                bool verify_account_authority(const std::string &name_or_id,
                                              const flat_set<public_key_type> &signers) const;


                std::vector<discussion> get_content_replies(std::string author, std::string permlink) const;

                std::vector<withdraw_route> get_withdraw_routes(std::string account, withdraw_route_type type) const;

                std::vector<tag_api_object> get_trending_tags(std::string after, uint32_t limit) const;

                std::vector<std::pair<std::string, uint32_t>> get_tags_used_by_author(const std::string &author) const;

                std::map<uint32_t, applied_operation> get_account_history(std::string account, uint64_t from,
                                                                          uint32_t limit) const;

                void set_pending_payout(discussion &d) const;

                void set_url(discussion &d) const;

                std::vector<discussion> get_replies_by_last_update(account_name_type start_parent_author,
                                                                   std::string start_permlink, uint32_t limit) const;

                discussion get_content(std::string author, std::string permlink) const;

                std::vector<witness_api_object> get_witnesses_by_vote(std::string from, uint32_t limit) const;

                std::vector<account_name_type> get_miner_queue() const;

                vector<discussion> get_comment_discussions_by_payout(const discussion_query &query) const;

                vector<discussion> get_post_discussions_by_payout(const discussion_query &query) const;

                std::vector<discussion> get_discussions_by_promoted(const discussion_query &query) const;

                std::vector<discussion> get_discussions_by_created(const discussion_query &query) const;

                std::vector<discussion> get_discussions_by_cashout(const discussion_query &query) const;

                std::vector<discussion> get_discussions_by_votes(const discussion_query &query) const;

                std::vector<discussion> get_discussions_by_active(const discussion_query &query) const;

                discussion get_discussion(comment_object::id_type, uint32_t truncate_body = 0) const;

                std::vector<discussion> get_discussions_by_children(const discussion_query &query) const;


                std::vector<discussion> get_discussions_by_hot(const discussion_query &query) const;

                comment_object::id_type get_parent(const discussion_query &query) const {
                    return database().with_read_lock([&]() {
                        comment_object::id_type parent;
                        if (query.parent_author && query.parent_permlink) {
                            parent = database().get_comment(*query.parent_author, *query.parent_permlink).id;
                        }
                        return parent;
                    });
                }

                template<typename Object, typename DatabaseIndex, typename DiscussionIndex, typename CommentIndex,
                        typename Index, typename StartItr>
                std::multimap<Object, discussion, DiscussionIndex> get_discussions(const discussion_query &query,
                                                                                   const std::string &tag,
                                                                                   comment_object::id_type parent,
                                                                                   const Index &tidx, StartItr tidx_itr,
                                                                                   const std::function<
                                                                                           bool(const comment_api_object &)> &filter,
                                                                                   const std::function<
                                                                                           bool(const comment_api_object &)> &exit,
                                                                                   const std::function<
                                                                                           bool(const Object &)> &tag_exit,
                                                                                   bool ignore_parent = false) const;


                template<typename Object, typename DatabaseIndex, typename DiscussionIndex, typename CommentIndex,
                        typename ...Args>
                std::multimap<Object, discussion, DiscussionIndex> select(const std::set<std::string> &select_set,
                                                                          const discussion_query &query,
                                                                          comment_object::id_type parent,
                                                                          const std::function<
                                                                                  bool(const comment_api_object &)> &filter,
                                                                          const std::function<
                                                                                  bool(const comment_api_object &)> &exit,
                                                                          const std::function<
                                                                                  bool(const Object &)> &exit2,
                                                                          Args... args) const;


                template<typename T>
                void subscribe_to_item(const T &i) const {
                    auto vec = fc::raw::pack(i);
                    if (!_subscribe_callback) {
                        return;
                    }

                    if (!is_subscribed_to_item(i)) {
                        idump((i));
                        _subscribe_filter.insert(vec.data(), vec.size());//(vecconst char*)&i, sizeof(i) );
                    }
                }

                template<typename T>
                bool is_subscribed_to_item(const T &i) const {
                    if (!_subscribe_callback) {
                        return false;
                    }

                    return _subscribe_filter.contains(i);
                }

                // signal handlers
                void on_applied_block(const golos::protocol::signed_block &b);

                mutable fc::bloom_filter _subscribe_filter;
                std::function<void(const fc::variant &)> _subscribe_callback;
                std::function<void(const fc::variant &)> _pending_trx_callback;
                std::function<void(const fc::variant &)> _block_applied_callback;


                golos::chain::database &database() const {
                    return _db;
                }


                boost::signals2::scoped_connection _block_applied_connection;

                std::map<std::pair<asset_symbol_type, asset_symbol_type>,
                        std::function<void(const variant &)>> _market_subscriptions;
            private:

                golos::chain::database &_db;
            };


            void find_accounts(std::set<std::string> &accounts, const discussion &d) {
                accounts.insert(d.author);
            }

            //////////////////////////////////////////////////////////////////////
            //                                                                  //
            // Subscriptions                                                    //
            //                                                                  //
            //////////////////////////////////////////////////////////////////////

            void plugin::set_subscribe_callback(std::function<void(const variant &)> cb, bool clear_filter) {
                my->database().with_read_lock([&]() {
                    my->set_subscribe_callback(cb, clear_filter);
                });
            }

            void plugin::api_impl::set_subscribe_callback(std::function<void(const variant &)> cb,
                                                                bool clear_filter) {
                _subscribe_callback = cb;
                if (clear_filter || !cb) {
                    static fc::bloom_parameters param;
                    param.projected_element_count = 10000;
                    param.false_positive_probability = 1.0 / 10000;
                    param.maximum_size = 1024 * 8 * 8 * 2;
                    param.compute_optimal_parameters();
                    _subscribe_filter = fc::bloom_filter(param);
                }
            }

            void plugin::set_pending_transaction_callback(std::function<void(const variant &)> cb) {
                my->database().with_read_lock([&]() {
                    my->set_pending_transaction_callback(cb);
                });
            }

            void plugin::api_impl::set_pending_transaction_callback(std::function<void(const variant &)> cb) {
                _pending_trx_callback = cb;
            }

            void plugin::set_block_applied_callback(std::function<void(const variant &block_id)> cb) {
                my->database().with_read_lock([&]() {
                    my->set_block_applied_callback(cb);
                });
            }

            void plugin::api_impl::on_applied_block(const golos::protocol::signed_block &b) {
                try {
                    _block_applied_callback(fc::variant(signed_block_header(b)));
                } catch (...) {
                    _block_applied_connection.release();
                }
            }

            void plugin::api_impl::set_block_applied_callback(std::function<void(const variant &block_header)> cb) {
                _block_applied_callback = cb;
                _block_applied_connection = connect_signal(database().applied_block, *this, &api_impl::on_applied_block);
            }

            void plugin::cancel_all_subscriptions() {
                my->database().with_read_lock([&]() {
                    my->cancel_all_subscriptions();
                });
            }

            void plugin::api_impl::cancel_all_subscriptions() {
                set_subscribe_callback(std::function<void(const fc::variant &)>(), true);
            }

            //////////////////////////////////////////////////////////////////////
            //                                                                  //
            // Constructors                                                     //
            //                                                                  //
            //////////////////////////////////////////////////////////////////////

            plugin::plugin()  {
            }

            plugin::~plugin() {
            }

            plugin::api_impl::api_impl() : _db(appbase::app().get_plugin<chain::plugin>().db()) {
                wlog("creating database plugin ${x}", ("x", int64_t(this)));
            }

            plugin::api_impl::~api_impl() {
                elog("freeing database plugin ${x}", ("x", int64_t(this)));
            }


            //////////////////////////////////////////////////////////////////////
            //                                                                  //
            // Blocks and transactions                                          //
            //                                                                  //
            //////////////////////////////////////////////////////////////////////

            DEFINE_API(plugin, get_block_header) {
                CHECK_ARG_SIZE(1)
                return my->database().with_read_lock([&]() {
                    return my->get_block_header(args.args->at(0).as<uint32_t>());
                });
            }

            optional<block_header> plugin::api_impl::get_block_header(uint32_t block_num) const {
                auto result = database().fetch_block_by_number(block_num);
                if (result) {
                    return *result;
                }
                return {};
            }

            DEFINE_API(plugin, get_block) {
                CHECK_ARG_SIZE(1)
                return my->database().with_read_lock([&]() {
                    return my->get_block(args.args->at(0).as<uint32_t>());
                });
            }

            optional<signed_block> plugin::api_impl::get_block(uint32_t block_num) const {
                return database().fetch_block_by_number(block_num);
            }

            DEFINE_API(plugin, get_ops_in_block) {
                CHECK_ARG_SIZE(2)
                auto block_num = args.args->at(0).as<uint32_t>();
                auto only_virtual = args.args->at(1).as<bool>();
                return my->database().with_read_lock([&]() {
                    return my->get_ops_in_block(block_num, only_virtual);
                });
            }

            std::vector<applied_operation> plugin::api_impl::get_ops_in_block(uint32_t block_num,  bool only_virtual) const {
                const auto &idx = database().get_index<operation_index>().indices().get<by_location>();
                auto itr = idx.lower_bound(block_num);
                std::vector<applied_operation> result;
                applied_operation temp;
                while (itr != idx.end() && itr->block == block_num) {
                    temp = *itr;
                    if (!only_virtual || is_virtual_operation(temp.op)) {
                        result.push_back(temp);
                    }
                    ++itr;
                }
                return result;
            }


            //////////////////////////////////////////////////////////////////////
            //                                                                  //
            // Globals                                                          //
            //                                                                  //
            //////////////////////////////////////////////////////////////////////

            DEFINE_API(plugin, get_config) {
                return my->database().with_read_lock([&]() {
                    return my->get_config();
                });
            }

            fc::variant_object plugin::api_impl::get_config() const {
                return golos::protocol::get_config();
            }

            DEFINE_API(plugin, get_dynamic_global_properties) {
                return my->database().with_read_lock([&]() {
                    return my->get_dynamic_global_properties();
                });
            }

            DEFINE_API(plugin, get_chain_properties) {
                return my->database().with_read_lock([&]() {
                    return my->database().get_witness_schedule_object().median_props;
                });
            }

            DEFINE_API(plugin, get_feed_history) {
                return my->database().with_read_lock([&]() {
                    return feed_history_api_object(my->database().get_feed_history());
                });
            }

            DEFINE_API(plugin, get_current_median_history_price) {
                return my->database().with_read_lock([&]() {
                    return my->database().get_feed_history().current_median_history;
                });
            }

            dynamic_global_property_api_object plugin::api_impl::get_dynamic_global_properties() const {
                return database().get(dynamic_global_property_object::id_type());
            }

            DEFINE_API(plugin, get_witness_schedule) {
                return my->database().with_read_lock([&]() {
                    return my->database().get(witness_schedule_object::id_type());
                });
            }

            DEFINE_API(plugin, get_hardfork_version) {
                return my->database().with_read_lock([&]() {
                    return my->database().get(hardfork_property_object::id_type()).current_hardfork_version;
                });
            }

            DEFINE_API(plugin, get_next_scheduled_hardfork) {
                return my->database().with_read_lock([&]() {
                    scheduled_hardfork shf;
                    const auto &hpo = my->database().get(hardfork_property_object::id_type());
                    shf.hf_version = hpo.next_hardfork;
                    shf.live_time = hpo.next_hardfork_time;
                    return shf;
                });
            }

            //////////////////////////////////////////////////////////////////////
            //                                                                  //
            // Accounts                                                         //
            //                                                                  //
            //////////////////////////////////////////////////////////////////////

            DEFINE_API(plugin, get_accounts) {
                CHECK_ARG_SIZE(1)
                return my->database().with_read_lock([&]() {
                    return my->get_accounts(args.args->at(0).as<vector<std::string> >());
                });
            }

            std::vector<extended_account> plugin::api_impl::get_accounts(std::vector<std::string> names) const {
                const auto &idx = _db.get_index<account_index>().indices().get<by_name>();
                const auto &vidx = _db.get_index<witness_vote_index>().indices().get<by_account_witness>();
                std::vector<extended_account> results;

                for (auto name: names) {
                    auto itr = idx.find(name);
                    if (itr != idx.end()) {
                        results.push_back(extended_account(*itr, _db));
                        auto vitr = vidx.lower_bound(boost::make_tuple(itr->id, witness_id_type()));
                        while (vitr != vidx.end() && vitr->account == itr->id) {
                            results.back().witness_votes.insert(_db.get(vitr->witness).owner);
                            ++vitr;
                        }
                    }
                }

                return results;
            }


            DEFINE_API(plugin, lookup_account_names) {
                CHECK_ARG_SIZE(1)
                return my->database().with_read_lock([&]() {
                    return my->lookup_account_names(args.args->at(0).as<vector<std::string> >());
                });
            }

            std::vector<optional<account_api_object>> plugin::api_impl::lookup_account_names(
                    const std::vector<std::string> &account_names) const {
                std::vector<optional<account_api_object>> result;
                result.reserve(account_names.size());

                for (auto &name : account_names) {
                    auto itr = database().find<account_object, by_name>(name);

                    if (itr) {
                        result.push_back(account_api_object(*itr, database()));
                    } else {
                        result.push_back(optional<account_api_object>());
                    }
                }

                return result;
            }

            DEFINE_API(plugin, lookup_accounts) {
                CHECK_ARG_SIZE(2)
                account_name_type lower_bound_name = args.args->at(0).as<account_name_type>();
                uint32_t limit = args.args->at(1).as<uint32_t>();
                return my->database().with_read_lock([&]() {
                    return my->lookup_accounts(lower_bound_name, limit);
                });
            }

            std::set<std::string> plugin::api_impl::lookup_accounts(const std::string &lower_bound_name,
                                                                          uint32_t limit) const {
                FC_ASSERT(limit <= 1000);
                const auto &accounts_by_name = database().get_index<account_index>().indices().get<by_name>();
                std::set<std::string> result;

                for (auto itr = accounts_by_name.lower_bound(lower_bound_name);
                     limit-- && itr != accounts_by_name.end(); ++itr) {
                    result.insert(itr->name);
                }

                return result;
            }

            DEFINE_API(plugin, get_account_count) {
                return my->database().with_read_lock([&]() {
                    return my->get_account_count();
                });
            }

            uint64_t plugin::api_impl::get_account_count() const {
                return database().get_index<account_index>().indices().size();
            }

            DEFINE_API(plugin, get_owner_history) {
                CHECK_ARG_SIZE(1)
                auto account = args.args->at(0).as<string>();
                return my->database().with_read_lock([&]() {
                    std::vector<owner_authority_history_api_object> results;
                    const auto &hist_idx = my->database().get_index<owner_authority_history_index>().indices().get<
                            by_account>();
                    auto itr = hist_idx.lower_bound(account);

                    while (itr != hist_idx.end() && itr->account == account) {
                        results.push_back(owner_authority_history_api_object(*itr));
                        ++itr;
                    }

                    return results;
                });
            }

            DEFINE_API(plugin, get_recovery_request) {
                CHECK_ARG_SIZE(1)
                auto account = args.args->at(0).as<account_name_type>();
                return my->database().with_read_lock([&]() {
                    optional<account_recovery_request_api_object> result;

                    const auto &rec_idx = my->database().get_index<account_recovery_request_index>().indices().get<
                            by_account>();
                    auto req = rec_idx.find(account);

                    if (req != rec_idx.end()) {
                        result = account_recovery_request_api_object(*req);
                    }

                    return result;
                });
            }

            DEFINE_API(plugin, get_escrow) {
                CHECK_ARG_SIZE(2)
                auto from = args.args->at(0).as<account_name_type>();
                auto escrow_id = args.args->at(1).as<uint32_t>();
                return my->database().with_read_lock([&]() {
                    optional<escrow_api_object> result;

                    try {
                        result = my->database().get_escrow(from, escrow_id);
                    } catch (...) {
                    }

                    return result;
                });
            }

            std::vector<withdraw_route> plugin::api_impl::get_withdraw_routes(std::string account,
                                                                                    withdraw_route_type type) const {
                std::vector<withdraw_route> result;

                const auto &acc = database().get_account(account);

                if (type == outgoing || type == all) {
                    const auto &by_route = database().get_index<withdraw_vesting_route_index>().indices().get<
                            by_withdraw_route>();
                    auto route = by_route.lower_bound(acc.id);

                    while (route != by_route.end() && route->from_account == acc.id) {
                        withdraw_route r;
                        r.from_account = account;
                        r.to_account = database().get(route->to_account).name;
                        r.percent = route->percent;
                        r.auto_vest = route->auto_vest;

                        result.push_back(r);

                        ++route;
                    }
                }

                if (type == incoming || type == all) {
                    const auto &by_dest = database().get_index<withdraw_vesting_route_index>().indices().get<
                            by_destination>();
                    auto route = by_dest.lower_bound(acc.id);

                    while (route != by_dest.end() && route->to_account == acc.id) {
                        withdraw_route r;
                        r.from_account = database().get(route->from_account).name;
                        r.to_account = account;
                        r.percent = route->percent;
                        r.auto_vest = route->auto_vest;

                        result.push_back(r);

                        ++route;
                    }
                }

                return result;
            }


            DEFINE_API(plugin, get_withdraw_routes) {
                FC_ASSERT(args.args->size() == 1 || args.args->size() == 2, "Expected 1-2 arguments, was ${n}",
                          ("n", args.args->size()));
                auto account = args.args->at(0).as<string>();
                auto type = args.args->at(1).as<withdraw_route_type>();
                return my->database().with_read_lock([&]() {
                    return my->get_withdraw_routes(account, type);
                });
            }

            DEFINE_API(plugin, get_account_bandwidth) {
                CHECK_ARG_SIZE(2)
                auto account = args.args->at(0).as<string>();
                auto type = args.args->at(1).as<bandwidth_type>();
                optional<account_bandwidth_api_object> result;
                auto band = my->database().find<account_bandwidth_object, by_account_bandwidth_type>(
                        boost::make_tuple(account, type));
                if (band != nullptr) {
                    result = *band;
                }

                return result;
            }

            //////////////////////////////////////////////////////////////////////
            //                                                                  //
            // Witnesses                                                        //
            //                                                                  //
            //////////////////////////////////////////////////////////////////////

            DEFINE_API(plugin, get_witnesses) {
                CHECK_ARG_SIZE(1)
                auto witness_ids = args.args->at(0).as<vector<witness_object::id_type> >();
                return my->database().with_read_lock([&]() {
                    return my->get_witnesses(witness_ids);
                });
            }

            std::vector<optional<witness_api_object>> plugin::api_impl::get_witnesses(
                    const std::vector<witness_object::id_type> &witness_ids) const {
                std::vector<optional<witness_api_object>> result;
                result.reserve(witness_ids.size());
                std::transform(witness_ids.begin(), witness_ids.end(), std::back_inserter(result),
                               [this](witness_object::id_type id) -> optional<witness_api_object> {
                                   if (auto o = database().find(id)) {
                                       return *o;
                                   }
                                   return {};
                               });
                return result;
            }

            DEFINE_API(plugin, get_witness_by_account) {
                CHECK_ARG_SIZE(1)
                auto account_name = args.args->at(0).as<std::string>();
                return my->database().with_read_lock([&]() {
                    return my->get_witness_by_account(account_name);
                });
            }


            fc::optional<witness_api_object> plugin::api_impl::get_witness_by_account(
                    std::string account_name) const {
                const auto &idx = database().get_index<witness_index>().indices().get<by_name>();
                auto itr = idx.find(account_name);
                if (itr != idx.end()) {
                    return witness_api_object(*itr);
                }
                return {};
            }

            std::vector<witness_api_object> plugin::api_impl::get_witnesses_by_vote(std::string from,
                                                                                          uint32_t limit) const {
                //idump((from)(limit));
                FC_ASSERT(limit <= 100);

                std::vector<witness_api_object> result;
                result.reserve(limit);

                const auto &name_idx = database().get_index<witness_index>().indices().get<by_name>();
                const auto &vote_idx = database().get_index<witness_index>().indices().get<by_vote_name>();

                auto itr = vote_idx.begin();
                if (from.size()) {
                    auto nameitr = name_idx.find(from);
                    FC_ASSERT(nameitr != name_idx.end(), "invalid witness name ${n}", ("n", from));
                    itr = vote_idx.iterator_to(*nameitr);
                }

                while (itr != vote_idx.end() && result.size() < limit && itr->votes > 0) {
                    result.push_back(witness_api_object(*itr));
                    ++itr;
                }
                return result;

            }

            DEFINE_API(plugin, get_witnesses_by_vote) {
                CHECK_ARG_SIZE(2)
                auto from = args.args->at(0).as<std::string>();
                auto limit = args.args->at(1).as<uint32_t>();
                return my->database().with_read_lock([&]() {
                    return my->get_witnesses_by_vote(from, limit);
                });
            }


            DEFINE_API(plugin, lookup_witness_accounts) {
                CHECK_ARG_SIZE(2)
                auto lower_bound_name = args.args->at(0).as<std::string>();
                auto limit = args.args->at(1).as<uint32_t>();
                return my->database().with_read_lock([&]() {
                    return my->lookup_witness_accounts(lower_bound_name, limit);
                });
            }

            std::set<account_name_type> plugin::api_impl::lookup_witness_accounts(
                    const std::string &lower_bound_name, uint32_t limit) const {
                FC_ASSERT(limit <= 1000);
                const auto &witnesses_by_id = database().get_index<witness_index>().indices().get<by_id>();

                // get all the names and look them all up, sort them, then figure out what
                // records to return.  This could be optimized, but we expect the
                // number of witnesses to be few and the frequency of calls to be rare
                std::set<account_name_type> witnesses_by_account_name;
                for (const witness_api_object &witness : witnesses_by_id) {
                    if (witness.owner >= lower_bound_name) { // we can ignore anything below lower_bound_name
                        witnesses_by_account_name.insert(witness.owner);
                    }
                }

                auto end_iter = witnesses_by_account_name.begin();
                while (end_iter != witnesses_by_account_name.end() && limit--) {
                    ++end_iter;
                }
                witnesses_by_account_name.erase(end_iter, witnesses_by_account_name.end());
                return witnesses_by_account_name;
            }

            DEFINE_API(plugin, get_witness_count) {
                return my->database().with_read_lock([&]() {
                    return my->get_witness_count();
                });
            }

            uint64_t plugin::api_impl::get_witness_count() const {
                return database().get_index<witness_index>().indices().size();
            }

            //////////////////////////////////////////////////////////////////////
            //                                                                  //
            // Authority / validation                                           //
            //                                                                  //
            //////////////////////////////////////////////////////////////////////

            DEFINE_API(plugin, get_transaction_hex) {
                CHECK_ARG_SIZE(1)
                auto trx = args.args->at(0).as<signed_transaction>();
                return my->database().with_read_lock([&]() {
                    return my->get_transaction_hex(trx);
                });
            }

            std::string plugin::api_impl::get_transaction_hex(const signed_transaction &trx) const {
                return fc::to_hex(fc::raw::pack(trx));
            }

            DEFINE_API(plugin, get_required_signatures) {
                CHECK_ARG_SIZE(2)
                auto trx = args.args->at(0).as<signed_transaction>();
                auto available_keys = args.args->at(1).as<flat_set<public_key_type>>();
                return my->database().with_read_lock([&]() {
                    return my->get_required_signatures(trx, available_keys);
                });
            }

            std::set<public_key_type> plugin::api_impl::get_required_signatures(const signed_transaction &trx,
                                                                                      const flat_set<
                                                                                              public_key_type> &available_keys) const {
                //   wdump((trx)(available_keys));
                auto result = trx.get_required_signatures(STEEMIT_CHAIN_ID, available_keys,
                                                          [&](std::string account_name) {
                                                              return authority(database().get<account_authority_object,
                                                                      by_account>(account_name).active);
                                                          }, [&](std::string account_name) {
                            return authority(database().get<account_authority_object, by_account>(account_name).owner);
                        }, [&](std::string account_name) {
                            return authority(
                                    database().get<account_authority_object, by_account>(account_name).posting);
                        }, STEEMIT_MAX_SIG_CHECK_DEPTH);
                //   wdump((result));
                return result;
            }

            DEFINE_API(plugin, get_potential_signatures) {
                CHECK_ARG_SIZE(1)
                return my->database().with_read_lock([&]() {
                    return my->get_potential_signatures(args.args->at(0).as<signed_transaction>());
                });
            }

            std::set<public_key_type> plugin::api_impl::get_potential_signatures(
                    const signed_transaction &trx) const {
                //   wdump((trx));
                std::set<public_key_type> result;
                trx.get_required_signatures(STEEMIT_CHAIN_ID, flat_set<public_key_type>(),
                                            [&](account_name_type account_name) {
                                                const auto &auth = database().get<account_authority_object, by_account>(
                                                        account_name).active;
                                                for (const auto &k : auth.get_keys()) {
                                                    result.insert(k);
                                                }
                                                return authority(auth);
                                            }, [&](account_name_type account_name) {
                            const auto &auth = database().get<account_authority_object, by_account>(account_name).owner;
                            for (const auto &k : auth.get_keys()) {
                                result.insert(k);
                            }
                            return authority(auth);
                        }, [&](account_name_type account_name) {
                            const auto &auth = database().get<account_authority_object, by_account>(
                                    account_name).posting;
                            for (const auto &k : auth.get_keys()) {
                                result.insert(k);
                            }
                            return authority(auth);
                        }, STEEMIT_MAX_SIG_CHECK_DEPTH);

                //   wdump((result));
                return result;
            }

            DEFINE_API(plugin, verify_authority) {
                CHECK_ARG_SIZE(1)
                return my->database().with_read_lock([&]() {
                    return my->verify_authority(args.args->at(0).as<signed_transaction>());
                });
            }

            bool plugin::api_impl::verify_authority(const signed_transaction &trx) const {
                trx.verify_authority(STEEMIT_CHAIN_ID, [&](std::string account_name) {
                    return authority(database().get<account_authority_object, by_account>(account_name).active);
                }, [&](std::string account_name) {
                    return authority(database().get<account_authority_object, by_account>(account_name).owner);
                }, [&](std::string account_name) {
                    return authority(database().get<account_authority_object, by_account>(account_name).posting);
                }, STEEMIT_MAX_SIG_CHECK_DEPTH);
                return true;
            }

            DEFINE_API(plugin, verify_account_authority) {
                CHECK_ARG_SIZE(2)
                return my->database().with_read_lock([&]() {
                    return my->verify_account_authority(args.args->at(0).as<account_name_type>(),
                                                        args.args->at(1).as<flat_set<public_key_type> >());
                });
            }

            bool plugin::api_impl::verify_account_authority(const std::string &name,
                                                                  const flat_set<public_key_type> &keys) const {
                FC_ASSERT(name.size() > 0);
                auto account = database().find<account_object, by_name>(name);
                FC_ASSERT(account, "no such account");

                /// reuse trx.verify_authority by creating a dummy transfer
                signed_transaction trx;
                transfer_operation op;
                op.from = account->name;
                trx.operations.emplace_back(op);

                return verify_authority(trx);
            }

            DEFINE_API(plugin, get_conversion_requests) {
                CHECK_ARG_SIZE(1)
                auto account = args.args->at(0).as<std::string>();
                return my->database().with_read_lock([&]() {
                    const auto &idx = my->database().get_index<convert_request_index>().indices().get<by_owner>();
                    std::vector<convert_request_api_object> result;
                    auto itr = idx.lower_bound(account);
                    while (itr != idx.end() && itr->owner == account) {
                        result.emplace_back(*itr);
                        ++itr;
                    }
                    return result;
                });
            }


            discussion plugin::api_impl::get_content(std::string author, std::string permlink) const {
                const auto &by_permlink_idx = database().get_index<comment_index>().indices().get<by_permlink>();
                auto itr = by_permlink_idx.find(boost::make_tuple(author, permlink));
                if (itr != by_permlink_idx.end()) {
                    discussion result(*itr);
                    set_pending_payout(result);
                    result.active_votes = get_active_votes(author, permlink);
                    return result;
                }
                return discussion();
            }

            DEFINE_API(plugin, get_content) {
                CHECK_ARG_SIZE(2)
                auto author = args.args->at(0).as<account_name_type>();
                auto permlink = args.args->at(1).as<string>();
                return my->database().with_read_lock([&]() {
                    return my->get_content(author, permlink);
                });
            }

            std::vector<vote_state> plugin::api_impl::get_active_votes(std::string author,
                                                                             std::string permlink) const {
                std::vector<vote_state> result;
                const auto &comment = database().get_comment(author, permlink);
                const auto &idx = database().get_index<comment_vote_index>().indices().get<by_comment_voter>();
                comment_object::id_type cid(comment.id);
                auto itr = idx.lower_bound(cid);
                while (itr != idx.end() && itr->comment == cid) {
                    const auto &vo = database().get(itr->voter);
                    vote_state vstate;
                    vstate.voter = vo.name;
                    vstate.weight = itr->weight;
                    vstate.rshares = itr->rshares;
                    vstate.percent = itr->vote_percent;
                    vstate.time = itr->last_update;

                    result.emplace_back(vstate);
                    ++itr;
                }
                return result;
            }

            DEFINE_API(plugin, get_active_votes) {
                CHECK_ARG_SIZE(2)
                auto author = args.args->at(0).as<string>();
                auto permlink = args.args->at(1).as<string>();
                return my->database().with_read_lock([&]() {
                    return my->get_active_votes(author, permlink);
                });
            }

            DEFINE_API(plugin, get_account_votes) {
                CHECK_ARG_SIZE(1)
                account_name_type voter = args.args->at(0).as<account_name_type>();
                return my->database().with_read_lock([&]() {
                    std::vector<account_vote> result;

                    const auto &voter_acnt = my->database().get_account(voter);
                    const auto &idx = my->database().get_index<comment_vote_index>().indices().get<by_voter_comment>();

                    account_object::id_type aid(voter_acnt.id);
                    auto itr = idx.lower_bound(aid);
                    auto end = idx.upper_bound(aid);
                    while (itr != end) {
                        const auto &vo = my->database().get(itr->comment);
                        account_vote avote;
                        avote.authorperm = vo.author + "/" + to_string(vo.permlink);
                        avote.weight = itr->weight;
                        avote.rshares = itr->rshares;
                        avote.percent = itr->vote_percent;
                        avote.time = itr->last_update;
                        result.emplace_back(avote);
                        ++itr;
                    }
                    return result;
                });
            }

            boost::multiprecision::uint256_t to256(const fc::uint128_t &t) {
                boost::multiprecision::uint256_t result(t.high_bits());
                result <<= 65;
                result += t.low_bits();
                return result;
            }

            void plugin::api_impl::set_pending_payout(discussion &d) const {
                const auto &cidx = database().get_index<tags::tag_index>().indices().get<tags::by_comment>();
                auto itr = cidx.lower_bound(d.id);
                if (itr != cidx.end() && itr->comment == d.id) {
                    d.promoted = asset(itr->promoted_balance, SBD_SYMBOL);
                }

                const auto &props = database().get_dynamic_global_properties();
                const auto &hist = database().get_feed_history();
                asset pot = props.total_reward_fund_steem;
                if (!hist.current_median_history.is_null()) {
                    pot = pot * hist.current_median_history;
                }

                u256 total_r2 = to256(props.total_reward_shares2);

                if (props.total_reward_shares2 > 0) {
                    auto vshares = database().calculate_vshares(d.net_rshares.value > 0 ? d.net_rshares.value : 0);

                    //int64_t abs_net_rshares = llabs(d.net_rshares.value);

                    u256 r2 = to256(vshares); //to256(abs_net_rshares);
                    /*
          r2 *= r2;
          */
                    r2 *= pot.amount.value;
                    r2 /= total_r2;

                    u256 tpp = to256(d.children_rshares2);
                    tpp *= pot.amount.value;
                    tpp /= total_r2;

                    d.pending_payout_value = asset(static_cast<uint64_t>(r2), pot.symbol);
                    d.total_pending_payout_value = asset(static_cast<uint64_t>(tpp), pot.symbol);

                }

                if (d.parent_author != STEEMIT_ROOT_POST_PARENT) {
                    d.cashout_time = database().calculate_discussion_payout_time(database().get<comment_object>(d.id));
                }

                if (d.body.size() > 1024 * 128) {
                    d.body = "body pruned due to size";
                }
                if (d.parent_author.size() > 0 && d.body.size() > 1024 * 16) {
                    d.body = "comment pruned due to size";
                }

                set_url(d);
            }

            void plugin::api_impl::set_url(discussion &d) const {
                const comment_api_object root(database().get<comment_object, by_id>(d.root_comment));
                d.url = "/" + root.category + "/@" + root.author + "/" + root.permlink;
                d.root_title = root.title;
                if (root.id != d.id) {
                    d.url += "#@" + d.author + "/" + d.permlink;
                }
            }

            std::vector<discussion> plugin::api_impl::get_content_replies(std::string author,
                                                                                std::string permlink) const {
                account_name_type acc_name = account_name_type(author);
                const auto &by_permlink_idx = database().get_index<comment_index>().indices().get<by_parent>();
                auto itr = by_permlink_idx.find(boost::make_tuple(acc_name, permlink));
                std::vector<discussion> result;
                while (itr != by_permlink_idx.end() && itr->parent_author == author &&
                       to_string(itr->parent_permlink) == permlink) {
                    discussion push_discussion(*itr);
                    push_discussion.active_votes = get_active_votes(author, permlink);

                    result.emplace_back(*itr);
                    set_pending_payout(result.back());
                    ++itr;
                }
                return result;
            }

            DEFINE_API(plugin, get_content_replies) {
                CHECK_ARG_SIZE(2)
                auto author = args.args->at(0).as<string>();
                auto permlink = args.args->at(1).as<string>();
                return my->database().with_read_lock([&]() {
                    return my->get_content_replies(author, permlink);
                });
            }


            std::vector<discussion> plugin::api_impl::get_replies_by_last_update(
                    account_name_type start_parent_author, std::string start_permlink, uint32_t limit) const {
                std::vector<discussion> result;

#ifndef STEEMIT_BUILD_LOW_MEMORY
                FC_ASSERT(limit <= 100);
                const auto &last_update_idx = database().get_index<comment_index>().indices().get<by_last_update>();
                auto itr = last_update_idx.begin();
                const account_name_type *parent_author = &start_parent_author;

                if (start_permlink.size()) {
                    const auto &comment = database().get_comment(start_parent_author, start_permlink);
                    itr = last_update_idx.iterator_to(comment);
                    parent_author = &comment.parent_author;
                } else if (start_parent_author.size()) {
                    itr = last_update_idx.lower_bound(start_parent_author);
                }

                result.reserve(limit);

                while (itr != last_update_idx.end() && result.size() < limit && itr->parent_author == *parent_author) {
                    result.emplace_back(*itr);
                    set_pending_payout(result.back());
                    result.back().active_votes = get_active_votes(itr->author, to_string(itr->permlink));
                    ++itr;
                }

#endif
                return result;

            }


            /**
             *  This method can be used to fetch replies to an account.
             *
             *  The first call should be (account_to_retrieve replies, "", limit)
             *  Subsequent calls should be (last_author, last_permlink, limit)
             */
            DEFINE_API(plugin, get_replies_by_last_update) {
                CHECK_ARG_SIZE(3)
                auto start_parent_author = args.args->at(0).as<account_name_type>();
                auto start_permlink = args.args->at(1).as<string>();
                auto limit = args.args->at(2).as<uint32_t>();
                return my->database().with_read_lock([&]() {
                    return my->get_replies_by_last_update(start_parent_author, start_permlink, limit);
                });
            }

            std::map<uint32_t, applied_operation> plugin::api_impl::get_account_history(std::string account,
                                                                                              uint64_t from,
                                                                                              uint32_t limit) const {
                FC_ASSERT(limit <= 10000, "Limit of ${l} is greater than maxmimum allowed", ("l", limit));
                FC_ASSERT(from >= limit, "From must be greater than limit");
                //   idump((account)(from)(limit));
                const auto &idx = database().get_index<account_history_index>().indices().get<by_account>();
                auto itr = idx.lower_bound(boost::make_tuple(account, from));
                //   if( itr != idx.end() ) idump((*itr));
                auto end = idx.upper_bound(
                        boost::make_tuple(account, std::max(int64_t(0), int64_t(itr->sequence) - limit)));
                //   if( end != idx.end() ) idump((*end));

                std::map<uint32_t, applied_operation> result;
                while (itr != end) {
                    result[itr->sequence] = database().get(itr->op);
                    ++itr;
                }
                return result;
            }


            DEFINE_API(plugin, get_account_history) {
                CHECK_ARG_SIZE(3)
                auto account = args.args->at(0).as<std::string>();
                auto from = args.args->at(1).as<uint64_t>();
                auto limit = args.args->at(2).as<uint32_t>();

                return my->database().with_read_lock([&]() {
                    return my->get_account_history(account, from, limit);
                });
            }


            std::vector<std::pair<std::string, uint32_t>> plugin::api_impl::get_tags_used_by_author(
                    const std::string &author) const {
                const auto *acnt = database().find_account(author);
                FC_ASSERT(acnt != nullptr);
                const auto &tidx = database().get_index<tags::author_tag_stats_index>().indices().get<
                        tags::by_author_posts_tag>();
                auto itr = tidx.lower_bound(boost::make_tuple(acnt->id, 0));
                std::vector<std::pair<std::string, uint32_t>> result;
                while (itr != tidx.end() && itr->author == acnt->id && result.size() < 1000) {
                    if (!fc::is_utf8(itr->tag)) {
                        result.emplace_back(std::make_pair(fc::prune_invalid_utf8(itr->tag), itr->total_posts));
                    } else {
                        result.emplace_back(std::make_pair(itr->tag, itr->total_posts));
                    }
                    ++itr;
                }
                return result;
            }

            DEFINE_API(plugin, get_tags_used_by_author) {
                CHECK_ARG_SIZE(1)
                auto author = args.args->at(0).as<string>();
                return my->database().with_read_lock([&]() {
                    return my->get_tags_used_by_author(author);
                });
            }

            std::vector<tag_api_object> plugin::api_impl::get_trending_tags(std::string after,
                                                                                  uint32_t limit) const {
                limit = std::min(limit, uint32_t(1000));
                std::vector<tag_api_object> result;
                result.reserve(limit);

                const auto &nidx = database().get_index<tags::tag_stats_index>().indices().get<tags::by_tag>();

                const auto &ridx = database().get_index<tags::tag_stats_index>().indices().get<tags::by_trending>();
                auto itr = ridx.begin();
                if (after != "" && nidx.size()) {
                    auto nitr = nidx.lower_bound(after);
                    if (nitr == nidx.end()) {
                        itr = ridx.end();
                    } else {
                        itr = ridx.iterator_to(*nitr);
                    }
                }

                while (itr != ridx.end() && result.size() < limit) {
                    tag_api_object push_object = tag_api_object(*itr);

                    if (!fc::is_utf8(push_object.name)) {
                        push_object.name = fc::prune_invalid_utf8(push_object.name);
                    }

                    result.emplace_back(push_object);
                    ++itr;
                }
                return result;
            }


            DEFINE_API(plugin, get_trending_tags) {
                CHECK_ARG_SIZE(2)
                auto after = args.args->at(0).as<string>();
                auto limit = args.args->at(1).as<uint32_t>();

                return my->database().with_read_lock([&]() {
                    return my->get_trending_tags(after, limit);
                });
            }

            discussion plugin::api_impl::get_discussion(comment_object::id_type id,
                                                              uint32_t truncate_body) const {
                discussion d = database().get(id);
                set_url(d);
                set_pending_payout(d);
                d.active_votes = get_active_votes(d.author, d.permlink);
                d.body_length = static_cast<uint32_t>(d.body.size());
                if (truncate_body) {
                    d.body = d.body.substr(0, truncate_body);

                    if (!fc::is_utf8(d.title)) {
                        d.title = fc::prune_invalid_utf8(d.title);
                    }

                    if (!fc::is_utf8(d.body)) {
                        d.body = fc::prune_invalid_utf8(d.body);
                    }

                    if (!fc::is_utf8(d.category)) {
                        d.category = fc::prune_invalid_utf8(d.category);
                    }

                    if (!fc::is_utf8(d.json_metadata)) {
                        d.json_metadata = fc::prune_invalid_utf8(d.json_metadata);
                    }

                }
                return d;
            }

            template<typename Object, typename DatabaseIndex, typename DiscussionIndex, typename CommentIndex,
                    typename Index, typename StartItr>
            std::multimap<Object, discussion, DiscussionIndex> plugin::api_impl::get_discussions(
                    const discussion_query &query, const std::string &tag, comment_object::id_type parent,
                    const Index &tidx, StartItr tidx_itr,
                    const std::function<bool(const comment_api_object &c)> &filter,
                    const std::function<bool(const comment_api_object &c)> &exit,
                    const std::function<bool(const Object &)> &tag_exit, bool ignore_parent) const {
                //   idump((query));

                const auto &cidx = database().get_index<DatabaseIndex>().indices().template get<CommentIndex>();
                comment_object::id_type start;


                if (query.start_author && query.start_permlink) {
                    start = database().get_comment(*query.start_author, *query.start_permlink).id;
                    auto itr = cidx.find(start);
                    while (itr != cidx.end() && itr->comment == start) {
                        if (itr->name == tag) {
                            tidx_itr = tidx.iterator_to(*itr);
                            break;
                        }
                        ++itr;
                    }
                }
                std::multimap<Object, discussion, DiscussionIndex> result;
                uint32_t count = query.limit;
                uint64_t filter_count = 0;
                uint64_t exc_count = 0;
                while (count > 0 && tidx_itr != tidx.end()) {
                    if (tidx_itr->name != tag || (!ignore_parent && tidx_itr->parent != parent)) {
                        break;
                    }

                    try {
                        discussion insert_discussion = get_discussion(tidx_itr->comment, query.truncate_body);
                        insert_discussion.promoted = asset(tidx_itr->promoted_balance, SBD_SYMBOL);

                        if (filter(insert_discussion)) {
                            ++filter_count;
                        } else if (exit(insert_discussion) || tag_exit(*tidx_itr)) {
                            break;
                        } else {
                            result.insert({*tidx_itr, insert_discussion});
                            --count;
                        }
                    } catch (const fc::exception &e) {
                        ++exc_count;
                        edump((e.to_detail_string()));
                    }

                    ++tidx_itr;
                }
                return result;
            }

/*
            template<typename FirstCompare, typename SecondCompare>
            std::vector<discussion> merge(std::multimap<tags::tag_object, discussion, FirstCompare> &result1,
                                          std::multimap<languages::language_object, discussion,
                                                  SecondCompare> &result2) {
                //TODO:std::set_intersection(
                std::vector<discussion> discussions;
                if (!result2.empty()) {
                    std::multimap<comment_object::id_type, discussion> tmp;

                    for (auto &&i:result1) {
                        tmp.emplace(i.second.id, std::move(i.second));
                    }

                    for (auto &&i:result2) {
                        if (tmp.count(i.second.id)) {
                            discussions.push_back(std::move(i.second));
                        }
                    }

                    return discussions;
                }

                discussions.reserve(result1.size());
                for (auto &&iterator : result1) {
                    discussions.push_back(std::move(iterator.second));
                }

                return discussions;
            }

            DEFINE_API(plugin, get_discussions_by_trending) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return my->database().with_read_lock([&]() {
                    query.validate();
                    auto parent = my->get_parent(query);

                    std::multimap<tags::tag_object, discussion, tags::by_parent_trending> map_result = my->select<
                            tags::tag_object, tags::tag_index, tags::by_parent_trending, tags::by_comment>(
                            query.select_tags, query, parent, std::bind(tags_filter, query, std::placeholders::_1,
                                                                        [&](const comment_api_object &c) -> bool {
                                                                            return c.net_rshares <= 0;
                                                                        }), [&](const comment_api_object &c) -> bool {
                                return false;
                            }, [&](const tags::tag_object &) -> bool {
                                return false;
                            }, parent, std::numeric_limits<double>::max());

                    std::multimap<languages::language_object, discussion,
                            languages::by_parent_trending> map_result_ = my->select<languages::language_object,
                            languages::language_index, languages::by_parent_trending, languages::by_comment>(
                            query.select_languages, query, parent,
                            std::bind(languages_filter, query, std::placeholders::_1,
                                      [&](const comment_api_object &c) -> bool {
                                          return c.net_rshares <= 0;
                                      }), [&](const comment_api_object &c) -> bool {
                                return false;
                            }, [&](const languages::language_object &) -> bool {
                                return false;
                            }, parent, std::numeric_limits<double>::max());


                    std::vector<discussion> return_result = merge(map_result, map_result_);

                    return return_result;
                });
            }

            vector<discussion> plugin::api_impl::get_post_discussions_by_payout(
                    const discussion_query &query) const {
                query.validate();
                auto parent = comment_object::id_type();

                std::multimap<tags::tag_object, discussion, tags::by_parent_promoted> map_result = select <
                tags::tag_object, tags::tag_index, tags::by_parent_promoted, tags::by_comment >
                                                                             (query.select_tags, query, parent, std::bind(
                                                                                     tags_filter, query,
                                                                                     std::placeholders::_1,
                                                                                     [&](const comment_api_object &c) -> bool {
                                                                                         return c.net_rshares <= 0;
                                                                                     }), [&](
                                                                                     const comment_api_object &c) -> bool {
                                                                                 return false;
                                                                             }, [&](const tags::tag_object &t) {
                                                                                 return false;
                                                                             }, true);

                std::multimap<languages::language_object, discussion,
                        languages::by_parent_promoted> map_result_language =
                        select < languages::language_object, languages::language_index, languages::by_parent_promoted,
                languages::by_comment >
                (query.select_tags, query, parent, std::bind(languages_filter, query, std::placeholders::_1,
                                                             [&](const comment_api_object &c) -> bool {
                                                                 return c.net_rshares <= 0;
                                                             }), [&](const comment_api_object &c) -> bool {
                    return false;
                }, [&](const languages::language_object &t) {
                    return false;
                }, true);

                std::vector<discussion> return_result = merge(map_result, map_result_language);

                return return_result;
            }


            DEFINE_API(plugin, get_post_discussions_by_payout) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return my->database().with_read_lock([&]() {
                    return my->get_post_discussions_by_payout(query);
                });
            }

            vector<discussion> plugin::api_impl::get_comment_discussions_by_payout(
                    const discussion_query &query) const {
                query.validate();
                auto parent = comment_object::id_type(1);
                std::multimap<tags::tag_object, discussion, tags::by_reward_fund_net_rshares> map_result = select <
                tags::tag_object, tags::tag_index, tags::by_reward_fund_net_rshares, tags::by_comment >
                                                                                     (query.select_tags, query, parent, std::bind(
                                                                                             tags_filter, query,
                                                                                             std::placeholders::_1,
                                                                                             [&](const comment_api_object &c) -> bool {
                                                                                                 return c.net_rshares <=
                                                                                                        0;
                                                                                             }), [&](
                                                                                             const comment_api_object &c) -> bool {
                                                                                         return false;
                                                                                     }, [&](const tags::tag_object &t) {
                                                                                         return false;
                                                                                     }, false);

                std::multimap<languages::language_object, discussion,
                        languages::by_reward_fund_net_rshares> map_result_language = select <
                                                                                     languages::language_object, languages::language_index, languages::by_reward_fund_net_rshares,
                languages::by_comment >
                (query.select_tags, query, parent, std::bind(languages_filter, query, std::placeholders::_1,
                                                             [&](const comment_api_object &c) -> bool {
                                                                 return c.net_rshares <= 0;
                                                             }), [&](const comment_api_object &c) -> bool {
                    return false;
                }, [&](const languages::language_object &t) {
                    return false;
                }, false);

                std::vector<discussion> return_result = merge(map_result, map_result_language);


                return return_result;

            }


            DEFINE_API(plugin, get_comment_discussions_by_payout) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return my->database().with_read_lock([&]() {
                    return my->get_comment_discussions_by_payout(query);
                });
            }

            std::vector<discussion> plugin::api_impl::get_discussions_by_promoted(
                    const discussion_query &query) const {
                query.validate();
                auto parent = get_parent(query);

                std::multimap<tags::tag_object, discussion, tags::by_parent_promoted> map_result = select <
                tags::tag_object, tags::tag_index, tags::by_parent_promoted, tags::by_comment >
                                                                             (query.select_tags, query, parent, std::bind(
                                                                                     tags_filter, query,
                                                                                     std::placeholders::_1,
                                                                                     [&](const comment_api_object &c) -> bool {
                                                                                         return c.children_rshares2 <=
                                                                                                0;
                                                                                     }), [&](
                                                                                     const comment_api_object &c) -> bool {
                                                                                 return false;
                                                                             }, [&](const tags::tag_object &) -> bool {
                                                                                 return false;
                                                                             }, parent, share_type(
                                                                                     STEEMIT_MAX_SHARE_SUPPLY));


                std::multimap<languages::language_object, discussion,
                        languages::by_parent_promoted> map_result_language =
                        select < languages::language_object, languages::language_index, languages::by_parent_promoted,
                languages::by_comment >
                (query.select_tags, query, parent, std::bind(languages_filter, query, std::placeholders::_1,
                                                             [&](const comment_api_object &c) -> bool {
                                                                 return c.children_rshares2 <= 0;
                                                             }), [&](const comment_api_object &c) -> bool {
                    return false;
                }, [&](const languages::language_object &) -> bool {
                    return false;
                }, parent, share_type(STEEMIT_MAX_SHARE_SUPPLY));


                std::vector<discussion> return_result = merge(map_result, map_result_language);


                return return_result;
            }


            DEFINE_API(plugin, get_discussions_by_promoted) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return my->database().with_read_lock([&]() {
                    return my->get_discussions_by_promoted(query);
                });
            }


            std::vector<discussion> plugin::api_impl::get_discussions_by_created(
                    const discussion_query &query) const {
                query.validate();
                auto parent = get_parent(query);

                std::multimap<tags::tag_object, discussion, tags::by_parent_created> map_result = select <
                tags::tag_object, tags::tag_index, tags::by_parent_created, tags::by_comment >
                                                                            (query.select_tags, query, parent, std::bind(
                                                                                    tags_filter, query,
                                                                                    std::placeholders::_1,
                                                                                    [&](const comment_api_object &c) -> bool {
                                                                                        return false;
                                                                                    }), [&](
                                                                                    const comment_api_object &c) -> bool {
                                                                                return false;
                                                                            }, [&](const tags::tag_object &) -> bool {
                                                                                return false;
                                                                            }, parent, fc::time_point_sec::maximum());

                std::multimap<languages::language_object, discussion,
                        languages::by_parent_created> map_result_language =
                        select < languages::language_object, languages::language_index, languages::by_parent_created,
                languages::by_comment >
                (query.select_tags, query, parent, std::bind(languages_filter, query, std::placeholders::_1,
                                                             [&](const comment_api_object &c) -> bool {
                                                                 return false;
                                                             }), [&](const comment_api_object &c) -> bool {
                    return false;
                }, [&](const languages::language_object &) -> bool {
                    return false;
                }, parent, fc::time_point_sec::maximum());

                std::vector<discussion> return_result = merge(map_result, map_result_language);

                return return_result;
            }

            DEFINE_API(plugin, get_discussions_by_created) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return my->database().with_read_lock([&]() {
                    return my->get_discussions_by_created(query);
                });
            }

            std::vector<discussion> plugin::api_impl::get_discussions_by_active(
                    const discussion_query &query) const {

                query.validate();
                auto parent = get_parent(query);


                std::multimap<tags::tag_object, discussion, tags::by_parent_active> map_result = select <
                tags::tag_object, tags::tag_index, tags::by_parent_active, tags::by_comment >
                                                                           (query.select_tags, query, parent, std::bind(
                                                                                   tags_filter, query,
                                                                                   std::placeholders::_1,
                                                                                   [&](const comment_api_object &c) -> bool {
                                                                                       return false;
                                                                                   }), [&](
                                                                                   const comment_api_object &c) -> bool {
                                                                               return false;
                                                                           }, [&](const tags::tag_object &) -> bool {
                                                                               return false;
                                                                           }, parent, fc::time_point_sec::maximum());

                std::multimap<languages::language_object, discussion, languages::by_parent_active> map_result_language =
                        select < languages::language_object, languages::language_index, languages::by_parent_active,
                languages::by_comment >
                (query.select_tags, query, parent, std::bind(languages_filter, query, std::placeholders::_1,
                                                             [&](const comment_api_object &c) -> bool {
                                                                 return false;
                                                             }), [&](const comment_api_object &c) -> bool {
                    return false;
                }, [&](const languages::language_object &) -> bool {
                    return false;
                }, parent, fc::time_point_sec::maximum());

                std::vector<discussion> return_result = merge(map_result, map_result_language);

                return return_result;

            }

            DEFINE_API(plugin, get_discussions_by_active) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return my->database().with_read_lock([&]() {
                    return my->get_discussions_by_active(query);
                });
            }


            std::vector<discussion> plugin::api_impl::get_discussions_by_cashout(
                    const discussion_query &query) const {
                query.validate();
                auto parent = get_parent(query);
                std::multimap<tags::tag_object, discussion, tags::by_cashout> map_result = select <
                tags::tag_object, tags::tag_index, tags::by_cashout, tags::by_comment >
                                                                     (query.select_tags, query, parent, std::bind(
                                                                             tags_filter, query, std::placeholders::_1,
                                                                             [&](const comment_api_object &c) -> bool {
                                                                                 return c.children_rshares2 <= 0;
                                                                             }), [&](
                                                                             const comment_api_object &c) -> bool {
                                                                         return false;
                                                                     }, [&](const tags::tag_object &) -> bool {
                                                                         return false;
                                                                     }, fc::time_point::now() - fc::minutes(60));

                std::multimap<languages::language_object, discussion, languages::by_cashout> map_result_language =
                        select <
                languages::language_object, languages::language_index, languages::by_cashout, languages::by_comment >
                                                                                              (query.select_tags, query, parent, std::bind(
                                                                                                      languages_filter,
                                                                                                      query,
                                                                                                      std::placeholders::_1,
                                                                                                      [&](const comment_api_object &c) -> bool {
                                                                                                          return c.children_rshares2 <=
                                                                                                                 0;
                                                                                                      }), [&](
                                                                                                      const comment_api_object &c) -> bool {
                                                                                                  return false;
                                                                                              }, [&](const languages::language_object &) -> bool {
                                                                                                  return false;
                                                                                              }, fc::time_point::now() -
                                                                                                 fc::minutes(60));


                std::vector<discussion> return_result = merge(map_result, map_result_language);
                return return_result;
            }


            DEFINE_API(plugin, get_discussions_by_cashout) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return my->database().with_read_lock([&]() {
                    return my->get_discussions_by_cashout(query);
                });
            }

            DEFINE_API(plugin, get_discussions_by_payout) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return my->database().with_read_lock([&]() {
                    query.validate();
                    auto parent = my->get_parent(query);

                    std::multimap<tags::tag_object, discussion, tags::by_net_rshares> map_result = my->select<
                            tags::tag_object, tags::tag_index, tags::by_net_rshares, tags::by_comment>(
                            query.select_tags, query, parent, std::bind(tags_filter, query, std::placeholders::_1,
                                                                        [&](const comment_api_object &c) -> bool {
                                                                            return c.children_rshares2 <= 0;
                                                                        }), [&](const comment_api_object &c) -> bool {
                                return false;
                            }, [&](const tags::tag_object &) -> bool {
                                return false;
                            });

                    std::multimap<languages::language_object, discussion,
                            languages::by_net_rshares> map_result_language = my->select<languages::language_object,
                            languages::language_index, languages::by_net_rshares, languages::by_comment>(
                            query.select_tags, query, parent, std::bind(languages_filter, query, std::placeholders::_1,
                                                                        [&](const comment_api_object &c) -> bool {
                                                                            return c.children_rshares2 <= 0;
                                                                        }), [&](const comment_api_object &c) -> bool {
                                return false;
                            }, [&](const languages::language_object &) -> bool {
                                return false;
                            });

                    std::vector<discussion> return_result = merge(map_result, map_result_language);

                    return return_result;
                });
            }

            std::vector<discussion> plugin::api_impl::get_discussions_by_votes(
                    const discussion_query &query) const {
                query.validate();
                auto parent = get_parent(query);

                std::multimap<tags::tag_object, discussion, tags::by_parent_net_votes> map_result = select <
                tags::tag_object, tags::tag_index, tags::by_parent_net_votes, tags::by_comment >
                                                                              (query.select_tags, query, parent, std::bind(
                                                                                      tags_filter, query,
                                                                                      std::placeholders::_1,
                                                                                      [&](const comment_api_object &c) -> bool {
                                                                                          return false;
                                                                                      }), [&](
                                                                                      const comment_api_object &c) -> bool {
                                                                                  return false;
                                                                              }, [&](const tags::tag_object &) -> bool {
                                                                                  return false;
                                                                              }, parent, std::numeric_limits<
                                                                                      int32_t>::max());

                std::multimap<languages::language_object, discussion,
                        languages::by_parent_net_votes> map_result_language =
                        select < languages::language_object, languages::language_index, languages::by_parent_net_votes,
                languages::by_comment >
                (query.select_tags, query, parent, std::bind(languages_filter, query, std::placeholders::_1,
                                                             [&](const comment_api_object &c) -> bool {
                                                                 return false;
                                                             }), [&](const comment_api_object &c) -> bool {
                    return false;
                }, [&](const languages::language_object &) -> bool {
                    return false;
                }, parent, std::numeric_limits<int32_t>::max());

                std::vector<discussion> return_result = merge(map_result, map_result_language);

                return return_result;
            }


            DEFINE_API(plugin, get_discussions_by_votes) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return my->database().with_read_lock([&]() {
                    return my->get_discussions_by_votes(query);
                });
            }


            std::vector<discussion> plugin::api_impl::get_discussions_by_children(
                    const discussion_query &query) const {

                query.validate();
                auto parent = get_parent(query);

                std::multimap<tags::tag_object, discussion, tags::by_parent_children> map_result =

                select < tags::tag_object, tags::tag_index, tags::by_parent_children, tags::by_comment >
                                                                                      (query.select_tags, query, parent, std::bind(
                                                                                              tags_filter, query,
                                                                                              std::placeholders::_1,
                                                                                              [&](const comment_api_object &c) -> bool {
                                                                                                  return false;
                                                                                              }), [&](
                                                                                              const comment_api_object &c) -> bool {
                                                                                          return false;
                                                                                      }, [&](const tags::tag_object &) -> bool {
                                                                                          return false;
                                                                                      }, parent, std::numeric_limits<
                                                                                              int32_t>::max());

                std::multimap<languages::language_object, discussion,
                        languages::by_parent_children> map_result_language =
                        select < languages::language_object, languages::language_index, languages::by_parent_children,
                languages::by_comment >
                (query.select_tags, query, parent, std::bind(languages_filter, query, std::placeholders::_1,
                                                             [&](const comment_api_object &c) -> bool {
                                                                 return false;
                                                             }), [&](const comment_api_object &c) -> bool {
                    return false;
                }, [&](const languages::language_object &) -> bool {
                    return false;
                }, parent, std::numeric_limits<int32_t>::max());

                std::vector<discussion> return_result = merge(map_result, map_result_language);

                return return_result;

            }

            DEFINE_API(plugin, get_discussions_by_children) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return my->database().with_read_lock([&]() {
                    return my->get_discussions_by_children(query);
                });
            }

            std::vector<discussion> plugin::api_impl::get_discussions_by_hot(
                    const discussion_query &query) const {

                query.validate();
                auto parent = get_parent(query);

                std::multimap<tags::tag_object, discussion, tags::by_parent_hot> map_result = select <
                tags::tag_object, tags::tag_index, tags::by_parent_hot, tags::by_comment >
                                                                        (query.select_tags, query, parent, std::bind(
                                                                                tags_filter, query,
                                                                                std::placeholders::_1,
                                                                                [&](const comment_api_object &c) -> bool {
                                                                                    return c.net_rshares <= 0;
                                                                                }), [&](
                                                                                const comment_api_object &c) -> bool {
                                                                            return false;
                                                                        }, [&](const tags::tag_object &) -> bool {
                                                                            return false;
                                                                        }, parent, std::numeric_limits<double>::max());

                std::multimap<languages::language_object, discussion, languages::by_parent_hot> map_result_language =
                        select <
                languages::language_object, languages::language_index, languages::by_parent_hot, languages::by_comment >
                                                                                                 (query.select_tags, query, parent, std::bind(
                                                                                                         languages_filter,
                                                                                                         query,
                                                                                                         std::placeholders::_1,
                                                                                                         [&](const comment_api_object &c) -> bool {
                                                                                                             return c.net_rshares <=
                                                                                                                    0;
                                                                                                         }), [&](
                                                                                                         const comment_api_object &c) -> bool {
                                                                                                     return false;
                                                                                                 }, [&](const languages::language_object &) -> bool {
                                                                                                     return false;
                                                                                                 }, parent, std::numeric_limits<
                                                                                                         double>::max());

                std::vector<discussion> return_result = merge(map_result, map_result_language);

                return return_result;

            }

            DEFINE_API(plugin, get_discussions_by_hot) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return my->database().with_read_lock([&]() {
                    return my->get_discussions_by_hot(query);
                });
            }
*/
            std::vector<discussion> merge(std::vector<discussion> &result1, std::vector<discussion> &result2) {
                //TODO:std::set_intersection(
                if (!result2.empty()) {
                    std::vector<discussion> discussions;
                    std::multimap<comment_object::id_type, discussion> tmp;

                    for (auto &&i:result1) {
                        tmp.emplace(i.id, std::move(i));
                    }


                    for (auto &&i:result2) {
                        if (tmp.count(i.id)) {
                            discussions.push_back(std::move(i));
                        }
                    }

                    return discussions;
                }

                std::vector<discussion> discussions(result1.begin(), result1.end());
                return discussions;
            }
/*
            template<typename DatabaseIndex, typename DiscussionIndex>
            std::vector<discussion> plugin::feed(const std::set<string> &select_set, const discussion_query &query,
                                              const std::string &start_author,
                                              const std::string &start_permlink) const {
                std::vector<discussion> result;

                for (const auto &iterator : query.select_authors) {
                    const auto &account = my->database().get_account(iterator);

                    const auto &tag_idx = my->database().get_index<DatabaseIndex>().indices().template get<
                            DiscussionIndex>();

                    const auto &c_idx = my->database().get_index<follow::feed_index>().indices().get<
                            follow::by_comment>();
                    const auto &f_idx = my->database().get_index<follow::feed_index>().indices().get<follow::by_feed>();
                    auto feed_itr = f_idx.lower_bound(account.name);

                    if (start_author.size() || start_permlink.size()) {
                        auto start_c = c_idx.find(
                                boost::make_tuple(my->database().get_comment(start_author, start_permlink).id,
                                                  account.name));
                        FC_ASSERT(start_c != c_idx.end(), "Comment is not in account's feed");
                        feed_itr = f_idx.iterator_to(*start_c);
                    }

                    while (result.size() < query.limit && feed_itr != f_idx.end()) {
                        if (feed_itr->account != account.name) {
                            break;
                        }
                        try {
                            if (!select_set.empty()) {
                                auto tag_itr = tag_idx.lower_bound(feed_itr->comment);

                                bool found = false;
                                while (tag_itr != tag_idx.end() && tag_itr->comment == feed_itr->comment) {
                                    if (select_set.find(tag_itr->name) != select_set.end()) {
                                        found = true;
                                        break;
                                    }
                                    ++tag_itr;
                                }
                                if (!found) {
                                    ++feed_itr;
                                    continue;
                                }
                            }

                            result.push_back(my->get_discussion(feed_itr->comment));
                            if (feed_itr->first_reblogged_by != account_name_type()) {
                                result.back().reblogged_by = std::vector<account_name_type>(
                                        feed_itr->reblogged_by.begin(), feed_itr->reblogged_by.end());
                                result.back().first_reblogged_by = feed_itr->first_reblogged_by;
                                result.back().first_reblogged_on = feed_itr->first_reblogged_on;
                            }
                        } catch (const fc::exception &e) {
                            edump((e.to_detail_string()));
                        }

                        ++feed_itr;
                    }
                }

                return result;
            }

            DEFINE_API(plugin, get_discussions_by_feed) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return my->database().with_read_lock([&]() {
                    query.validate();
                    FC_ASSERT(my->_follow_api, "Node is not running the follow plugin");
                    FC_ASSERT(query.select_authors.size(), "No such author to select feed from");

                    std::string start_author = query.start_author ? *(query.start_author) : "";
                    std::string start_permlink = query.start_permlink ? *(query.start_permlink) : "";

                    std::vector<discussion> tags_ = feed<tags::tag_index, tags::by_comment>(query.select_tags, query,
                                                                                            start_author,
                                                                                            start_permlink);

                    std::vector<discussion> languages_ = feed<languages::language_index, languages::by_comment>(
                            query.select_languages, query, start_author, start_permlink);

                    std::vector<discussion> result = merge(tags_, languages_);

                    return result;
                });
            }

            template<typename DatabaseIndex, typename DiscussionIndex>
            std::vector<discussion> plugin::blog(const std::set<string> &select_set, const discussion_query &query,
                                              const std::string &start_author,
                                              const std::string &start_permlink) const {
                std::vector<discussion> result;
                for (const auto &iterator : query.select_authors) {

                    const auto &account = my->database().get_account(iterator);

                    const auto &tag_idx = my->database().get_index<DatabaseIndex>().indices().template get<
                            DiscussionIndex>();

                    const auto &c_idx = my->database().get_index<follow::blog_index>().indices().get<
                            follow::by_comment>();
                    const auto &b_idx = my->database().get_index<follow::blog_index>().indices().get<follow::by_blog>();
                    auto blog_itr = b_idx.lower_bound(account.name);

                    if (start_author.size() || start_permlink.size()) {
                        auto start_c = c_idx.find(
                                boost::make_tuple(my->database().get_comment(start_author, start_permlink).id,
                                                  account.name));
                        FC_ASSERT(start_c != c_idx.end(), "Comment is not in account's blog");
                        blog_itr = b_idx.iterator_to(*start_c);
                    }

                    while (result.size() < query.limit && blog_itr != b_idx.end()) {
                        if (blog_itr->account != account.name) {
                            break;
                        }
                        try {
                            if (select_set.size()) {
                                auto tag_itr = tag_idx.lower_bound(blog_itr->comment);

                                bool found = false;
                                while (tag_itr != tag_idx.end() && tag_itr->comment == blog_itr->comment) {
                                    if (select_set.find(tag_itr->name) != select_set.end()) {
                                        found = true;
                                        break;
                                    }
                                    ++tag_itr;
                                }
                                if (!found) {
                                    ++blog_itr;
                                    continue;
                                }
                            }

                            result.push_back(my->get_discussion(blog_itr->comment, query.truncate_body));
                            if (blog_itr->reblogged_on > time_point_sec()) {
                                result.back().first_reblogged_on = blog_itr->reblogged_on;
                            }
                        } catch (const fc::exception &e) {
                            edump((e.to_detail_string()));
                        }

                        ++blog_itr;
                    }
                }
                return result;
            }

            DEFINE_API(plugin, get_discussions_by_blog) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return my->database().with_read_lock([&]() {
                    query.validate();
                    FC_ASSERT(my->_follow_api, "Node is not running the follow plugin");
                    FC_ASSERT(query.select_authors.size(), "No such author to select feed from");

                    auto start_author = query.start_author ? *(query.start_author) : "";
                    auto start_permlink = query.start_permlink ? *(query.start_permlink) : "";

                    std::vector<discussion> tags_ = blog<tags::tag_index, tags::by_comment>(query.select_tags, query,
                                                                                            start_author,
                                                                                            start_permlink);

                    std::vector<discussion> languages_ = blog<languages::language_index, languages::by_comment>(
                            query.select_languages, query, start_author, start_permlink);

                    std::vector<discussion> result = merge(tags_, languages_);

                    return result;
                });
            }

            DEFINE_API(plugin, get_discussions_by_comments) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return my->database().with_read_lock([&]() {
                    std::vector<discussion> result;
#ifndef STEEMIT_BUILD_LOW_MEMORY
                    query.validate();
                    FC_ASSERT(query.start_author, "Must get comments for a specific author");
                    auto start_author = *(query.start_author);
                    auto start_permlink = query.start_permlink ? *(query.start_permlink) : "";

                    const auto &c_idx = my->database().get_index<comment_index>().indices().get<by_permlink>();
                    const auto &t_idx = my->database().get_index<comment_index>().indices().get<
                            by_author_last_update>();
                    auto comment_itr = t_idx.lower_bound(start_author);

                    if (start_permlink.size()) {
                        auto start_c = c_idx.find(boost::make_tuple(start_author, start_permlink));
                        FC_ASSERT(start_c != c_idx.end(), "Comment is not in account's comments");
                        comment_itr = t_idx.iterator_to(*start_c);
                    }

                    result.reserve(query.limit);

                    while (result.size() < query.limit && comment_itr != t_idx.end()) {
                        if (comment_itr->author != start_author) {
                            break;
                        }
                        if (comment_itr->parent_author.size() > 0) {
                            try {
                                if (query.select_authors.size() &&
                                    query.select_authors.find(comment_itr->author) == query.select_authors.end()) {
                                    ++comment_itr;
                                    continue;
                                }

                                result.emplace_back(my->get_discussion(comment_itr->id));
                            } catch (const fc::exception &e) {
                                edump((e.to_detail_string()));
                            }
                        }

                        ++comment_itr;
                    }
#endif
                    return result;
                });
            }

            DEFINE_API(plugin, get_trending_categories) {
                CHECK_ARG_SIZE(2)
                auto after = args.args->at(0).as<string>();
                auto limit = args.args->at(1).as<uint32_t>();
                return my->database().with_read_lock([&]() {
                    limit = std::min(limit, uint32_t(100));
                    std::vector<category_api_object> result;
                    result.reserve(limit);

                    const auto &nidx = my->database().get_index<golos::chain::category_index>().indices().get<by_name>();

                    const auto &ridx = my->database().get_index<golos::chain::category_index>().indices().get<by_rshares>();
                    auto itr = ridx.begin();
                    if (after != "" && nidx.size()) {
                        auto nitr = nidx.lower_bound(after);
                        if (nitr == nidx.end()) {
                            itr = ridx.end();
                        } else {
                            itr = ridx.iterator_to(*nitr);
                        }
                    }

                    while (itr != ridx.end() && result.size() < limit) {
                        result.emplace_back(category_api_object(*itr));
                        ++itr;
                    }
                    return result;
                });
            }

            DEFINE_API(plugin, get_best_categories) {
                CHECK_ARG_SIZE(2)
                auto after = args.args->at(0).as<string>();
                auto limit = args.args->at(1).as<uint32_t>();
                return my->database().with_read_lock([&]() {
                    limit = std::min(limit, uint32_t(100));
                    std::vector<category_api_object> result;
                    result.reserve(limit);
                    return result;
                });
            }

            DEFINE_API(plugin, get_active_categories) {
                CHECK_ARG_SIZE(2)
                auto after = args.args->at(0).as<string>();
                auto limit = args.args->at(1).as<uint32_t>();
                return my->database().with_read_lock([&]() {
                    limit = std::min(limit, uint32_t(100));
                    std::vector<category_api_object> result;
                    result.reserve(limit);
                    return result;
                });
            }

            DEFINE_API(plugin, get_recent_categories) {
                CHECK_ARG_SIZE(2)
                auto after = args.args->at(0).as<string>();
                auto limit = args.args->at(1).as<uint32_t>();
                return my->database().with_read_lock([&]() {
                    limit = std::min(limit, uint32_t(100));
                    std::vector<category_api_object> result;
                    result.reserve(limit);
                    return result;
                });
            }
*/

            std::vector<account_name_type> plugin::api_impl::get_miner_queue() const {
                std::vector<account_name_type> result;
                const auto &pow_idx = database().get_index<witness_index>().indices().get<by_pow>();

                auto itr = pow_idx.upper_bound(0);
                while (itr != pow_idx.end()) {
                    if (itr->pow_worker) {
                        result.push_back(itr->owner);
                    }
                    ++itr;
                }
                return result;
            }


            DEFINE_API(plugin, get_miner_queue) {
                return my->database().with_read_lock([&]() {
                    return my->get_miner_queue();
                });
            }

            DEFINE_API(plugin, get_active_witnesses) {
                return my->database().with_read_lock([&]() {
                    const auto &wso = my->database().get_witness_schedule_object();
                    size_t n = wso.current_shuffled_witnesses.size();
                    vector<account_name_type> result;
                    result.reserve(n);
                    for (size_t i = 0; i < n; i++) {
                        result.push_back(wso.current_shuffled_witnesses[i]);
                    }
                    return result;
                });
            }

            DEFINE_API(plugin, get_discussions_by_author_before_date) {
                CHECK_ARG_SIZE(4)
                auto author = args.args->at(0).as<string>();
                auto start_permlink = args.args->at(1).as<string>();
                auto before_date = args.args->at(2).as<time_point_sec>();
                auto limit = args.args->at(3).as<uint32_t>();

                return my->database().with_read_lock([&]() {
                    try {
                        std::vector<discussion> result;
#ifndef STEEMIT_BUILD_LOW_MEMORY
                        FC_ASSERT(limit <= 100);
                        result.reserve(limit);
                        uint32_t count = 0;
                        const auto &didx = my->database().get_index<comment_index>().indices().get<
                                by_author_last_update>();

                        if (before_date == time_point_sec()) {
                            before_date = time_point_sec::maximum();
                        }

                        auto itr = didx.lower_bound(boost::make_tuple(author, time_point_sec::maximum()));
                        if (start_permlink.size()) {
                            const auto &comment = my->database().get_comment(author, start_permlink);
                            if (comment.created < before_date) {
                                itr = didx.iterator_to(comment);
                            }
                        }


                        while (itr != didx.end() && itr->author == author && count < limit) {
                            if (itr->parent_author.size() == 0) {
                                result.push_back(*itr);
                                my->set_pending_payout(result.back());
                                result.back().active_votes = my->get_active_votes(itr->author,
                                                                                  to_string(itr->permlink));
                                ++count;
                            }
                            ++itr;
                        }

#endif
                        return result;
                    } FC_CAPTURE_AND_RETHROW((author)(start_permlink)(before_date)(limit))
                });
            }

            DEFINE_API(plugin, get_savings_withdraw_from) {
                CHECK_ARG_SIZE(1)
                auto account = args.args->at(0).as<string>();
                return my->database().with_read_lock([&]() {
                    std::vector<savings_withdraw_api_object> result;

                    const auto &from_rid_idx = my->database().get_index<savings_withdraw_index>().indices().get<
                            by_from_rid>();
                    auto itr = from_rid_idx.lower_bound(account);
                    while (itr != from_rid_idx.end() && itr->from == account) {
                        result.push_back(savings_withdraw_api_object(*itr));
                        ++itr;
                    }
                    return result;
                });
            }

            DEFINE_API(plugin, get_savings_withdraw_to) {
                CHECK_ARG_SIZE(1)
                auto account = args.args->at(0).as<string>();
                return my->database().with_read_lock([&]() {
                    std::vector<savings_withdraw_api_object> result;

                    const auto &to_complete_idx = my->database().get_index<savings_withdraw_index>().indices().get<
                            by_to_complete>();
                    auto itr = to_complete_idx.lower_bound(account);
                    while (itr != to_complete_idx.end() && itr->to == account) {
                        result.push_back(savings_withdraw_api_object(*itr));
                        ++itr;
                    }
                    return result;
                });
            }





            DEFINE_API(plugin, get_transaction) {
                CHECK_ARG_SIZE(1)
                auto id = args.args->at(0).as<transaction_id_type>();
                return my->database().with_read_lock([&]() {
                    const auto &idx = my->database().get_index<operation_index>().indices().get<by_transaction_id>();
                    auto itr = idx.lower_bound(id);
                    if (itr != idx.end() && itr->trx_id == id) {
                        auto blk = my->database().fetch_block_by_number(itr->block);
                        FC_ASSERT(blk.valid());
                        FC_ASSERT(blk->transactions.size() > itr->trx_in_block);
                        annotated_signed_transaction result = blk->transactions[itr->trx_in_block];
                        result.block_num = itr->block;
                        result.transaction_num = itr->trx_in_block;
                        return result;
                    }
                    FC_ASSERT(false, "Unknown Transaction ${t}", ("t", id));
                });
            }

            template<typename Object, typename DatabaseIndex, typename DiscussionIndex, typename CommentIndex,
                    typename ...Args>
            std::multimap<Object, discussion, DiscussionIndex> plugin::api_impl::select(
                    const std::set<std::string> &select_set, const discussion_query &query,
                    comment_object::id_type parent, const std::function<bool(const comment_api_object &)> &filter,
                    const std::function<bool(const comment_api_object &)> &exit,
                    const std::function<bool(const Object &)> &exit2, Args... args) const {
                std::multimap<Object, discussion, DiscussionIndex> map_result;
                std::string helper;

                const auto &index = database().get_index<DatabaseIndex>().indices().template get<DiscussionIndex>();

                if (!select_set.empty()) {
                    for (const auto &iterator : select_set) {
                        helper = fc::to_lower(iterator);

                        auto tidx_itr = index.lower_bound(boost::make_tuple(helper, args...));

                        auto result = get_discussions<Object, DatabaseIndex, DiscussionIndex, CommentIndex>(query,
                                                                                                            helper,
                                                                                                            parent,
                                                                                                            index,
                                                                                                            tidx_itr,
                                                                                                            filter,
                                                                                                            exit,
                                                                                                            exit2);

                        map_result.insert(result.cbegin(), result.cend());
                    }
                } else {
                    auto tidx_itr = index.lower_bound(boost::make_tuple(helper, args...));

                    map_result = get_discussions<Object, DatabaseIndex, DiscussionIndex, CommentIndex>(query, helper,
                                                                                                       parent, index,
                                                                                                       tidx_itr, filter,
                                                                                                       exit, exit2);
                }

                return map_result;
            }

            void plugin::plugin_initialize(const boost::program_options::variables_map &options) {
                my.reset(new api_impl());
                JSON_RPC_REGISTER_API(plugin_name)
            }

        }
    }
} // golos::application
