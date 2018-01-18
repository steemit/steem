#pragma once

#include <golos/chain/comment_object.hpp>
#include <golos/chain/account_object.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <golos/plugins/chain/plugin.hpp>
#include <golos/plugins/json_rpc/utility.hpp>
#include <golos/plugins/json_rpc/plugin.hpp>

namespace golos {
    namespace plugins {
        namespace social_network {
            namespace languages {
                using namespace golos::chain;
                using namespace boost::multi_index;
                using golos::chain::account_object;

                using chainbase::object;
                using chainbase::object_id;
                using chainbase::allocator;

                //
                // Plugins should #define their SPACE_ID's so plugins with
                // conflicting SPACE_ID assignments can be compiled into the
                // same binary (by simply re-assigning some of the conflicting #defined
                // SPACE_ID's in a build script).
                //
                // Assignment of SPACE_ID's cannot be done at run-time because
                // various template automagic depends on them being known at compile
                // time.
                //
#ifndef LANGUAGES_SPACE_ID
#define LANGUAGES_SPACE_ID 12
#endif

                typedef fc::fixed_string<fc::sha256> language_name_type;

                // Plugins need to define object type IDs such that they do not conflict
                // globally. If each plugin uses the upper 8 bits as a space identifier,
                // with 0 being for chain, then the lower 8 bits are free for each plugin
                // to define as they see fit.
                enum languages_object_types {
                    language_object_type = (LANGUAGES_SPACE_ID << 8),
                    language_stats_object_type = (LANGUAGES_SPACE_ID << 8) + 1,
                    peer_stats_object_type = (LANGUAGES_SPACE_ID << 8) + 2,
                    author_language_stats_object_type = (LANGUAGES_SPACE_ID << 8) + 3
                };

                /**
                 *  The purpose of the tag object is to allow the generation and listing of
                 *  all top level posts by a string tag.  The desired sort orders include:
                 *
                 *  1. created - time of creation
                 *  2. maturing - about to receive a payout
                 *  3. active - last reply the post or any child of the post
                 *  4. netvotes - individual accounts voting for post minus accounts voting against it
                 *
                 *  When ever a comment is modified, all language_objects for that comment are updated to match.
                 */
                class language_object : public object<language_object_type, language_object> {
                public:
                    template<typename Constructor, typename Allocator>
                    language_object(Constructor &&c, allocator<Allocator> a): name("") {
                        c(*this);
                    }

                    language_object() {
                    }

                    id_type id;

                    language_name_type name;
                    time_point_sec created;
                    time_point_sec active;
                    time_point_sec cashout;
                    int64_t net_rshares = 0;
                    int32_t net_votes = 0;
                    int32_t children = 0;
                    double hot = 0;
                    double trending = 0;
                    share_type promoted_balance = 0;

                    /**
                     *  Used to track the total rshares^2 of all children, this is used for indexing purposes. A discussion
                     *  that has a nested comment of high value should promote the entire discussion so that the comment can
                     *  be reviewed.
                     */
                    fc::uint128_t children_rshares2;

                    account_object::id_type author;
                    comment_object::id_type parent;
                    comment_object::id_type comment;

                    bool is_post() const {
                        return parent == comment_object::id_type();
                    }
                };

                typedef object_id<language_object> language_id_type;

                template<typename T, typename C = std::less<T>>
                class comparable_index {
                public:
                    typedef T value_type;

                    virtual bool operator()(const T &first, const T &second) const = 0;
                };

                class by_cashout : public comparable_index<language_object> {
                public:
                    virtual bool
                    operator()(const language_object &first, const language_object &second) const override {
                        return std::less<language_name_type>()(first.name, second.name) &&
                               std::less<time_point_sec>()(first.cashout, second.cashout) &&
                               std::less<language_id_type>()(first.id, second.id);
                    }
                }; /// all posts regardless of depth

                class by_net_rshares : public comparable_index<language_object> {
                public:
                    virtual bool
                    operator()(const language_object &first, const language_object &second) const override {
                        return std::greater<int64_t>()(first.net_rshares, second.net_rshares) &&
                               std::less<language_id_type>()(first.id, second.id);
                    }
                }; /// all comments regardless of depth

