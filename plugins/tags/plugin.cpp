#include <boost/program_options/options_description.hpp>
#include <golos/plugins/tags/plugin.hpp>
#include <golos/plugins/tags/tags_object.hpp>
#include <golos/chain/index.hpp>
#include <golos/api/discussion.hpp>
#include <golos/plugins/tags/discussion_query.hpp>
#include <golos/api/vote_state.hpp>
#include <golos/chain/steem_objects.hpp>
#include <golos/api/discussion_helper.hpp>
// These visitors creates additional tables, we don't really need them in LOW_MEM mode
#include <golos/plugins/tags/tag_visitor.hpp>
#include <golos/chain/operation_notification.hpp>

#define CHECK_ARG_SIZE(_S)                                 \
   FC_ASSERT(                                              \
       args.args->size() == _S,                            \
       "Expected #_S argument(s), was ${n}",               \
       ("n", args.args->size()) );

#define CHECK_ARG_MIN_SIZE(_S, _M)                         \
   FC_ASSERT(                                              \
       args.args->size() >= _S && args.args->size() <= _M, \
       "Expected #_S (maximum #_M) argument(s), was ${n}", \
       ("n", args.args->size()) );

#define GET_OPTIONAL_ARG(_I, _T, _D)   \
   (args.args->size() > _I) ?          \
   (args.args->at(_I).as<_T>()) :      \
   static_cast<_T>(_D)

namespace golos { namespace plugins { namespace tags {

    using golos::chain::feed_history_object;
    using golos::api::discussion_helper;

    struct tags_plugin::impl final {
        impl(): database_(appbase::app().get_plugin<chain::plugin>().db()) {
            helper = std::make_unique<discussion_helper>(
                database_,
                follow::fill_account_reputation,
                fill_promoted);
        }

        ~impl() {}

        void on_operation(const operation_notification& note) {
#ifndef IS_LOW_MEM
            try {
                /// plugins shouldn't ever throw
                note.op.visit(tags::operation_visitor(database()));
            } catch (const fc::exception& e) {
                edump((e.to_detail_string()));
            } catch (...) {
                elog("unhandled exception");
            }
#endif
        }

        golos::chain::database& database() {
            return database_;
        }

        golos::chain::database& database() const {
            return database_;
        }

        void select_active_votes(
            std::vector<vote_state>& result, uint32_t& total_count,
            const std::string& author, const std::string& permlink, uint32_t limit
        ) const ;

        bool filter_tags(const tags::tag_type type, std::set<std::string>& select_tags) const;

        bool filter_authors(discussion_query& query) const;

        bool filter_start_comment(discussion_query& query) const;

        bool filter_parent_comment(discussion_query& query) const;

        bool filter_query(discussion_query& query) const;

        template<typename DatabaseIndex, typename DiscussionIndex>
        std::vector<discussion> select_unordered_discussions(discussion_query& query) const;

        template<typename Iterator, typename Order, typename Select, typename Exit>
        void select_discussions(
            std::set<comment_object::id_type>& id_set,
            std::vector<discussion>& result,
            const discussion_query& query,
            Iterator itr, Iterator etr,
            Select&& select,
            Exit&& exit,
            Order&& order
        ) const;

        template<typename DiscussionOrder, typename Selector>
        std::vector<discussion> select_ordered_discussions(discussion_query&, Selector&&) const;

        std::vector<tag_api_object> get_trending_tags(const std::string& after, uint32_t limit) const;

        std::vector<std::pair<std::string, uint32_t>> get_tags_used_by_author(const std::string& author) const;

        void set_pending_payout(discussion& d) const;

        void set_url(discussion& d) const;

        std::vector<discussion> get_replies_by_last_update(
            account_name_type start_parent_author, std::string start_permlink,
            uint32_t limit, uint32_t vote_limit
        ) const;

        discussion get_discussion(const comment_object& c, uint32_t vote_limit) const ;

        discussion create_discussion(const comment_object& o) const;
        discussion create_discussion(const comment_object& o, const discussion_query& query) const;
        void fill_discussion(discussion& d, const discussion_query& query) const;

        get_languages_result get_languages();

