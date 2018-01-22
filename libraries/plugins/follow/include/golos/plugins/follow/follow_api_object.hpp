#ifndef GOLOS_FOLLOW_API_OBJECT_HPP
#define GOLOS_FOLLOW_API_OBJECT_HPP

#include <golos/plugins/social_network/api_object/comment_api_object.hpp>
#include <golos/plugins/follow/follow_objects.hpp>
#include "follow_forward.hpp"
namespace golos {
    namespace plugins {
        namespace follow {
            using golos::protocol::account_name_type;

            struct feed_entry {
                account_name_type author;
                std::string permlink;
                std::vector<account_name_type> reblog_by;
                time_point_sec reblog_on;
                uint32_t entry_id = 0;
            };

            struct comment_feed_entry {
                social_network::comment_api_object comment;
                std::vector<account_name_type> reblog_by;
                time_point_sec reblog_on;
                uint32_t entry_id = 0;
            };

            struct blog_entry {
                account_name_type author;
                account_name_type permlink;
                account_name_type blog;
                time_point_sec reblog_on;
                uint32_t entry_id = 0;
            };

            struct comment_blog_entry {
                social_network::comment_api_object comment;
                std::string blog;
                time_point_sec reblog_on;
                uint32_t entry_id = 0;
            };

            struct account_reputation {
                account_name_type account;
                golos::protocol::share_type reputation;
            };

            struct follow_api_object {
                account_name_type follower;
                account_name_type following;
                std::vector<follow_type> what;
            };

            struct reblog_count {
                account_name_type author;
                uint32_t count;
            };

            struct get_followers_a {
                account_name_type account;
                account_name_type start;
                follow_type type;
                uint32_t limit = 1000;
            };

            struct get_followers_r {
                std::vector<follow_api_object> followers;
            };

            typedef get_followers_a get_following_a;

            struct get_following_r {
                std::vector<follow_api_object> following;
            };

            struct get_follow_count_a {
                account_name_type account;
            };

            struct get_follow_count_r {
                account_name_type account;
                uint32_t follower_count;
                uint32_t following_count;
            };

            struct get_feed_entries_a {
                account_name_type account;
                uint32_t start_entry_id = 0;
                uint32_t limit = 500;
            };

            struct get_feed_entries_r {
                std::vector<feed_entry> feed;
            };

            typedef get_feed_entries_a get_feed_a;

            struct get_feed_r {
                std::vector<comment_feed_entry> feed;
            };

            typedef get_feed_entries_a get_blog_entries_a;

            struct get_blog_entries_r {
                std::vector<blog_entry> blog;
            };

            typedef get_feed_entries_a get_blog_a;

            struct get_blog_r {
                std::vector<comment_blog_entry> blog;
            };

            struct get_account_reputations_a {
                account_name_type account_lower_bound;
                uint32_t limit = 1000;
            };

            struct get_account_reputations_r {
                std::vector<account_reputation> reputations;
            };

            struct get_reblogged_by_a {
                account_name_type author;
                std::string permlink;
            };

            struct get_reblogged_by_r {
                std::vector<account_name_type> accounts;
            };

            struct get_blog_authors_a {
                account_name_type blog_account;
            };

            struct get_blog_authors_r {
                std::vector<std::pair<account_name_type, uint32_t> > blog_authors;
            };
        }}}

FC_REFLECT((golos::plugins::follow::feed_entry), (author)(permlink)(reblog_by)(reblog_on)(entry_id));

FC_REFLECT((golos::plugins::follow::comment_feed_entry), (comment)(reblog_by)(reblog_on)(entry_id));

FC_REFLECT((golos::plugins::follow::blog_entry), (author)(permlink)(blog)(reblog_on)(entry_id));

FC_REFLECT((golos::plugins::follow::comment_blog_entry), (comment)(blog)(reblog_on)(entry_id));

FC_REFLECT((golos::plugins::follow::account_reputation), (account)(reputation));

FC_REFLECT((golos::plugins::follow::follow_api_object), (follower)(following)(what));

FC_REFLECT((golos::plugins::follow::reblog_count), (author)(count));

FC_REFLECT((golos::plugins::follow::get_followers_a), (account)(start)(type)(limit));

FC_REFLECT((golos::plugins::follow::get_followers_r), (followers));

FC_REFLECT((golos::plugins::follow::get_following_r), (following));

FC_REFLECT((golos::plugins::follow::get_follow_count_a), (account));

FC_REFLECT((golos::plugins::follow::get_follow_count_r), (account)(follower_count)(following_count));

FC_REFLECT((golos::plugins::follow::get_feed_entries_a), (account)(start_entry_id)(limit));

FC_REFLECT((golos::plugins::follow::get_feed_entries_r), (feed));

FC_REFLECT((golos::plugins::follow::get_feed_r), (feed));

FC_REFLECT((golos::plugins::follow::get_blog_entries_r), (blog));

FC_REFLECT((golos::plugins::follow::get_blog_r), (blog));

FC_REFLECT((golos::plugins::follow::get_account_reputations_a), (account_lower_bound)(limit));

FC_REFLECT((golos::plugins::follow::get_account_reputations_r), (reputations));

FC_REFLECT((golos::plugins::follow::get_reblogged_by_a), (author)(permlink));

FC_REFLECT((golos::plugins::follow::get_reblogged_by_r), (accounts));

FC_REFLECT((golos::plugins::follow::get_blog_authors_a), (blog_account));

FC_REFLECT((golos::plugins::follow::get_blog_authors_r), (blog_authors));

#endif //GOLOS_FOLLOW_API_OBJECT_HPP