                class by_parent_created : public comparable_index<language_object> {
                public:
                    virtual bool
                    operator()(const language_object &first, const language_object &second) const override {
                        return std::less<comment_object::id_type>()(first.parent, second.parent) &&
                               std::greater<time_point_sec>()(first.created, second.created) &&
                               std::less<language_id_type>()(first.id, second.id);
                    }
                };

                class by_parent_active : public comparable_index<language_object> {
                public:
                    virtual bool
                    operator()(const language_object &first, const language_object &second) const override {
                        return std::less<comment_object::id_type>()(first.parent, second.parent) &&
                               std::greater<time_point_sec>()(first.active, second.active) &&
                               std::less<language_id_type>()(first.id, second.id);
                    }
                };

                class by_parent_promoted : public comparable_index<language_object> {
                public:
                    virtual bool
                    operator()(const language_object &first, const language_object &second) const override {
                        return std::less<comment_object::id_type>()(first.parent, second.parent) &&
                               std::greater<share_type>()(first.promoted_balance, second.promoted_balance) &&
                               std::less<language_id_type>()(first.id, second.id);
                    }
                };

                class by_parent_net_rshares : public comparable_index<language_object> {
                public:
                    virtual bool
                    operator()(const language_object &first, const language_object &second) const override {
                        return std::less<comment_object::id_type>()(first.parent, second.parent) &&
                               std::greater<int64_t>()(first.net_rshares, second.net_rshares) &&
                               std::less<language_id_type>()(first.id, second.id);
                    }
                }; /// all top level posts by direct pending payout

                class by_parent_net_votes : public comparable_index<language_object> {
                public:
                    virtual bool
                    operator()(const language_object &first, const language_object &second) const override {
                        return std::less<comment_object::id_type>()(first.parent, second.parent) &&
                               std::greater<int32_t>()(first.net_votes, second.net_votes) &&
                               std::less<language_id_type>()(first.id, second.id);
                    }
                }; /// all top level posts by direct votes

                class by_parent_children_rshares2 : public comparable_index<language_object> {
                public:
                    virtual bool
                    operator()(const language_object &first, const language_object &second) const override {
                        return std::less<comment_object::id_type>()(first.parent, second.parent) &&
                               std::greater<fc::uint128_t>()(first.children_rshares2, second.children_rshares2) &&
                               std::less<language_id_type>()(first.id, second.id);
                    }
                }; /// all top level posts by total cumulative payout (aka payout)

                class by_parent_trending : public comparable_index<language_object> {
                public:
                    virtual bool
                    operator()(const language_object &first, const language_object &second) const override {
                        return std::less<comment_object::id_type>()(first.parent, second.parent) &&
                               std::greater<double>()(first.trending, second.trending) &&
                               std::less<language_id_type>()(first.id, second.id);
                    }
                };

                class by_parent_children : public comparable_index<language_object> {
                public:
                    virtual bool
                    operator()(const language_object &first, const language_object &second) const override {
                        return std::less<comment_object::id_type>()(first.parent, second.parent) &&
                               std::greater<int32_t>()(first.children, second.children) &&
                               std::less<language_id_type>()(first.id, second.id);
                    }
                }; /// all top level posts with the most discussion (replies at all levels)

                class by_parent_hot : public comparable_index<language_object> {
                public:
                    virtual bool
                    operator()(const language_object &first, const language_object &second) const override {
                        return std::less<comment_object::id_type>()(first.parent, second.parent) &&
                               std::greater<double>()(first.hot, second.hot) &&
                               std::less<language_id_type>()(first.id, second.id);
                    }
                };

                class by_author_parent_created : public comparable_index<language_object> {
                public:
                    virtual bool
                    operator()(const language_object &first, const language_object &second) const override {
                        return std::less<account_object::id_type>()(first.author, second.author) &&
                               std::greater<time_point_sec>()(first.created, second.created) &&
                               std::less<language_id_type>()(first.id, second.id);
                    }
                };  /// all blog posts by author with tag

