#include <steemit/follow/follow_operations.hpp>
#include <steemit/follow/follow_objects.hpp>

#include <steemit/chain/account_object.hpp>
#include <steemit/chain/comment_object.hpp>

namespace steemit {
    namespace follow {

        void follow_evaluator::do_apply(const follow_operation &o) {
            static map<string, follow_type> follow_type_map = []() {
                map<string, follow_type> follow_map;
                follow_map["undefined"] = follow_type::undefined;
                follow_map["blog"] = follow_type::blog;
                follow_map["ignore"] = follow_type::ignore;

                return follow_map;
            }();

            const auto &idx = db().get_index_type<follow_index>().indices().get<by_follower_following>();
            auto itr = idx.find(boost::make_tuple(o.follower, o.following));

            set<follow_type> what;

            for (auto target : o.what) {
                switch (follow_type_map[target]) {
                    case blog:
                        what.insert(blog);
                        break;
                    case ignore:
                        what.insert(ignore);
                        break;
                    default:
                        //ilog( "Encountered unknown option ${o}", ("o", target) );
                        break;
                }
            }

            if (what.find(ignore) != what.end())
                FC_ASSERT(what.find(blog) ==
                          what.end(), "Cannot follow blog and ignore author at the same time");

            if (itr == idx.end()) {
                db().create<follow_object>([&](follow_object &obj) {
                    obj.follower = o.follower;
                    obj.following = o.following;
                    obj.what = what;
                });
            } else {
                db().modify(*itr, [&](follow_object &obj) {
                    obj.what = what;
                });
            }
        }

        void reblog_evaluator::do_apply(const reblog_operation &o) {
            try {
                auto &db = _plugin->database();
                const auto &c = db.get_comment(o.author, o.permlink);
                if (c.parent_author.size() > 0) {
                    return;
                }

                const auto &reblog_account = db.get_account(o.account);
                const auto &blog_idx = db.get_index_type<blog_index>().indices().get<by_blog>();
                const auto &blog_comment_idx = db.get_index_type<blog_index>().indices().get<by_comment>();

                auto next_blog_id = 0;
                auto last_blog = blog_idx.lower_bound(reblog_account.id);

                if (last_blog != blog_idx.end() &&
                    last_blog->account == reblog_account.id) {
                    next_blog_id = last_blog->blog_feed_id + 1;
                }

                auto blog_itr = blog_comment_idx.find(boost::make_tuple(c.id, reblog_account.id));

                if (blog_itr == blog_comment_idx.end()) {
                    db.create<blog_object>([&](blog_object &b) {
                        b.account = reblog_account.id;
                        b.comment = c.id;
                        b.blog_feed_id = next_blog_id;
                    });
                }

                const auto &feed_idx = db.get_index_type<feed_index>().indices().get<by_feed>();
                const auto &comment_idx = db.get_index_type<feed_index>().indices().get<by_comment>();
                const auto &idx = db.get_index_type<follow_index>().indices().get<by_following_follower>();
                auto itr = idx.find(o.account);

                while (itr != idx.end() && itr->following == o.account) {

                    if (itr->what.find(follow_type::blog) != itr->what.end()) {
                        auto account_id = db.get_account(itr->follower).id;
                        uint32_t next_id = 0;
                        auto last_feed = feed_idx.lower_bound(account_id);

                        if (last_feed != feed_idx.end() &&
                            last_feed->account == account_id) {
                            next_id = last_feed->account_feed_id + 1;
                        }

                        auto feed_itr = comment_idx.find(boost::make_tuple(c.id, account_id));

                        if (feed_itr == comment_idx.end()) {
                            auto &fd = db.create<feed_object>([&](feed_object &f) {
                                f.account = account_id;
                                f.first_reblogged_by = reblog_account.id;
                                f.comment = c.id;
                                f.reblogs = 1;
                                f.account_feed_id = next_id;
                            });

                        } else {
                            db.modify(*feed_itr, [&](feed_object &f) {
                                f.reblogs++;
                            });
                        }

                        const auto &old_feed_idx = db.get_index_type<feed_index>().indices().get<by_old_feed>();
                        auto old_feed = old_feed_idx.lower_bound(account_id);

                        while (old_feed->account == account_id &&
                               next_id - old_feed->account_feed_id >
                               _plugin->max_feed_size) {
                            db.remove(*old_feed);
                            old_feed = old_feed_idx.lower_bound(account_id);
                        };
                    }

                    ++itr;
                }
            }
            FC_LOG_AND_RETHROW()
        }

    }
} // steemit::follow
