#ifndef GOLOS_FOLLOW_API_OBJECT_HPP
#define GOLOS_FOLLOW_API_OBJECT_HPP

#include <golos/plugins/social_network/api_object/comment_api_object.hpp>
#include <golos/plugins/follow/follow_objects.hpp>
#include "follow_forward.hpp"
namespace golos {
    namespace plugins {
        namespace follow {

            struct feed_entry {
                std::string author;
                std::string permlink;
                std::vector<std::string> reblog_by;
                time_point_sec reblog_on;
                uint32_t entry_id = 0;
            };

            struct comment_feed_entry {
                social_network::comment_api_object comment;
                std::vector<std::string> reblog_by;
                time_point_sec reblog_on;
                uint32_t entry_id = 0;
            };

            struct blog_entry {
                std::string author;
                std::string permlink;
                std::string blog;
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
                std::string account;
                golos::protocol::share_type reputation;
            };

            struct follow_api_object {
                std::string follower;
                std::string following;
                std::vector<follow_type> what;
            };

            struct reblog_count {
                std::string author;
                uint32_t count;
            };
            struct follow_count_api_obj {
                follow_count_api_obj() {}
                follow_count_api_obj(const std::string& acc,
                    uint32_t followers,
                    uint32_t followings,
                    uint32_t lim)
                     : account(acc),
                     follower_count(followers),
                     following_count(followings),
                     limit(lim) {
                }

                follow_count_api_obj(const follow_count_api_obj &o) :
                        account(o.account),
                        follower_count(o.follower_count),
                        following_count(o.following_count),
                        limit(o.limit) {
                }
                string account;
                uint32_t follower_count = 0;
                uint32_t following_count = 0;
                uint32_t limit = 1000;
            };

            using blog_authors_r = std::vector<std::pair<std::string, uint32_t>>;
        }}}

FC_REFLECT((golos::plugins::follow::feed_entry), (author)(permlink)(reblog_by)(reblog_on)(entry_id));

FC_REFLECT((golos::plugins::follow::comment_feed_entry), (comment)(reblog_by)(reblog_on)(entry_id));

FC_REFLECT((golos::plugins::follow::blog_entry), (author)(permlink)(blog)(reblog_on)(entry_id));

FC_REFLECT((golos::plugins::follow::comment_blog_entry), (comment)(blog)(reblog_on)(entry_id));

FC_REFLECT((golos::plugins::follow::account_reputation), (account)(reputation));

FC_REFLECT((golos::plugins::follow::follow_api_object), (follower)(following)(what));

FC_REFLECT((golos::plugins::follow::reblog_count), (author)(count));

FC_REFLECT((golos::plugins::follow::follow_count_api_obj), (account)(follower_count)(following_count)(limit));


#endif //GOLOS_FOLLOW_API_OBJECT_HPP