    private:
        golos::chain::database& database_;
        std::unique_ptr<discussion_helper> helper;
    };

    void tags_plugin::impl::select_active_votes(
        std::vector<vote_state>& result, uint32_t& total_count,
        const std::string& author, const std::string& permlink, uint32_t limit
    ) const {
        helper->select_active_votes(result, total_count, author, permlink, limit);
    }

    discussion tags_plugin::impl::get_discussion(const comment_object& c, uint32_t vote_limit) const {
        return helper->get_discussion(c, vote_limit);
    }

    get_languages_result tags_plugin::impl::get_languages() {
        auto& idx = database().get_index<tags::language_index>().indices().get<tags::by_tag>();
        get_languages_result result;
        for (auto itr = idx.begin(); idx.end() != itr; ++itr) {
            result.languages.insert(itr->name);
        }
        return result;
    }

    discussion tags_plugin::impl::create_discussion(const comment_object& o) const {
        return helper->create_discussion(o);
    }

    void tags_plugin::impl::fill_discussion(discussion& d, const discussion_query& query) const {
        set_url(d);
        set_pending_payout(d);
        select_active_votes(d.active_votes, d.active_votes_count, d.author, d.permlink, query.vote_limit);
        d.body_length = static_cast<uint32_t>(d.body.size());
        if (query.truncate_body) {
            if (d.body.size() > query.truncate_body) {
                d.body.erase(query.truncate_body);
            }

            if (!fc::is_utf8(d.title)) {
                d.title = fc::prune_invalid_utf8(d.title);
            }

            if (!fc::is_utf8(d.body)) {
                d.body = fc::prune_invalid_utf8(d.body);
            }

            if (!fc::is_utf8(d.json_metadata)) {
                d.json_metadata = fc::prune_invalid_utf8(d.json_metadata);
            }
        }
    }

    discussion tags_plugin::impl::create_discussion(const comment_object& o, const discussion_query& query) const {

        discussion d = create_discussion(o);
        fill_discussion(d, query);

        return d;
    }

    DEFINE_API(tags_plugin, get_languages) {
        return pimpl->database().with_weak_read_lock([&]() {
            return pimpl->get_languages();
        });
    }

    void tags_plugin::plugin_startup() {
        wlog("tags plugin: plugin_startup()");
    }

    void tags_plugin::plugin_shutdown() {
        wlog("tags plugin: plugin_shutdown(()");
    }

    const std::string& tags_plugin::name() {
        static std::string name = "tags";
        return name;
    }

    tags_plugin::tags_plugin() {
    }

    void tags_plugin::set_program_options(
        boost::program_options::options_description&,
        boost::program_options::options_description& config_file_options
    ) {
    }

    void tags_plugin::plugin_initialize(const boost::program_options::variables_map& options) {
        pimpl.reset(new impl());
// Disable index creation for tag visitor
#ifndef IS_LOW_MEM
        auto& db = pimpl->database();
        db.post_apply_operation.connect([&](const operation_notification& note) {
            pimpl->on_operation(note);
        });
        add_plugin_index<tags::tag_index>(db);
        add_plugin_index<tags::tag_stats_index>(db);
        add_plugin_index<tags::author_tag_stats_index>(db);
        add_plugin_index<tags::language_index>(db);
#endif
        JSON_RPC_REGISTER_API (name());

    }

    tags_plugin::~tags_plugin() = default;

    void tags_plugin::impl::set_url(discussion& d) const {
        helper->set_url( d );
    }

    boost::multiprecision::uint256_t to256(const fc::uint128_t& t) {
        boost::multiprecision::uint256_t result(t.high_bits());
        result <<= 65;
        result += t.low_bits();
        return result;
    }

    void tags_plugin::impl::set_pending_payout(discussion& d) const {
        helper->set_pending_payout(d);
    }

    bool tags_plugin::impl::filter_tags(const tags::tag_type type, std::set<std::string>& select_tags) const {
        if (select_tags.empty()) {
            return true;
        }

        auto& db = database();
        auto& idx = db.get_index<tags::tag_index>().indices().get<tags::by_tag>();
        auto src_tags = std::move(select_tags);
        for (const auto& name: src_tags) {
            if (idx.find(std::make_tuple(name, type)) != idx.end()) {
                select_tags.insert(name);
            }
        }
        return !select_tags.empty();
    }

