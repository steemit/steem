#include <golos/plugins/social_network/languages/language_visitor.hpp>
namespace golos { namespace plugins { namespace social_network { namespace languages {

    operation_visitor::operation_visitor(golos::chain::database& db_, std::set<std::string>& cache_languages_):
        db_(db_),
        cache_languages_(cache_languages_) {
    }

    void
    operation_visitor::remove_stats(const language_object& tag, const language_stats_object& stats) const {
        db_.modify(stats, [&](language_stats_object& s) {
            if (tag.parent == comment_object::id_type()) {
                s.total_children_rshares2 -= tag.children_rshares2;
                s.top_posts--;
            } else {
                s.comments--;
            }
            s.net_votes -= tag.net_votes;
        });
    }

    void operation_visitor::remove_tag(const language_object& tag) const {
        /// TODO: update tag stats object
        db_.remove(tag);

        const auto& idx = db_.get_index<author_language_stats_index>().indices().get<by_author_tag_posts>();
        auto itr = idx.lower_bound(boost::make_tuple(tag.author, tag.name));
        if (itr != idx.end() && itr->author == tag.author && itr->language == tag.name) {
            db_.modify(*itr, [&](author_language_stats_object& stats) {
                stats.total_posts--;
            });
        }
    }

    const language_stats_object& operation_visitor::get_stats(const std::string& tag) const {
        const auto& stats_idx = db_.get_index<language_stats_index>().indices().get<by_tag>();
        auto itr = stats_idx.find(tag);
        if (itr != stats_idx.end()) {
            return *itr;
        }

        return db_.create<language_stats_object>([&](language_stats_object& stats) {
            stats.language = tag;
        });
    }

    void
    operation_visitor::update_tag(const language_object& current, const comment_object& comment, double hot,
                                  double trending) const {
        const auto& stats = get_stats(current.name);
        remove_stats(current, stats);

        if (comment.cashout_time != fc::time_point_sec::maximum()) {
            db_.modify(current, [&](language_object& obj) {
                obj.active = comment.active;
                obj.cashout = db_.calculate_discussion_payout_time(comment);
                obj.children = comment.children;
                obj.net_rshares = comment.net_rshares.value;
                obj.net_votes = comment.net_votes;
                obj.children_rshares2 = comment.children_rshares2;
                obj.hot = hot;
                obj.trending = trending;
                if (obj.cashout == fc::time_point_sec()) {
                    obj.promoted_balance = 0;
                }
            });
            add_stats(current, stats);
        } else {
            db_.remove(current);
        }
    }

    void
    operation_visitor::create_tag(const std::string& language, const comment_object& comment, double hot,
                                  double trending) const {
        comment_object::id_type parent;
        account_object::id_type author = db_.get_account(comment.author).id;

        if (comment.parent_author.size()) {
            parent = db_.get_comment(comment.parent_author, comment.parent_permlink).id;
        }

        const auto& tag_obj = db_.create<language_object>([&](language_object& obj) {
            obj.name = language;
            obj.comment = comment.id;
            obj.parent = parent;
            obj.created = comment.created;
            obj.active = comment.active;
            obj.cashout = comment.cashout_time;
            obj.net_votes = comment.net_votes;
            obj.children = comment.children;
            obj.net_rshares = comment.net_rshares.value;
            obj.children_rshares2 = comment.children_rshares2;
            obj.author = author;
            obj.hot = hot;
            obj.trending = trending;
        });
        add_stats(tag_obj, get_stats(language));


        const auto& idx = db_.get_index<author_language_stats_index>().indices().get<
            by_author_tag_posts>();
        auto itr = idx.lower_bound(boost::make_tuple(author, language));
        if (itr != idx.end() && itr->author == author && itr->language == language) {
            db_.modify(*itr, [&](author_language_stats_object& stats) {
                stats.total_posts++;
            });
        } else {
            db_.create<author_language_stats_object>([&](author_language_stats_object& stats) {
                stats.author = author;
                stats.language = language;
                stats.total_posts = 1;
            });
        }
        ///TODO: CACHE push
        cache_languages_.emplace(language);
    }

    std::string operation_visitor::filter_tags(const comment_object& c) const {
        return get_language(c);
    }

    double operation_visitor::calculate_hot(const share_type& score, const time_point_sec& created) const {
        return calculate_score<10000000, 10000>(score, created);
    }

    double
    operation_visitor::calculate_trending(const share_type& score, const time_point_sec& created) const {
        return calculate_score<10000000, 480000>(score, created);
    }

    void operation_visitor::update_tags(const comment_object& c) const {
        try {
            auto hot = calculate_hot(c.net_rshares, c.created);
            auto trending = calculate_trending(c.net_rshares, c.created);
            auto language = filter_tags(c);
            const auto& comment_idx = db_.get_index<language_index>().indices().get<by_comment>();

            auto itr = comment_idx.find(c.id);

            if (itr == comment_idx.end()) {
                create_tag(language, c, hot, trending);
            } else {
                update_tag(*itr, c, hot, trending);
            }

            if (c.parent_author.size()) {
                update_tags(db_.get_comment(c.parent_author, c.parent_permlink));
            }

        }
        FC_CAPTURE_LOG_AND_RETHROW((c))

    }