                class by_author_comment : public comparable_index<language_object> {
                public:
                    virtual bool
                    operator()(const language_object &first, const language_object &second) const override {
                        return std::less<account_object::id_type>()(first.author, second.author) &&
                               std::less<comment_object::id_type>()(first.comment, second.comment) &&
                               std::less<language_id_type>()(first.id, second.id);
                    }
                };

                class by_reward_fund_net_rshares : public comparable_index<language_object> {
                public:
                    virtual bool
                    operator()(const language_object &first, const language_object &second) const override {
                        return std::less<bool>()(first.is_post(), second.is_post()) &&
                               std::greater<int64_t>()(first.net_rshares, second.net_rshares) &&
                               std::less<language_id_type>()(first.id, second.id);
                    }
                };

                struct by_comment;
                struct by_tag;

                typedef multi_index_container<language_object, indexed_by<
                        ordered_unique<tag<by_id>, member<language_object, language_id_type, &language_object::id>>,
                        ordered_non_unique<tag<by_comment>,
                                member<language_object, comment_object::id_type, &language_object::comment>>,
                        ordered_unique<tag<by_author_comment>, composite_key<language_object,
                                member<language_object, account_object::id_type, &language_object::author>,
                                member<language_object, comment_object::id_type, &language_object::comment>,
                                member<language_object, language_id_type, &language_object::id> >,
                                composite_key_compare<std::less<account_object::id_type>,
                                        std::less<comment_object::id_type>, std::less<language_id_type>>>,
                        ordered_unique<tag<by_parent_created>, composite_key<language_object,
                                member<language_object, language_name_type, &language_object::name>,
                                member<language_object, comment_object::id_type, &language_object::parent>,
                                member<language_object, time_point_sec, &language_object::created>,
                                member<language_object, language_id_type, &language_object::id> >,
                                composite_key_compare<std::less<language_name_type>, std::less<comment_object::id_type>,
                                        std::greater<time_point_sec>, std::less<language_id_type>>>,
                        ordered_unique<tag<by_parent_active>, composite_key<language_object,
                                member<language_object, language_name_type, &language_object::name>,
                                member<language_object, comment_object::id_type, &language_object::parent>,
                                member<language_object, time_point_sec, &language_object::active>,
                                member<language_object, language_id_type, &language_object::id> >,
                                composite_key_compare<std::less<language_name_type>, std::less<comment_object::id_type>,
                                        std::greater<time_point_sec>, std::less<language_id_type>>>,
                        ordered_unique<tag<by_parent_promoted>, composite_key<language_object,
                                member<language_object, language_name_type, &language_object::name>,
                                member<language_object, comment_object::id_type, &language_object::parent>,
                                member<language_object, share_type, &language_object::promoted_balance>,
                                member<language_object, language_id_type, &language_object::id> >,
                                composite_key_compare<std::less<language_name_type>, std::less<comment_object::id_type>,
                                        std::greater<share_type>, std::less<language_id_type>>>,
                        ordered_unique<tag<by_parent_net_rshares>, composite_key<language_object,
                                member<language_object, language_name_type, &language_object::name>,
                                member<language_object, comment_object::id_type, &language_object::parent>,
                                member<language_object, int64_t, &language_object::net_rshares>,
                                member<language_object, language_id_type, &language_object::id> >,
                                composite_key_compare<std::less<language_name_type>, std::less<comment_object::id_type>,
                                        std::greater<int64_t>, std::less<language_id_type>>>,
                        ordered_unique<tag<by_parent_net_votes>, composite_key<language_object,
                                member<language_object, language_name_type, &language_object::name>,
                                member<language_object, comment_object::id_type, &language_object::parent>,
                                member<language_object, int32_t, &language_object::net_votes>,
                                member<language_object, language_id_type, &language_object::id> >,
                                composite_key_compare<std::less<language_name_type>, std::less<comment_object::id_type>,
                                        std::greater<int32_t>, std::less<language_id_type>>>,
                        ordered_unique<tag<by_parent_children>, composite_key<language_object,
                                member<language_object, language_name_type, &language_object::name>,
                                member<language_object, comment_object::id_type, &language_object::parent>,
                                member<language_object, int32_t, &language_object::children>,
                                member<language_object, language_id_type, &language_object::id> >,
                                composite_key_compare<std::less<language_name_type>, std::less<comment_object::id_type>,
                                        std::greater<int32_t>, std::less<language_id_type>>>,
                        ordered_unique<tag<by_parent_hot>, composite_key<language_object,
                                member<language_object, language_name_type, &language_object::name>,
                                member<language_object, comment_object::id_type, &language_object::parent>,
                                member<language_object, double, &language_object::hot>,
                                member<language_object, language_id_type, &language_object::id> >,
                                composite_key_compare<std::less<language_name_type>, std::less<comment_object::id_type>,
                                        std::greater<double>, std::less<language_id_type>>>,
                        ordered_unique<tag<by_parent_trending>, composite_key<language_object,
                                member<language_object, language_name_type, &language_object::name>,
                                member<language_object, comment_object::id_type, &language_object::parent>,
                                member<language_object, double, &language_object::trending>,
                                member<language_object, language_id_type, &language_object::id> >,
                                composite_key_compare<std::less<language_name_type>, std::less<comment_object::id_type>,
                                        std::greater<double>, std::less<language_id_type>>>,
                        ordered_unique<tag<by_parent_children_rshares2>, composite_key<language_object,
                                member<language_object, language_name_type, &language_object::name>,
                                member<language_object, comment_object::id_type, &language_object::parent>,
                                member<language_object, fc::uint128_t, &language_object::children_rshares2>,
                                member<language_object, language_id_type, &language_object::id> >,
                                composite_key_compare<std::less<language_name_type>, std::less<comment_object::id_type>,
                                        std::greater<fc::uint128_t>, std::less<language_id_type>>>,
                        ordered_unique<tag<by_cashout>, composite_key<language_object,
                                member<language_object, language_name_type, &language_object::name>,
                                member<language_object, time_point_sec, &language_object::cashout>,
                                member<language_object, language_id_type, &language_object::id> >,
                                composite_key_compare<std::less<language_name_type>, std::less<time_point_sec>,
                                        std::less<language_id_type>>>, ordered_unique<tag<by_net_rshares>,
                                composite_key<language_object,
                                        member<language_object, language_name_type, &language_object::name>,
                                        member<language_object, int64_t, &language_object::net_rshares>,
                                        member<language_object, language_id_type, &language_object::id> >,
                                composite_key_compare<std::less<language_name_type>, std::greater<int64_t>,
                                        std::less<language_id_type>>>, ordered_unique<tag<by_author_parent_created>,
                                composite_key<language_object,
                                        member<language_object, language_name_type, &language_object::name>,
                                        member<language_object, account_object::id_type, &language_object::author>,
                                        member<language_object, time_point_sec, &language_object::created>,
                                        member<language_object, language_id_type, &language_object::id> >,
                                composite_key_compare<std::less<language_name_type>, std::less<account_object::id_type>,
                                        std::greater<time_point_sec>, std::less<language_id_type>>>,
                        ordered_unique<tag<by_reward_fund_net_rshares>, composite_key<language_object,
                                member<language_object, language_name_type, &language_object::name>,
                                const_mem_fun<language_object, bool, &language_object::is_post>,
                                member<language_object, int64_t, &language_object::net_rshares>,
                                member<language_object, language_id_type, &language_object::id> >,
                                composite_key_compare<std::less<language_name_type>, std::less<bool>, std::greater<int64_t>,
                                        std::less<language_id_type>>> >, allocator<language_object> > language_index;