    bool tags_plugin::impl::filter_authors(discussion_query& query) const {
        if (query.select_authors.empty()) {
            return true;
        }

        auto& db = database();
        auto select_authors = std::move(query.select_authors);
        for (auto& name: select_authors) {
            auto* author = db.find_account(name);
            if (author) {
                query.select_author_ids.insert(author->id);
                query.select_authors.insert(name);
            }
        }
        return query.has_author_selector();
    }

    bool tags_plugin::impl::filter_start_comment(discussion_query& query) const {
        if (query.has_start_comment()) {
            if (!query.start_permlink.valid()) {
                return false;
            }
            auto* comment = database().find_comment(*query.start_author, *query.start_permlink);
            if (!comment) {
                return false;
            }
            query.start_comment = create_discussion(*comment, query);
        }
        return true;
    }

    bool tags_plugin::impl::filter_parent_comment(discussion_query& query) const {
        if (query.has_parent_comment()) {
            if (!query.parent_permlink) {
                return false;
            }
            auto* comment = database().find_comment(*query.parent_author, *query.parent_permlink);
            if (comment) {
                return false;
            }
            query.parent_comment = create_discussion(*comment, query);
        }
        return true;
    }

    bool tags_plugin::impl::filter_query(discussion_query& query) const {
        if (!filter_tags(tags::tag_type::language, query.select_languages) ||
            !filter_tags(tags::tag_type::tag, query.select_tags) ||
            !filter_authors(query)
        ) {
            return false;
        }

        return true;
    }

    template<
        typename DatabaseIndex,
        typename DiscussionIndex>
    std::vector<discussion> tags_plugin::impl::select_unordered_discussions(discussion_query& query) const {
        std::vector<discussion> result;

        if (!filter_start_comment(query) || !filter_query(query)) {
            return result;
        }

        auto& db = database();
        const auto& idx = db.get_index<DatabaseIndex>().indices().template get<DiscussionIndex>();
        auto etr = idx.end();
        bool can_add = true;

        result.reserve(query.limit);

        std::set<comment_object::id_type> id_set;
        auto aitr = query.select_authors.begin();
        if (query.has_start_comment()) {
            can_add = false;
        }

        for (; query.select_authors.end() != aitr && result.size() < query.limit; ++aitr) {
            auto itr = idx.lower_bound(*aitr);
            for (; itr != etr && itr->account == *aitr && result.size() < query.limit; ++itr) {
                if (id_set.count(itr->comment)) {
                    continue;
                }
                id_set.insert(itr->comment);

                if (query.has_start_comment() && !can_add) {
                    can_add = (query.is_good_start(itr->comment));
                    if (!can_add) {
                        continue;
                    }
                }

                const auto* comment = db.find(itr->comment);
                if (!comment) {
                    continue;
                }

                if ((query.parent_author && *query.parent_author != comment->parent_author) ||
                    (query.parent_permlink && *query.parent_permlink != to_string(comment->parent_permlink))
                ) {
                    continue;
                }

                discussion d = create_discussion(*comment);
                if (!query.is_good_tags(d)) {
                    continue;
                }

                fill_discussion(d, query);
                result.push_back(d);
            }
        }
        return result;
    }

    template<
        typename Iterator,
        typename Order,
        typename Select,
        typename Exit>
    void tags_plugin::impl::select_discussions(
        std::set<comment_object::id_type>& id_set,
        std::vector<discussion>& result,
        const discussion_query& query,
        Iterator itr, Iterator etr,
        Select&& select,
        Exit&& exit,
        Order&& order
    ) const {
        auto& db = database();
        for (; itr != etr && !exit(*itr); ++itr) {
            if (id_set.count(itr->comment)) {
                continue;
            }
            id_set.insert(itr->comment);

            if (!query.is_good_parent(itr->parent) || !query.is_good_author(itr->author)) {
                continue;
            }

            const auto* comment = db.find(itr->comment);
            if (!comment) {
                continue;
            }

            discussion d = create_discussion(*comment);

            fill_discussion(d, query);
            d.promoted = asset(itr->promoted_balance, SBD_SYMBOL);
            d.hot = itr->hot;
            d.trending = itr->trending;

            if (!select(d) || !query.is_good_tags(d)) {
                continue;
            }

            if (query.has_start_comment() && !query.is_good_start(d.id) && !order(query.start_comment, d)) {
                continue;
            }

            result.push_back(std::move(d));
        }
    }