    const peer_stats_object& operation_visitor::get_or_create_peer_stats(account_object::id_type voter,
                                                                         account_object::id_type peer) const {
        const auto& peeridx = db_.get_index<peer_stats_index>().indices().get<by_voter_peer>();
        auto itr = peeridx.find(boost::make_tuple(voter, peer));
        if (itr == peeridx.end()) {
            return db_.create<peer_stats_object>([&](peer_stats_object& obj) {
                obj.voter = voter;
                obj.peer = peer;
            });
        }
        return *itr;
    }

    void operation_visitor::update_indirect_vote(account_object::id_type a, account_object::id_type b,
                                                 int positive) const {
        if (a == b) {
            return;
        }
        const auto& ab = get_or_create_peer_stats(a, b);
        const auto& ba = get_or_create_peer_stats(b, a);
        db_.modify(ab, [&](peer_stats_object& o) {
            o.indirect_positive_votes += positive;
            o.indirect_votes++;
            o.update_rank();
        });
        db_.modify(ba, [&](peer_stats_object& o) {
            o.indirect_positive_votes += positive;
            o.indirect_votes++;
            o.update_rank();
        });
    }

    void operation_visitor::update_peer_stats(const account_object& voter, const account_object& author,
                                              const comment_object& c, int vote) const {
        if (voter.id == author.id) {
            return;
        } /// ignore votes for yourself
        if (c.parent_author.size()) {
            return;
        } /// only count top level posts

        const auto& stat = get_or_create_peer_stats(voter.id, author.id);
        db_.modify(stat, [&](peer_stats_object& obj) {
            obj.direct_votes++;
            obj.direct_positive_votes += vote > 0;
            obj.update_rank();
        });

        const auto& voteidx = db_.get_index<comment_vote_index>().indices().get<by_comment_voter>();
        auto itr = voteidx.lower_bound(boost::make_tuple(comment_object::id_type(c.id), account_object::id_type()));
        while (itr != voteidx.end() && itr->comment == c.id) {
            update_indirect_vote(voter.id, itr->voter, (itr->vote_percent > 0) == (vote > 0));
            ++itr;
        }
    }

    void operation_visitor::operator()(const transfer_operation& op) const {
        if (op.to == STEEMIT_NULL_ACCOUNT && op.amount.symbol == SBD_SYMBOL) {
            std::vector<std::string> part;
            part.reserve(4);
            auto path = op.memo;
            boost::split(part, path, boost::is_any_of("/"));
            if (part[0].size() && part[0][0] == '@') {
                auto acnt = part[0].substr(1);
                auto perm = part[1];

                auto c = db_.find_comment(acnt, perm);
                if (c && c->parent_author.size() == 0) {
                    const auto& comment_idx = db_.get_index<language_index>().indices().get<by_comment>();
                    auto citr = comment_idx.lower_bound(c->id);
                    while (citr != comment_idx.end() && citr->comment == c->id) {
                        db_.modify(*citr, [&](language_object& t) {
                            if (t.cashout != fc::time_point_sec::maximum()) {
                                t.promoted_balance += op.amount.amount;
                            }
                        });
                        ++citr;
                    }
                } else {
                    ilog("unable to find body");
                }
            }
        }
    }

    void operation_visitor::operator()(const vote_operation& op) const {
        update_tags(db_.get_comment(op.author, op.permlink));
        /*
        update_peer_stats( db_.get_account(op.voter),
                           db_.get_account(op.author),
                           db_.get_comment(op.author, op.permlink),
                           op.weight );
                           */
    }

    void operation_visitor::operator()(const delete_comment_operation& op) const {
        const auto& idx = db_.get_index<language_index>().indices().get<by_author_comment>();

        const auto& auth = db_.get_account(op.author);
        auto itr = idx.lower_bound(boost::make_tuple(auth.id));
        while (itr != idx.end() && itr->author == auth.id) {
            const auto& tobj = *itr;
            const auto* obj = db_.find<comment_object>(itr->comment);
            ++itr;
            if (obj == nullptr) {
                db_.remove(tobj);
            } else {
                //TODO:: clear cache
                cache_languages_.erase(get_language(*obj));
            }
        }
    }

    void operation_visitor::operator()(const comment_reward_operation& op) const {
        const auto& c = db_.get_comment(op.author, op.permlink);
        update_tags(c);
    }

    void operation_visitor::operator()(const comment_payout_update_operation& op) const {
        const auto& c = db_.get_comment(op.author, op.permlink);
        update_tags(c);
    }

    void operation_visitor::add_stats(const language_object& tag, const language_stats_object& stats) const {
        db_.modify(stats, [&](language_stats_object& s) {
            if (tag.parent == comment_object::id_type()) {
                s.total_children_rshares2 += tag.children_rshares2;
                s.top_posts++;
            } else {
                s.comments++;
            }
            s.net_votes += tag.net_votes;
        });
    }
} } } } // golos::plugins::::social_network::namespace languages