                /**
                 *  The purpose of this index is to quickly identify how popular various tags by maintaining various sums over
                 *  all posts under a particular tag
                 */
                class language_stats_object : public object<language_stats_object_type, language_stats_object> {
                public:
                    template<typename Constructor, typename Allocator>
                    language_stats_object(Constructor &&c, allocator<Allocator>) {
                        c(*this);
                    }

                    language_stats_object() {
                    }

                    id_type id;

                    language_name_type language;
                    fc::uint128_t total_children_rshares2;
                    asset total_payout = asset (0, SBD_SYMBOL);
                    int32_t net_votes = 0;
                    uint32_t top_posts = 0;
                    uint32_t comments = 0;
                };

                typedef object_id<language_stats_object> language_stats_id_type;

                struct by_comments;
                struct by_top_posts;
                struct by_trending;

                typedef multi_index_container<language_stats_object, indexed_by<ordered_unique<tag<by_id>,
                        member<language_stats_object, language_stats_id_type, &language_stats_object::id>>,
                        ordered_unique<tag<by_tag>,
                                member<language_stats_object, language_name_type, &language_stats_object::language>>,
                        ordered_non_unique<tag<by_trending>, composite_key<language_stats_object,
                                member<language_stats_object, fc::uint128_t,
                                        &language_stats_object::total_children_rshares2>,
                                member<language_stats_object, language_name_type, &language_stats_object::language> >,
                                composite_key_compare<std::greater<uint128_t>, std::less<language_name_type>>> >,
                        allocator<language_stats_object> > language_stats_index;


