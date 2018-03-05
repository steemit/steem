#include <boost/program_options/options_description.hpp>
#include <golos/plugins/social_network/social_network.hpp>
#include <golos/plugins/social_network/tag/tags_object.hpp>
#include <golos/plugins/social_network/languages/language_object.hpp>
#include <golos/chain/index.hpp>
#include <golos/plugins/social_network/api_object/discussion.hpp>
#include <golos/plugins/social_network/api_object/discussion_query.hpp>
#include <golos/plugins/social_network/api_object/vote_state.hpp>
#include <golos/plugins/social_network/languages/language_object.hpp>
#include <golos/chain/steem_objects.hpp>
#include <golos/plugins/social_network/tag/tag_visitor.hpp>
#include <golos/plugins/social_network/languages/language_visitor.hpp>
#include <golos/chain/operation_notification.hpp>

#define CHECK_ARG_SIZE(s) \
   FC_ASSERT( args.args->size() == s, "Expected #s argument(s), was ${n}", ("n", args.args->size()) );

namespace golos {
    namespace plugins {
        namespace social_network {
            using golos::chain::feed_history_object;

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

            bool tags_filter(const discussion_query &query, const comment_api_object &c, const std::function<bool(const comment_api_object &)> &condition) {
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


            bool languages_filter(const discussion_query &query, const comment_api_object &c, const std::function<bool(const comment_api_object &)> &condition) {
                std::string language = languages::get_language(c);

                if (query.filter_languages.size()) {
                    if (language.empty()) {
                        return true;
                    }
                }

                if (query.filter_languages.count(language)) {
                    return true;
                }

                return condition(c);
            }


            struct social_network_t::impl final {
                impl():database_(appbase::app().get_plugin<chain::plugin>().db()){}
                ~impl(){}

                void on_operation(const operation_notification &note){
                    try {
                        /// plugins shouldn't ever throw
                        note.op.visit(languages::operation_visitor(database(), cache_languages));
                        note.op.visit(tags::operation_visitor(database()));
                    } catch (const fc::exception &e) {
                        edump((e.to_detail_string()));
                    } catch (...) {
                        elog("unhandled exception");
                    }
                }

                golos::chain::database& database() {
                    return database_;
                }

                golos::chain::database& database() const {
                    return database_;
                }

                comment_object::id_type get_parent(const discussion_query &query) const {
                    return database().with_read_lock([&]() {
                        comment_object::id_type parent;
                        if (query.parent_author && query.parent_permlink) {
                            parent = database().get_comment(*query.parent_author, *query.parent_permlink).id;
                        }
                        return parent;
                    });
                }

                std::vector<vote_state> get_active_votes(std::string author, std::string permlink) const {
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


                template<
                        typename Object,
                        typename DatabaseIndex,
                        typename DiscussionIndex,
                        typename CommentIndex,
                        typename Index,
                        typename StartItr
                >
                std::multimap<Object, discussion, DiscussionIndex> get_discussions(
                        const discussion_query &query,
                        const std::string &tag,
                        comment_object::id_type parent,
                        const Index &tidx,
                        StartItr tidx_itr,
                        const std::function<bool(const comment_api_object &)> &filter,
                        const std::function<bool(const comment_api_object &)> &exit,
                        const std::function<bool(const Object &)> &tag_exit,
                        bool ignore_parent = false
                ) const;


                template<
                        typename Object,
                        typename DatabaseIndex,
                        typename DiscussionIndex,
                        typename CommentIndex,
                        typename ...Args
                >
                std::multimap<Object, discussion, DiscussionIndex> select(
                        const std::set<std::string> &select_set,
                        const discussion_query &query,
                        comment_object::id_type parent,
                        const std::function<bool(const comment_api_object &)> &filter,
                        const std::function<bool(const comment_api_object &)> &exit,
                        const std::function<bool(const Object &)> &exit2,
                        Args... args
                ) const;

                std::vector<discussion> get_content_replies(std::string author, std::string permlink) const;

                std::vector<tag_api_object> get_trending_tags(std::string after, uint32_t limit) const;

                std::vector<std::pair<std::string, uint32_t>> get_tags_used_by_author(const std::string &author) const;

                void set_pending_payout(discussion &d) const;

                void set_url(discussion &d) const;

                std::vector<discussion> get_replies_by_last_update(account_name_type start_parent_author, std::string start_permlink, uint32_t limit) const;

                discussion get_content(std::string author, std::string permlink) const;

                std::vector<discussion> get_discussions_by_promoted(const discussion_query &query) const;

                std::vector<discussion> get_discussions_by_created(const discussion_query &query) const;

                std::vector<discussion> get_discussions_by_cashout(const discussion_query &query) const;

                std::vector<discussion> get_discussions_by_votes(const discussion_query &query) const;

                std::vector<discussion> get_discussions_by_active(const discussion_query &query) const;

                discussion get_discussion(comment_object::id_type, uint32_t truncate_body = 0) const;

                std::vector<discussion> get_discussions_by_children(const discussion_query &query) const;

                std::vector<discussion> get_discussions_by_hot(const discussion_query &query) const;


                template<
                        typename DatabaseIndex,
                        typename DiscussionIndex
                >
                std::vector<discussion> blog(
                        const std::set<string> &select_set,
                        const discussion_query &query,
                        const std::string &start_author,
                        const std::string &start_permlink
                ) const;



                template<
                        typename DatabaseIndex,
                        typename DiscussionIndex
                >
                std::vector<discussion> feed(
                        const std::set<string> &select_set,
                        const discussion_query &query,
                        const std::string &start_author,
                        const std::string &start_permlink
                ) const;


                get_languages_r get_languages() ;

                std::set<std::string> cache_languages;
            private:
                golos::chain::database& database_;
            };


            get_languages_r social_network_t::impl::get_languages() {
                get_languages_r result;
                result.languages = cache_languages;
                return result;
            }

            DEFINE_API ( social_network_t, get_languages ) {
                auto &db = pimpl->database();
                return db.with_read_lock([&]() {
                    get_languages_r result;
                    result = pimpl->get_languages();
                    return result;
                });
            }


            void social_network_t::plugin_startup() {

            }

            void social_network_t::plugin_shutdown() {

            }

            const std::string &social_network_t::name() {
                static std::string name = "social_network";
                return name;
            }

            social_network_t::social_network_t() {

            }

            void social_network_t::set_program_options(boost::program_options::options_description &, boost::program_options::options_description &config_file_options) {

            }

            void social_network_t::plugin_initialize(const boost::program_options::variables_map &options) {
                pimpl.reset(new impl());
                auto &db = pimpl->database();
                pimpl->database().post_apply_operation.connect([&](const operation_notification &note) {
                    pimpl->on_operation(note);
                });
                add_plugin_index<tags::tag_index>(db);
                add_plugin_index<tags::tag_stats_index>(db);
                add_plugin_index<tags::peer_stats_index>(db);
                add_plugin_index<tags::author_tag_stats_index>(db);

                add_plugin_index<languages::language_index>(db);
                add_plugin_index<languages::language_stats_index>(db);
                add_plugin_index<languages::peer_stats_index>(db);
                add_plugin_index<languages::author_language_stats_index>(db);


                JSON_RPC_REGISTER_API ( name() ) ;

            }

            social_network_t::~social_network_t() = default;


            DEFINE_API(social_network_t, get_active_votes) {
                CHECK_ARG_SIZE(2)
                auto author = args.args->at(0).as<string>();
                auto permlink = args.args->at(1).as<string>();
                return pimpl->database().with_read_lock([&]() {
                    return pimpl->get_active_votes(author, permlink);
                });
            }

            void social_network_t::impl::set_url(discussion &d) const {
                const comment_api_object root(database().get<comment_object, by_id>(d.root_comment));
                d.url = "/" + root.category + "/@" + root.author + "/" + root.permlink;
                d.root_title = root.title;
                if (root.id != d.id) {
                    d.url += "#@" + d.author + "/" + d.permlink;
                }
            }

            std::vector<discussion> social_network_t::impl::get_content_replies(std::string author, std::string permlink) const {
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

            DEFINE_API(social_network_t, get_content_replies) {
                CHECK_ARG_SIZE(2)
                auto author = args.args->at(0).as<string>();
                auto permlink = args.args->at(1).as<string>();
                return pimpl->database().with_read_lock([&]() {
                    return pimpl->get_content_replies(author, permlink);
                });
            }

            boost::multiprecision::uint256_t to256(const fc::uint128_t &t) {
                boost::multiprecision::uint256_t result(t.high_bits());
                result <<= 65;
                result += t.low_bits();
                return result;
            }


            void social_network_t::impl::set_pending_payout(discussion &d) const {
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
                    auto vshares = database().calculate_vshares(
                            d.net_rshares.value > 0 ? d.net_rshares.value : 0, d.curve);

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

            template<
                    typename DatabaseIndex,
                    typename DiscussionIndex
            >
            std::vector<discussion> social_network_t::impl::feed(
                    const std::set<string> &select_set,
                    const discussion_query &query,
                    const std::string &start_author,
                    const std::string &start_permlink
            ) const {
                std::vector<discussion> result;

                for (const auto &iterator : query.select_authors) {
                    const auto &account = database().get_account(iterator);

                    const auto &tag_idx = database().get_index<DatabaseIndex>().indices().template get<DiscussionIndex>();

                    const auto &c_idx = database().get_index<follow::feed_index>().indices().get<follow::by_comment>();
                    const auto &f_idx = database().get_index<follow::feed_index>().indices().get<follow::by_feed>();
                    auto feed_itr = f_idx.lower_bound(account.name);

                    if (start_author.size() || start_permlink.size()) {
                        auto start_c = c_idx.find(boost::make_tuple(database().get_comment(start_author, start_permlink).id, account.name));
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

                            result.push_back(get_discussion(feed_itr->comment));
                            if (feed_itr->first_reblogged_by != account_name_type()) {
                                result.back().reblogged_by = std::vector<account_name_type>(feed_itr->reblogged_by.begin(), feed_itr->reblogged_by.end());
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

            DEFINE_API(social_network_t, get_discussions_by_feed) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return pimpl->database().with_read_lock([&]() {
                    query.validate();
                    FC_ASSERT(pimpl->database().has_index<follow::feed_index>(), "Node is not running the follow plugin");
                    FC_ASSERT(query.select_authors.size(), "No such author to select feed from");

                    std::string start_author = query.start_author ? *(query.start_author) : "";
                    std::string start_permlink = query.start_permlink ? *(query.start_permlink) : "";

                    std::vector<discussion> tags_ = pimpl->feed<tags::tag_index, tags::by_comment>(
                            query.select_tags,
                            query,
                            start_author,
                            start_permlink
                    );

                    std::vector<discussion> languages_ = pimpl->feed<languages::language_index, languages::by_comment>(
                            query.select_languages,
                            query,
                            start_author,
                            start_permlink
                    );

                    std::vector<discussion> result = merge(tags_, languages_);

                    return result;
                });
            }

            template<typename DatabaseIndex, typename DiscussionIndex>
            std::vector<discussion> social_network_t::impl::blog(
                    const std::set<string> &select_set,
                    const discussion_query &query,
                    const std::string &start_author,
                    const std::string &start_permlink
            ) const {
                std::vector<discussion> result;
                for (const auto &iterator : query.select_authors) {

                    const auto &account = database().get_account(iterator);

                    const auto &tag_idx = database().get_index<DatabaseIndex>().indices().template get<DiscussionIndex>();

                    const auto &c_idx = database().get_index<follow::blog_index>().indices().get<follow::by_comment>();
                    const auto &b_idx = database().get_index<follow::blog_index>().indices().get<follow::by_blog>();
                    auto blog_itr = b_idx.lower_bound(account.name);

                    if (start_author.size() || start_permlink.size()) {
                        auto start_c = c_idx.find(boost::make_tuple(database().get_comment(start_author, start_permlink).id, account.name));
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

                            result.push_back(get_discussion(blog_itr->comment, query.truncate_body));
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

            DEFINE_API(social_network_t, get_discussions_by_blog) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return pimpl->database().with_read_lock([&]() {
                    query.validate();
                    FC_ASSERT(pimpl->database().has_index<follow::feed_index>(), "Node is not running the follow plugin");
                    FC_ASSERT(query.select_authors.size(), "No such author to select feed from");

                    auto start_author = query.start_author ? *(query.start_author) : "";
                    auto start_permlink = query.start_permlink ? *(query.start_permlink) : "";

                    std::vector<discussion> tags_ = pimpl->blog<tags::tag_index, tags::by_comment>(query.select_tags, query,
                                                                                            start_author,
                                                                                            start_permlink);

                    std::vector<discussion> languages_ = pimpl->blog<languages::language_index, languages::by_comment>(
                            query.select_languages,
                            query,
                            start_author,
                            start_permlink
                    );

                    std::vector<discussion> result = merge(tags_, languages_);

                    return result;
                });
            }

            DEFINE_API(social_network_t, get_discussions_by_comments) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return pimpl->database().with_read_lock([&]() {
                    std::vector<discussion> result;
#ifndef IS_LOW_MEM
                    query.validate();
                    FC_ASSERT(query.start_author, "Must get comments for a specific author");
                    auto start_author = *(query.start_author);
                    auto start_permlink = query.start_permlink ? *(query.start_permlink) : "";

                    const auto &c_idx = pimpl->database().get_index<comment_index>().indices().get<by_permlink>();
                    const auto &t_idx = pimpl->database().get_index<comment_index>().indices().get<
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

                                result.emplace_back(pimpl->get_discussion(comment_itr->id));
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

            DEFINE_API(social_network_t, get_trending_categories) {
                CHECK_ARG_SIZE(2)
                auto after = args.args->at(0).as<string>();
                auto limit = args.args->at(1).as<uint32_t>();
                return pimpl->database().with_read_lock([&]() {
                    limit = std::min(limit, uint32_t(100));
                    std::vector<category_api_object> result;
                    result.reserve(limit);

                    const auto &nidx = pimpl->database().get_index<golos::chain::category_index>().indices().get<by_name>();

                    const auto &ridx = pimpl->database().get_index<golos::chain::category_index>().indices().get<by_rshares>();
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

            DEFINE_API(social_network_t, get_best_categories) {
                CHECK_ARG_SIZE(2)
                auto after = args.args->at(0).as<string>();
                auto limit = args.args->at(1).as<uint32_t>();
                return pimpl->database().with_read_lock([&]() {
                    limit = std::min(limit, uint32_t(100));
                    std::vector<category_api_object> result;
                    result.reserve(limit);
                    return result;
                });
            }

            DEFINE_API(social_network_t, get_active_categories) {
                CHECK_ARG_SIZE(2)
                auto after = args.args->at(0).as<string>();
                auto limit = args.args->at(1).as<uint32_t>();
                return pimpl->database().with_read_lock([&]() {
                    limit = std::min(limit, uint32_t(100));
                    std::vector<category_api_object> result;
                    result.reserve(limit);
                    return result;
                });
            }

            DEFINE_API(social_network_t, get_recent_categories) {
                CHECK_ARG_SIZE(2)
                auto after = args.args->at(0).as<string>();
                auto limit = args.args->at(1).as<uint32_t>();
                return pimpl->database().with_read_lock([&]() {
                    limit = std::min(limit, uint32_t(100));
                    std::vector<category_api_object> result;
                    result.reserve(limit);
                    return result;
                });
            }


            template<
                    typename Object,
                    typename DatabaseIndex,
                    typename DiscussionIndex,
                    typename CommentIndex,
                    typename Index,
                    typename StartItr
            >
            std::multimap<
                    Object,
                    discussion,
                    DiscussionIndex
            > social_network_t::impl::get_discussions(
                    const discussion_query &query,
                    const std::string &tag,
                    comment_object::id_type parent,
                    const Index &tidx,
                    StartItr tidx_itr,
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



            template<
                    typename Object,
                    typename DatabaseIndex,
                    typename DiscussionIndex,
                    typename CommentIndex,
                    typename ...Args>
            std::multimap<Object, discussion, DiscussionIndex> social_network_t::impl::select(
                    const std::set<std::string> &select_set,
                    const discussion_query &query,
                    comment_object::id_type parent,
                    const std::function<bool(const comment_api_object &)> &filter,
                    const std::function<bool(const comment_api_object &)> &exit,
                    const std::function<bool(const Object &)> &exit2, Args... args) const {
            std::multimap<Object, discussion, DiscussionIndex> map_result;
            std::string helper;

            const auto &index = database().get_index<DatabaseIndex>().indices().template get<DiscussionIndex>();

            if (!select_set.empty()) {
                for (const auto &iterator : select_set) {
                    helper = fc::to_lower(iterator);
                    auto tidx_itr = index.lower_bound(boost::make_tuple(helper, args...));
                    auto result = get_discussions<
                            Object,
                            DatabaseIndex,
                            DiscussionIndex,
                            CommentIndex
                    >(
                            query,
                            helper,
                            parent,
                            index,
                            tidx_itr,
                            filter,
                            exit,
                            exit2
                    );
                        map_result.insert(result.cbegin(), result.cend());
                }
        } else {
            auto tidx_itr = index.lower_bound(boost::make_tuple(helper, args...));
            map_result = get_discussions<
                    Object,
                    DatabaseIndex,
                    DiscussionIndex,
                    CommentIndex>(
                    query, helper,
                    parent, index,
                    tidx_itr, filter,
                    exit, exit2);
        }

            return map_result;
        }



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

            DEFINE_API(social_network_t, get_discussions_by_trending) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return pimpl->database().with_read_lock([&]() {
                    query.validate();
                    auto parent = pimpl->get_parent(query);

                    std::multimap<
                            tags::tag_object,
                            discussion,
                            tags::by_parent_trending
                    > map_result = pimpl->select<
                            tags::tag_object,
                            tags::tag_index,
                            tags::by_parent_trending,
                            tags::by_comment>(
                            query.select_tags,
                            query, parent,
                            std::bind(tags_filter, query, std::placeholders::_1, [&](const comment_api_object &c) -> bool {
                                return c.net_rshares <= 0;
                            }),
                            [&](const comment_api_object &c) -> bool {
                                return false;
                            },
                            [&](const tags::tag_object &) -> bool {
                                return false;
                            },
                            parent,
                            std::numeric_limits<double>::max()
                    );

                    std::multimap<
                            languages::language_object,
                            discussion,
                            languages::by_parent_trending
                    > map_result_ = pimpl->select<
                            languages::language_object,
                            languages::language_index,
                            languages::by_parent_trending,
                            languages::by_comment>(
                            query.select_languages,
                            query,
                            parent,
                            std::bind(
                                    languages_filter,
                                    query,
                                    std::placeholders::_1,
                                    [&](const comment_api_object &c) -> bool {
                                        return c.net_rshares <= 0;
                                    }
                            ),
                            [&](const comment_api_object &c) -> bool {
                                return false;
                            },
                            [&](const languages::language_object &) -> bool {
                                return false;
                            },
                            parent,
                            std::numeric_limits<double>::max()
                    );


                    std::vector<discussion> return_result = merge(map_result, map_result_);

                    return return_result;
                });
            }

            std::vector<discussion> social_network_t::impl::get_discussions_by_promoted(const discussion_query &query) const {
                query.validate();
                auto parent = get_parent(query);

                std::multimap<tags::tag_object, discussion, tags::by_parent_promoted> map_result = select <
                        tags::tag_object, tags::tag_index, tags::by_parent_promoted, tags::by_comment >
                        (
                                query.select_tags,
                                        query,
                                        parent,
                                        std::bind(
                                                tags_filter,
                                                query,
                                                std::placeholders::_1,
                                                [&](const comment_api_object &c) -> bool {
                                                    return c.children_rshares2 <= 0; }
                                        ),
                                        [&](const comment_api_object &c) -> bool {
                                            return false; },
                                        [&](const tags::tag_object &) -> bool {
                                            return false;
                                        },
                                        parent,
                                        share_type(STEEMIT_MAX_SHARE_SUPPLY)
                        );


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


            DEFINE_API(social_network_t, get_discussions_by_promoted) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return pimpl->database().with_read_lock([&]() {
                    return pimpl->get_discussions_by_promoted(query);
                });
            }


            DEFINE_API(social_network_t, get_account_votes) {
                CHECK_ARG_SIZE(1)
                account_name_type voter = args.args->at(0).as<account_name_type>();
                return pimpl->database().with_read_lock([&]() {
                    std::vector<account_vote> result;

                    const auto &voter_acnt = pimpl->database().get_account(voter);
                    const auto &idx = pimpl->database().get_index<comment_vote_index>().indices().get<by_voter_comment>();

                    account_object::id_type aid(voter_acnt.id);
                    auto itr = idx.lower_bound(aid);
                    auto end = idx.upper_bound(aid);
                    while (itr != end) {
                        const auto &vo = pimpl->database().get(itr->comment);
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


            std::vector<discussion> social_network_t::impl::get_discussions_by_created(
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

            DEFINE_API(social_network_t, get_discussions_by_created) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return pimpl->database().with_read_lock([&]() {
                    return pimpl->get_discussions_by_created(query);
                });
            }

            std::vector<discussion> social_network_t::impl::get_discussions_by_active(
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

            DEFINE_API(social_network_t, get_discussions_by_active) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return pimpl->database().with_read_lock([&]() {
                    return pimpl->get_discussions_by_active(query);
                });
            }


            std::vector<discussion> social_network_t::impl::get_discussions_by_cashout(
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
                                            return c.children_rshares2 <= 0;
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


            DEFINE_API(social_network_t, get_discussions_by_cashout) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return pimpl->database().with_read_lock([&]() {
                    return pimpl->get_discussions_by_cashout(query);
                });
            }

            DEFINE_API(social_network_t, get_discussions_by_payout) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return pimpl->database().with_read_lock([&]() {
                    query.validate();
                    auto parent = pimpl->get_parent(query);

                    std::multimap<tags::tag_object, discussion, tags::by_net_rshares> map_result = pimpl->select<
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
                            languages::by_net_rshares> map_result_language = pimpl->select<languages::language_object,
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

            std::vector<discussion> social_network_t::impl::get_discussions_by_votes(
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


            DEFINE_API(social_network_t, get_discussions_by_votes) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return pimpl->database().with_read_lock([&]() {
                    return pimpl->get_discussions_by_votes(query);
                });
            }


            std::vector<discussion> social_network_t::impl::get_discussions_by_children(
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

            DEFINE_API(social_network_t, get_discussions_by_children) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return pimpl->database().with_read_lock([&]() {
                    return pimpl->get_discussions_by_children(query);
                });
            }

            std::vector<discussion> social_network_t::impl::get_discussions_by_hot(
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

            DEFINE_API(social_network_t, get_discussions_by_hot) {
                CHECK_ARG_SIZE(1)
                auto query = args.args->at(0).as<discussion_query>();
                return pimpl->database().with_read_lock([&]() {
                    return pimpl->get_discussions_by_hot(query);
                });
            }

            DEFINE_API(social_network_t, get_trending_tags) {
                CHECK_ARG_SIZE(2)
                auto after = args.args->at(0).as<string>();
                auto limit = args.args->at(1).as<uint32_t>();

                return pimpl->database().with_read_lock([&]() {
                    return pimpl->get_trending_tags(after, limit);
                });
            }



            discussion social_network_t::impl::get_discussion(comment_object::id_type id, uint32_t truncate_body) const {
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


            DEFINE_API(social_network_t, get_tags_used_by_author) {
                CHECK_ARG_SIZE(1)
                auto author = args.args->at(0).as<string>();
                return pimpl->database().with_read_lock([&]() {
                    return pimpl->get_tags_used_by_author(author);
                });
            }

            std::vector<tag_api_object> social_network_t::impl::get_trending_tags(std::string after, uint32_t limit) const {
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

            std::vector<std::pair<std::string, uint32_t>> social_network_t::impl::get_tags_used_by_author(
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


            discussion social_network_t::impl::get_content(std::string author, std::string permlink) const {
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

            DEFINE_API(social_network_t, get_content) {
                CHECK_ARG_SIZE(2)
                auto author = args.args->at(0).as<account_name_type>();
                auto permlink = args.args->at(1).as<string>();
                return pimpl->database().with_read_lock([&]() {
                    return pimpl->get_content(author, permlink);
                });
            }



            DEFINE_API(social_network_t, get_discussions_by_author_before_date) {
                CHECK_ARG_SIZE(4)
                auto author = args.args->at(0).as<std::string>();
                auto start_permlink = args.args->at(1).as<std::string>();
                auto before_date = args.args->at(2).as<time_point_sec>();
                auto limit = args.args->at(3).as<uint32_t>();

                return pimpl->database().with_read_lock([&]() {
                    try {
                        std::vector<discussion> result;
#ifndef IS_LOW_MEM
                        FC_ASSERT(limit <= 100);
                        result.reserve(limit);
                        uint32_t count = 0;
                        const auto &didx = pimpl->database().get_index<comment_index>().indices().get<by_author_last_update>();

                        if (before_date == time_point_sec()) {
                            before_date = time_point_sec::maximum();
                        }

                        auto itr = didx.lower_bound(boost::make_tuple(author, time_point_sec::maximum()));
                        if (start_permlink.size()) {
                            const auto &comment = pimpl->database().get_comment(author, start_permlink);
                            if (comment.created < before_date) {
                                itr = didx.iterator_to(comment);
                            }
                        }


                        while (itr != didx.end() && itr->author == author && count < limit) {
                            if (itr->parent_author.size() == 0) {
                                result.push_back(*itr);
                                pimpl->set_pending_payout(result.back());
                                result.back().active_votes = pimpl->get_active_votes(itr->author,
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

            std::vector<discussion> social_network_t::impl::get_replies_by_last_update(
                    account_name_type start_parent_author,
                    std::string start_permlink,
                    uint32_t limit
            ) const {
                std::vector<discussion> result;

#ifndef IS_LOW_MEM
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
            DEFINE_API(social_network_t, get_replies_by_last_update) {
                CHECK_ARG_SIZE(3)
                auto start_parent_author = args.args->at(0).as<account_name_type>();
                auto start_permlink = args.args->at(1).as<string>();
                auto limit = args.args->at(2).as<uint32_t>();
                return pimpl->database().with_read_lock([&]() {
                    return pimpl->get_replies_by_last_update(start_parent_author, start_permlink, limit);
                });
            }



        }
    }
}