    template<
        typename DiscussionOrder,
        typename Selector>
    std::vector<discussion> tags_plugin::impl::select_ordered_discussions(
        discussion_query& query,
        Selector&& selector
    ) const {
        std::vector<discussion> unordered;
        auto& db = database();

        db.with_weak_read_lock([&]() {
            if (!filter_query(query) || !filter_start_comment(query) || !filter_parent_comment(query) ||
                (query.has_start_comment() && !query.is_good_author(*query.start_author))
            ) {
                return false;
            }

            std::set<comment_object::id_type> id_set;
            if (query.has_tags_selector()) { // seems to have a least complexity
                const auto& idx = db.get_index<tags::tag_index>().indices().get<tags::by_tag>();
                auto etr = idx.end();
                unordered.reserve(query.select_tags.size() * query.limit);

                for (auto& name: query.select_tags) {
                    select_discussions(
                        id_set, unordered, query, idx.lower_bound(std::make_tuple(name, tags::tag_type::tag)), etr,
                        selector,
                        [&](const tags::tag_object& tag){
                            return tag.name != name || tag.type != tags::tag_type::tag;
                        },
                        DiscussionOrder());
                }
            } else if (query.has_author_selector()) { // a more complexity
                const auto& idx = db.get_index<tags::tag_index>().indices().get<tags::by_author_comment>();
                auto etr = idx.end();
                unordered.reserve(query.select_author_ids.size() * query.limit);

                for (auto& id: query.select_author_ids) {
                    select_discussions(
                        id_set, unordered, query, idx.lower_bound(id), etr,
                        selector,
                        [&](const tags::tag_object& tag){
                            return tag.author != id;
                        },
                        DiscussionOrder());
                }
            } else if (query.has_language_selector()) { // the most complexity
                const auto& idx = db.get_index<tags::tag_index>().indices().get<tags::by_tag>();
                auto etr = idx.end();
                unordered.reserve(query.select_languages.size() * query.limit);

                for (auto& name: query.select_languages) {
                    select_discussions(
                        id_set, unordered, query, idx.lower_bound(std::make_tuple(name, tags::tag_type::language)), etr,
                        selector,
                        [&](const tags::tag_object& tag){
                            return tag.name != name || tag.type != tags::tag_type::language;
                        },
                        DiscussionOrder());
                }
            } else {
                const auto& indices = db.get_index<tags::tag_index>().indices();
                const auto& idx = indices.get<DiscussionOrder>();
                auto itr = idx.begin();

                if (query.has_start_comment()) {
                    const auto& cidx = indices.get<tags::by_comment>();
                    const auto citr = cidx.find(query.start_comment.id);
                    if (citr != cidx.end()) {
                        return false;
                    }

                    query.reset_start_comment();
                    itr = idx.iterator_to(*citr);
                }

                unordered.reserve(query.limit);

                select_discussions(
                    id_set, unordered, query, itr, idx.end(), selector,
                    [&](const tags::tag_object& tag){
                        return unordered.size() >= query.limit;
                    },
                    [&](const auto&, const auto&) {
                        return true;
                    });
            }
            return true;
        });

        std::vector<discussion> result;
        if (unordered.empty()) {
            return result;
        }

        auto it = unordered.begin();
        const auto et = unordered.end();
        std::sort(it, et, DiscussionOrder());

        if (query.has_start_comment()) {
            for (; et != it && it->id != query.start_comment.id; ++it);
            if (et == it) {
                return result;
            }
        }

        for (uint32_t idx = 0; idx < query.limit && et != it; ++it, ++idx) {
            result.push_back(std::move(*it));
        }

        return result;
    }