                /**
                 *  The purpose of this object is to track the relationship between accounts based upon how a user votes. Every time
                 *  a user votes on a post, the relationship between voter and author increases direct rshares.
                 */
                class peer_stats_object : public object<peer_stats_object_type, peer_stats_object> {
                public:
                    template<typename Constructor, typename Allocator>
                    peer_stats_object(Constructor &&c, allocator<Allocator> a) {
                        c(*this);
                    }

                    peer_stats_object() {
                    }

                    id_type id;

                    account_object::id_type voter;
                    account_object::id_type peer;
                    int32_t direct_positive_votes = 0;
                    int32_t direct_votes = 1;

                    int32_t indirect_positive_votes = 0;
                    int32_t indirect_votes = 1;

                    float rank = 0;

                    void update_rank() {
                        auto direct = float(direct_positive_votes) / direct_votes;
                        auto indirect = float(indirect_positive_votes) / indirect_votes;
                        auto direct_order = log(direct_votes);
                        auto indirect_order = log(indirect_votes);

                        if (!(direct_positive_votes + indirect_positive_votes)) {
                            direct_order *= -1;
                            indirect_order *= -1;
                        }

                        direct *= direct;
                        indirect *= indirect;

                        direct *= direct_order * 10;
                        indirect *= indirect_order;

                        rank = direct + indirect;
                    }
                };

                typedef object_id<peer_stats_object> peer_stats_id_type;

                struct by_rank;
                struct by_voter_peer;
                typedef multi_index_container<peer_stats_object, indexed_by<
                        ordered_unique<tag<by_id>, member<peer_stats_object, peer_stats_id_type, &peer_stats_object::id>>,
                        ordered_unique<tag<by_rank>, composite_key<peer_stats_object,
                                member<peer_stats_object, account_object::id_type, &peer_stats_object::voter>,
                                member<peer_stats_object, float, &peer_stats_object::rank>,
                                member<peer_stats_object, account_object::id_type, &peer_stats_object::peer> >,
                                composite_key_compare<std::less<account_object::id_type>, std::greater<float>,
                                        std::less<account_object::id_type>>>, ordered_unique<tag<by_voter_peer>,
                                composite_key<peer_stats_object,
                                        member<peer_stats_object, account_object::id_type, &peer_stats_object::voter>,
                                        member<peer_stats_object, account_object::id_type, &peer_stats_object::peer> >,
                                composite_key_compare<std::less<account_object::id_type>,
                                        std::less<account_object::id_type>>> >,
                        allocator<peer_stats_object> > peer_stats_index;

                /**
                 *  This purpose of this object is to maintain stats about which tags an author uses, how frequnetly, and
                 *  how many total earnings of all posts by author in tag.  It also allows us to answer the question of which
                 *  authors earn the most in each tag category.  This helps users to discover the best bloggers to follow for
                 *  particular tags.
                 */
                class author_language_stats_object : public object<author_language_stats_object_type,
                        author_language_stats_object> {
                public:
                    template<typename Constructor, typename Allocator>
                    author_language_stats_object(Constructor &&c, allocator<Allocator>) {
                        c(*this);
                    }

                    id_type id;
                    account_object::id_type author;
                    language_name_type language;
                    asset total_rewards = asset (0, SBD_SYMBOL);
                    uint32_t total_posts = 0;
                };

                typedef object_id<author_language_stats_object> author_tag_stats_id_type;

