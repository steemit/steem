#pragma once

#include <appbase/application.hpp>
#include <golos/plugins/chain/plugin.hpp>
#include <golos/plugins/social_network/api_object/discussion.hpp>
#include <golos/plugins/social_network/api_object/category_api_object.hpp>
#include <golos/plugins/social_network/api_object/tag_api_object.hpp>
#include <golos/plugins/follow/plugin.hpp>
#include <golos/plugins/social_network/api_object/account_vote.hpp>

namespace golos {
    namespace plugins {
        namespace social_network {
            using plugins::json_rpc::msg_pack;

            using tags_used_by_author_r = std::vector<std::pair<std::string, uint32_t>>;

            struct get_languages_r {
                std::set<std::string> languages;
            };

            DEFINE_API_ARGS(get_content,                           msg_pack, discussion)
            DEFINE_API_ARGS(get_trending_tags,                     msg_pack, std::vector<tag_api_object>)
            DEFINE_API_ARGS(get_content_replies,                   msg_pack, std::vector<discussion>)
            DEFINE_API_ARGS(get_tags_used_by_author,               msg_pack, tags_used_by_author_r)
            DEFINE_API_ARGS(get_discussions_by_payout,             msg_pack, std::vector<discussion>)
            DEFINE_API_ARGS(get_discussions_by_trending,           msg_pack, std::vector<discussion>)
            DEFINE_API_ARGS(get_discussions_by_created,            msg_pack, std::vector<discussion>)
            DEFINE_API_ARGS(get_discussions_by_active,             msg_pack, std::vector<discussion>)
            DEFINE_API_ARGS(get_discussions_by_cashout,            msg_pack, std::vector<discussion>)
            DEFINE_API_ARGS(get_discussions_by_votes,              msg_pack, std::vector<discussion>)
            DEFINE_API_ARGS(get_discussions_by_children,           msg_pack, std::vector<discussion>)
            DEFINE_API_ARGS(get_discussions_by_hot,                msg_pack, std::vector<discussion>)
            DEFINE_API_ARGS(get_discussions_by_feed,               msg_pack, std::vector<discussion>)
            DEFINE_API_ARGS(get_discussions_by_blog,               msg_pack, std::vector<discussion>)
            DEFINE_API_ARGS(get_discussions_by_comments,           msg_pack, std::vector<discussion>)
            DEFINE_API_ARGS(get_discussions_by_promoted,           msg_pack, std::vector<discussion>)
            DEFINE_API_ARGS(get_replies_by_last_update,            msg_pack, std::vector<discussion>)
            DEFINE_API_ARGS(get_discussions_by_author_before_date, msg_pack, std::vector<discussion>)
            DEFINE_API_ARGS(get_trending_categories,               msg_pack, std::vector<category_api_object>)
            DEFINE_API_ARGS(get_active_categories,                 msg_pack, std::vector<category_api_object>)
            DEFINE_API_ARGS(get_recent_categories,                 msg_pack, std::vector<category_api_object>)
            DEFINE_API_ARGS(get_best_categories,                   msg_pack, std::vector<category_api_object>)
            DEFINE_API_ARGS(get_account_votes,                     msg_pack, std::vector<account_vote>)
            DEFINE_API_ARGS(get_active_votes,                      msg_pack, std::vector<vote_state>)
            DEFINE_API_ARGS( get_languages,                        msg_pack, get_languages_r );

            class social_network_t final : public appbase::plugin<social_network_t> {
            public:
                APPBASE_PLUGIN_REQUIRES((chain::plugin)(follow::plugin))