    DEFINE_API(tags_plugin, get_discussions_by_blog) {
        CHECK_ARG_SIZE(1)
        std::vector<discussion> result;

        auto query = args.args->at(0).as<discussion_query>();
        query.prepare();
        query.validate();
        FC_ASSERT(query.select_authors.size(), "Must get blogs for specific authors");


#ifndef IS_LOW_MEM
        auto& db = pimpl->database();
        FC_ASSERT(db.has_index<follow::feed_index>(), "Node is not running the follow plugin");

        return db.with_weak_read_lock([&]() {
            return pimpl->select_unordered_discussions<follow::blog_index, follow::by_blog>(query);
        });
#endif
        return result;
    }

    DEFINE_API(tags_plugin, get_discussions_by_feed) {
        CHECK_ARG_SIZE(1)
        std::vector<discussion> result;

        auto query = args.args->at(0).as<discussion_query>();
        query.prepare();
        query.validate();
        FC_ASSERT(query.select_authors.size(), "Must get feeds for specific authors");

#ifndef IS_LOW_MEM
        auto& db = pimpl->database();
        FC_ASSERT(db.has_index<follow::feed_index>(), "Node is not running the follow plugin");

        return db.with_weak_read_lock([&]() {
            return pimpl->select_unordered_discussions<follow::feed_index, follow::by_feed>(query);
        });
#endif
        return result;
    }

    DEFINE_API(tags_plugin, get_discussions_by_comments) {
        CHECK_ARG_SIZE(1)
        std::vector<discussion> result;
#ifndef IS_LOW_MEM
        auto query = args.args->at(0).as<discussion_query>();
        query.prepare();
        query.validate();
        FC_ASSERT(!!query.start_author, "Must get comments for a specific author");

        auto& db = pimpl->database();
        return db.with_weak_read_lock([&]() {
            const auto &idx = db.get_index<comment_index>().indices().get<by_author_last_update>();
            auto itr = idx.lower_bound(*query.start_author);
            if (itr == idx.end()) {
                return result;
            }

            if (!!query.start_permlink) {
                const auto &lidx = db.get_index<comment_index>().indices().get<by_permlink>();
                auto litr = lidx.find(std::make_tuple(*query.start_author, *query.start_permlink));
                if (litr == lidx.end()) {
                    return result;
                }
                itr = idx.iterator_to(*litr);
            }

            if (!pimpl->filter_query(query)) {
                return result;
            }

            result.reserve(query.limit);

            for (; itr != idx.end() && itr->author == *query.start_author && result.size() < query.limit; ++itr) {
                if (itr->parent_author.size() > 0) {
                    discussion p(db.get<comment_object>(itr->root_comment), db);
                    if (!query.is_good_tags(p) || !query.is_good_author(p.author)) {
                        continue;
                    }
                    result.emplace_back(discussion(*itr, db));
                    pimpl->fill_discussion(result.back(), query);
                }
            }
            return result;
        });
#endif
        return result;
    }

    DEFINE_API(tags_plugin, get_discussions_by_trending) {
        CHECK_ARG_SIZE(1)
        auto query = args.args->at(0).as<discussion_query>();
        query.prepare();
        query.validate();
#ifndef IS_LOW_MEM
        return pimpl->select_ordered_discussions<sort::by_trending>(
            query,
            [&](const discussion& d) -> bool {
                return d.net_rshares > 0;
            }
        );
#endif
        return std::vector<discussion>();
    }

    DEFINE_API(tags_plugin, get_discussions_by_promoted) {
        CHECK_ARG_SIZE(1)
        auto query = args.args->at(0).as<discussion_query>();
        query.prepare();
        query.validate();
#ifndef IS_LOW_MEM
        return pimpl->select_ordered_discussions<sort::by_promoted>(
            query,
            [&](const discussion& d) -> bool {
                return !!d.promoted && d.promoted->amount > 0;
            }
        );
#endif
        return std::vector<discussion>();
    }

    DEFINE_API(tags_plugin, get_discussions_by_created) {
        CHECK_ARG_SIZE(1)
        auto query = args.args->at(0).as<discussion_query>();
        query.prepare();
        query.validate();
#ifndef IS_LOW_MEM
        return pimpl->select_ordered_discussions<sort::by_created>(
            query,
            [&](const discussion& d) -> bool {
                return true;
            }
        );
#endif
        return std::vector<discussion>();
    }