                struct by_author_tag_posts;
                struct by_author_posts_tag;
                struct by_author_tag_rewards;
                struct by_tag_rewards_author;
                using std::less;
                using std::greater;

                typedef chainbase::shared_multi_index_container<author_language_stats_object, indexed_by<
                        ordered_unique<tag<by_id>, member<author_language_stats_object, author_tag_stats_id_type,
                                &author_language_stats_object::id> >, ordered_unique<tag<by_author_posts_tag>,
                                composite_key<author_language_stats_object,
                                        member<author_language_stats_object, account_object::id_type,
                                                &author_language_stats_object::author>,
                                        member<author_language_stats_object, uint32_t,
                                                &author_language_stats_object::total_posts>,
                                        member<author_language_stats_object, language_name_type,
                                                &author_language_stats_object::language> >,
                                composite_key_compare<less<account_object::id_type>, greater<uint32_t>,
                                        less<language_name_type>>>, ordered_unique<tag<by_author_tag_posts>,
                                composite_key<author_language_stats_object,
                                        member<author_language_stats_object, account_object::id_type,
                                                &author_language_stats_object::author>,
                                        member<author_language_stats_object, language_name_type,
                                                &author_language_stats_object::language>,
                                        member<author_language_stats_object, uint32_t,
                                                &author_language_stats_object::total_posts> >,
                                composite_key_compare<less<account_object::id_type>, less<language_name_type>,
                                        greater<uint32_t>>>, ordered_unique<tag<by_author_tag_rewards>,
                                composite_key<author_language_stats_object,
                                        member<author_language_stats_object, account_object::id_type,
                                                &author_language_stats_object::author>,
                                        member<author_language_stats_object, language_name_type,
                                                &author_language_stats_object::language>,
                                        member<author_language_stats_object, asset,
                                                &author_language_stats_object::total_rewards> >,
                                composite_key_compare<less<account_object::id_type>, less<language_name_type>,
                                        greater<asset>>>, ordered_unique<tag<by_tag_rewards_author>,
                                composite_key<author_language_stats_object,
                                        member<author_language_stats_object, language_name_type,
                                                &author_language_stats_object::language>,
                                        member<author_language_stats_object, asset,
                                                &author_language_stats_object::total_rewards>,
                                        member<author_language_stats_object, account_object::id_type,
                                                &author_language_stats_object::author> >,
                                composite_key_compare<less<language_name_type>, greater<asset>,
                                        less<account_object::id_type>>> > > author_language_stats_index;


                /**
                 * Used to parse the metadata from the comment json_meta field.
                 */
                struct comment_metadata {
                    std::string language;
                };




            }
        }
    } // golos::plugins::languages
}

FC_REFLECT((golos::plugins::social_network::languages::comment_metadata), (language));

FC_REFLECT((golos::plugins::social_network::languages::language_object),
           (id)(name)(created)(active)(cashout)(net_rshares)(net_votes)(hot)(trending)(promoted_balance)(children)(
                   children_rshares2)(author)(parent)(comment))
CHAINBASE_SET_INDEX_TYPE(golos::plugins::social_network::languages::language_object, golos::plugins::social_network::languages::language_index)

FC_REFLECT((golos::plugins::social_network::languages::language_stats_object),
           (id)(language)(total_children_rshares2)(total_payout)(net_votes)(top_posts)(comments));
CHAINBASE_SET_INDEX_TYPE(golos::plugins::social_network::languages::language_stats_object,
                         golos::plugins::social_network::languages::language_stats_index)

FC_REFLECT((golos::plugins::social_network::languages::peer_stats_object),
           (id)(voter)(peer)(direct_positive_votes)(direct_votes)(indirect_positive_votes)(indirect_votes)(rank));
CHAINBASE_SET_INDEX_TYPE(golos::plugins::social_network::languages::peer_stats_object, golos::plugins::social_network::languages::peer_stats_index)

FC_REFLECT((golos::plugins::social_network::languages::author_language_stats_object),
           (id)(author)(language)(total_posts)(total_rewards))
CHAINBASE_SET_INDEX_TYPE(golos::plugins::social_network::languages::author_language_stats_object,
                         golos::plugins::social_network::languages::author_language_stats_index)