#pragma once

#include <golos/app/application.hpp>
#include <golos/app/steem_api_objects.hpp>

#include <steemit/follow/follow_objects.hpp>
#include <fc/include/fc/reflect/reflect.hpp>


namespace golos {
    namespace plugins {
        namespace follow {

            struct follow_api_obj {
                string follower;
                string following;
                vector <follow_type> what;
            };

            struct follow_count_api_obj {
                follow_count_api_obj() {
                }

                follow_count_api_obj(const follow_count_object &o) :
                        account(o.account),
                        follower_count(o.follower_count),
                        following_count(o.following_count) {
                }

                string account;
                uint32_t follower_count = 0;
                uint32_t following_count = 0;
            };



            class follow_api {
            public:
                follow_api(const app::api_context &ctx);

                void on_api_startup();

                vector <follow_api_obj> get_followers(string to, string start, follow_type type, uint16_t limit) const;

                vector <follow_api_obj>
                get_following(string from, string start, follow_type type, uint16_t limit) const;

                follow_count_api_obj get_follow_count(string account) const;

                vector <feed_entry> get_feed_entries(string account, uint32_t entry_id = 0, uint16_t limit = 500) const;

                vector <comment_feed_entry> get_feed(string account, uint32_t entry_id = 0, uint16_t limit = 500) const;

                vector <blog_entry> get_blog_entries(string account, uint32_t entry_id = 0, uint16_t limit = 500) const;

                vector <comment_blog_entry> get_blog(string account, uint32_t entry_id = 0, uint16_t limit = 500) const;

                vector <account_reputation>
                get_account_reputations(string lower_bound_name, uint32_t limit = 1000) const;

                /**
                 * Gets list of accounts that have reblogged a particular post
                 */
                vector <account_name_type> get_reblogged_by(const string &author, const string &permlink) const;

                /**
                 * Gets a list of authors that have had their content reblogged on a given blog account
                 */
                vector <pair<account_name_type, uint32_t>>
                get_blog_authors(const account_name_type &blog_account) const;

            private:
                std::shared_ptr <detail::follow_api_impl> my;
            };

        }
    } // golos::follow
}

FC_REFLECT(golos::plugins::follow::feed_entry, (author)(permlink)(reblog_by)(reblog_on)(entry_id));
FC_REFLECT(golos::plugins::follow::comment_feed_entry, (comment)(reblog_by)(reblog_on)(entry_id));
FC_REFLECT(golos::plugins::follow::blog_entry, (author)(permlink)(blog)(reblog_on)(entry_id));
FC_REFLECT(golos::plugins::follow::comment_blog_entry, (comment)(blog)(reblog_on)(entry_id));
FC_REFLECT(golos::plugins::follow::account_reputation, (account)(reputation));
FC_REFLECT(golos::plugins::follow::follow_api_obj, (follower)(following)(what));
FC_REFLECT(golos::plugins::follow::follow_count_api_obj, (account)(follower_count)(following_count));