    DEFINE_API(tags_plugin, get_discussions_by_active) {
        CHECK_ARG_SIZE(1)
        auto query = args.args->at(0).as<discussion_query>();
        query.prepare();
        query.validate();
#ifndef IS_LOW_MEM
        return pimpl->select_ordered_discussions<sort::by_active>(
            query,
            [&](const discussion& d) -> bool {
                return true;
            }
        );
#endif
        return std::vector<discussion>();
    }

    DEFINE_API(tags_plugin, get_discussions_by_cashout) {
        CHECK_ARG_SIZE(1)
        auto query = args.args->at(0).as<discussion_query>();
        query.prepare();
        query.validate();
#ifndef IS_LOW_MEM
        return pimpl->select_ordered_discussions<sort::by_cashout>(
            query,
            [&](const discussion& d) -> bool {
                return d.net_rshares > 0;
            }
        );
#endif
        return std::vector<discussion>();
    }

    DEFINE_API(tags_plugin, get_discussions_by_payout) {
        CHECK_ARG_SIZE(1)
        auto query = args.args->at(0).as<discussion_query>();
        query.prepare();
        query.validate();
#ifndef IS_LOW_MEM
        return pimpl->select_ordered_discussions<sort::by_net_rshares>(
            query,
            [&](const discussion& d) -> bool {
                return d.net_rshares > 0;
            }
        );
#endif
        return std::vector<discussion>();
    }

    DEFINE_API(tags_plugin, get_discussions_by_votes) {
        CHECK_ARG_SIZE(1)
        auto query = args.args->at(0).as<discussion_query>();
        query.prepare();
        query.validate();
#ifndef IS_LOW_MEM
        return pimpl->select_ordered_discussions<sort::by_net_votes>(
            query,
            [&](const discussion& d) -> bool {
                return true;
            }
        );
#endif
        return std::vector<discussion>();
    }

    DEFINE_API(tags_plugin, get_discussions_by_children) {
        CHECK_ARG_SIZE(1)
        auto query = args.args->at(0).as<discussion_query>();
        query.prepare();
        query.validate();
#ifndef IS_LOW_MEM
        return pimpl->select_ordered_discussions<sort::by_children>(
            query,
            [&](const discussion& d) -> bool {
                return true;
            }
        );
#endif
        return std::vector<discussion>();
    }

    DEFINE_API(tags_plugin, get_discussions_by_hot) {
        CHECK_ARG_SIZE(1)
        auto query = args.args->at(0).as<discussion_query>();
        query.prepare();
        query.validate();
#ifndef IS_LOW_MEM
        return pimpl->select_ordered_discussions<sort::by_hot>(
            query,
            [&](const discussion& d) -> bool {
                return d.net_rshares > 0;
            }
        );
#endif
        return std::vector<discussion>();
    }

    std::vector<tag_api_object>
    tags_plugin::impl::get_trending_tags(const std::string& after, uint32_t limit) const {
        limit = std::min(limit, uint32_t(1000));
        std::vector<tag_api_object> result;
#ifndef IS_LOW_MEM
        result.reserve(limit);

        const auto& nidx = database().get_index<tags::tag_stats_index>().indices().get<tags::by_tag>();
        const auto& ridx = database().get_index<tags::tag_stats_index>().indices().get<tags::by_trending>();
        auto itr = ridx.begin();
        if (!after.empty() && nidx.size()) {
            auto nitr = nidx.lower_bound(std::make_tuple(tags::tag_type::tag, after));
            if (nitr == nidx.end()) {
                itr = ridx.end();
            } else {
                itr = ridx.iterator_to(*nitr);
            }
        }

        for (; itr->type == tags::tag_type::tag && itr != ridx.end() && result.size() < limit; ++itr) {
            tag_api_object push_object = tag_api_object(*itr);

            if (push_object.name.empty()) {
                continue;
            }

            if (!fc::is_utf8(push_object.name)) {
                push_object.name = fc::prune_invalid_utf8(push_object.name);
            }

            result.emplace_back(push_object);
        }
#endif
        return result;
    }