                DECLARE_API(
                /**
                                    *  Return the active discussions with the highest cumulative pending payouts without respect to category, total
                                    *  pending payout means the pending payout of all children as well.
                                    */
                        (get_replies_by_last_update)
                        (get_trending_tags)

                (get_trending_categories)

                (get_best_categories)

                (get_active_categories)

                (get_recent_categories)

                /**
                             * Used to retrieve the list of first payout discussions sorted by rshares^2 amount
                             * @param query @ref discussion_query
                             * @return vector of first payout mode discussions sorted by rshares^2 amount
                             **/
                (get_discussions_by_trending)

                /**
                 * Used to retrieve the list of discussions sorted by created time
                 * @param query @ref discussion_query
                 * @return vector of discussions sorted by created time
                 **/
                (get_discussions_by_created)

                /**
                 * Used to retrieve the list of discussions sorted by last activity time
                 * @param query @ref discussion_query
                 * @return vector of discussions sorted by last activity time
                 **/
                (get_discussions_by_active)

                /**
                 * Used to retrieve the list of discussions sorted by cashout time
                 * @param query @ref discussion_query
                 * @return vector of discussions sorted by last cashout time
                 **/
                (get_discussions_by_cashout)

                /**
                 * Used to retrieve the list of discussions sorted by net rshares amount
                 * @param query @ref discussion_query
                 * @return vector of discussions sorted by net rshares amount
                 **/
                (get_discussions_by_payout)

                                /**
                                     *  if permlink is "" then it will return all votes for author
                                     */
                                (get_active_votes)

                /**
                 * Used to retrieve the list of discussions sorted by direct votes amount
                 * @param query @ref discussion_query
                 * @return vector of discussions sorted by direct votes amount
                 **/
                (get_discussions_by_votes)

                /**
                 * Used to retrieve the list of discussions sorted by children posts amount
                 * @param query @ref discussion_query
                 * @return vector of discussions sorted by children posts amount
                 **/
                (get_discussions_by_children)

                /**
                 * Used to retrieve the list of discussions sorted by hot amount
                 * @param query @ref discussion_query
                 * @return vector of discussions sorted by hot amount
                 **/
                (get_discussions_by_hot)

                /**
                 * Used to retrieve the list of discussions from the feed of a specific author
                 * @param query @ref discussion_query
                 * @attention @ref discussion_query#select_authors must be set and must contain the @ref discussion_query#start_author param if the last one is not null
                 * @return vector of discussions from the feed of authors in @ref discussion_query#select_authors
                 **/
                (get_discussions_by_feed)

                /**
                 * Used to retrieve the list of discussions from the blog of a specific author
                 * @param query @ref discussion_query
                 * @attention @ref discussion_query#select_authors must be set and must contain the @ref discussion_query#start_author param if the last one is not null
                 * @return vector of discussions from the blog of authors in @ref discussion_query#select_authors
                 **/
                (get_discussions_by_blog)

                (get_account_votes)

                (get_discussions_by_comments)

                                /**
                                                     * Used to retrieve top 1000 tags list used by an author sorted by most frequently used
                                                     * @param author select tags of this author
                                                     * @return vector of top 1000 tags used by an author sorted by most frequently used
                                                     **/
                                (get_tags_used_by_author)

                /**
                 * Used to retrieve the list of discussions sorted by promoted balance amount
                 * @param query @ref discussion_query
                 * @return vector of discussions sorted by promoted balance amount
                 **/
                (get_discussions_by_promoted)
                                (get_content_replies)
                                /**
                                    *  This method is used to fetch all posts/comments by start_author that occur after before_date and start_permlink with up to limit being returned.
                                    *
                                    *  If start_permlink is empty then only before_date will be considered. If both are specified the eariler to the two metrics will be used. This
                                    *  should allow easy pagination.
                                    */
                                (get_discussions_by_author_before_date)

                                (get_content)
                                (get_languages)
                )

                social_network_t();

                ~social_network_t();

                void set_program_options(boost::program_options::options_description &, boost::program_options::options_description &config_file_options) override;

                static const std::string &name();

                void plugin_initialize(const boost::program_options::variables_map &options) override;

                void plugin_startup() override;

                void plugin_shutdown() override;


            private:
                struct impl;
                std::unique_ptr<impl> pimpl;
            };
        }
    }
}

FC_REFLECT((golos::plugins::social_network::get_languages_r), (languages))