    DEFINE_API(tags_plugin, get_trending_tags) {
        CHECK_ARG_SIZE(2)
        auto after = args.args->at(0).as<string>();
        auto limit = args.args->at(1).as<uint32_t>();

        return pimpl->database().with_weak_read_lock([&]() {
            return pimpl->get_trending_tags(after, limit);
        });
    }

    std::vector<std::pair<std::string, uint32_t>> tags_plugin::impl::get_tags_used_by_author(
        const std::string& author
    ) const {
        std::vector<std::pair<std::string, uint32_t>> result;
#ifndef IS_LOW_MEM
        auto& db = database();
        const auto* acnt = db.find_account(author);
        if (acnt == nullptr) {
            return result;
        }
        const auto& tidx = db.get_index<tags::author_tag_stats_index>().indices().get<tags::by_author_posts_tag>();
        auto itr = tidx.lower_bound(std::make_tuple(acnt->id, tags::tag_type::tag));
        for (;itr != tidx.end() && itr->author == acnt->id && result.size() < 1000; ++itr) {
            if (itr->type == tags::tag_type::tag && itr->name.size()) {
                if (!fc::is_utf8(itr->name)) {
                    result.emplace_back(std::make_pair(fc::prune_invalid_utf8(itr->name), itr->total_posts));
                } else {
                    result.emplace_back(std::make_pair(itr->name, itr->total_posts));
                }
            }
        }
#endif
        return result;
    }

    DEFINE_API(tags_plugin, get_tags_used_by_author) {
        CHECK_ARG_SIZE(1)
        auto author = args.args->at(0).as<string>();
        return pimpl->database().with_weak_read_lock([&]() {
            return pimpl->get_tags_used_by_author(author);
        });
    }

    DEFINE_API(tags_plugin, get_discussions_by_author_before_date) {
        std::vector<discussion> result;

        CHECK_ARG_MIN_SIZE(4, 5)
#ifndef IS_LOW_MEM
        auto author = args.args->at(0).as<std::string>();
        auto start_permlink = args.args->at(1).as<std::string>();
        auto before_date = args.args->at(2).as<time_point_sec>();
        auto limit = args.args->at(3).as<uint32_t>();
        auto vote_limit = GET_OPTIONAL_ARG(4, uint32_t, DEFAULT_VOTE_LIMIT);

        FC_ASSERT(limit <= 100);

        result.reserve(limit);

        if (before_date == time_point_sec()) {
            before_date = time_point_sec::maximum();
        }

        auto& db = pimpl->database();

        return db.with_weak_read_lock([&]() {
            try {
                uint32_t count = 0;
                const auto& didx = db.get_index<comment_index>().indices().get<by_author_last_update>();

                auto itr = didx.lower_bound(std::make_tuple(author, before_date));
                if (start_permlink.size()) {
                    const auto& comment = db.get_comment(author, start_permlink);
                    if (comment.last_update < before_date) {
                        itr = didx.iterator_to(comment);
                    }
                }

                while (itr != didx.end() && itr->author == author && count < limit) {
                    if (itr->parent_author.size() == 0) {
                        result.push_back(pimpl->get_discussion(*itr, vote_limit));
                        ++count;
                    }
                    ++itr;
                }

                return result;
            } FC_CAPTURE_AND_RETHROW((author)(start_permlink)(before_date)(limit))
        });
#endif
        return result;
    }

    // Needed for correct work of golos::api::discussion_helper::set_pending_payout and etc api methods
    void fill_promoted(const golos::chain::database& db, discussion & d) {
        if (!db.has_index<tags::tag_index>()) {
            return;
        }

        const auto& cidx = db.get_index<tags::tag_index>().indices().get<tags::by_comment>();
        auto itr = cidx.lower_bound(d.id);
        if (itr != cidx.end() && itr->comment == d.id) {
            d.promoted = asset(itr->promoted_balance, SBD_SYMBOL);
        } else {
            d.promoted = asset(0, SBD_SYMBOL);
        }
    }

} } } // golos::plugins::tags
