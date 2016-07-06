#include <steemit/app/plugin.hpp>

#include <graphene/db/generic_index.hpp>

#include <boost/multi_index/composite_key.hpp>

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
#ifndef BLOCKCHAIN_STATISTICS_SPACE_ID
#define BLOCKCHAIN_STATISTICS_SPACE_ID 9
#endif

#ifndef BLOCKCHAIN_STATISTICS_PLUGIN_NAME
#define BLOCKCHAIN_STATISTICS_PLUGIN_NAME "chain_stats"
#endif

namespace steemit { namespace blockchain_statistics {

using namespace chain;
using namespace graphene::db;

namespace detail
{
   class blockchain_statistics_plugin_impl;
}

class blockchain_statistics_plugin : public steemit::app::plugin
{
   public:
      blockchain_statistics_plugin();
      virtual ~blockchain_statistics_plugin();

      virtual std::string plugin_name()const override { return BLOCKCHAIN_STATISTICS_PLUGIN_NAME; }
      virtual void plugin_set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg ) override;
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;

      const flat_set< uint32_t >& get_tracked_buckets() const;
      uint32_t get_max_history_per_bucket() const;

   private:
      friend class detail::blockchain_statistics_plugin_impl;
      std::unique_ptr< detail::blockchain_statistics_plugin_impl > _my;
};

struct bucket_object : public abstract_object< bucket_object >
{
   static const uint8_t space_id = BLOCKCHAIN_STATISTICS_SPACE_ID;
   static const uint8_t type_id = 0;

   fc::time_point_sec   open;
   uint32_t             seconds = 0;
   uint32_t             blocks = 0;
   uint32_t             operations = 0;
   uint32_t             transactions = 0;
   uint32_t             transfers = 0;
   share_type           steem_transferred = 0;
   share_type           sbd_transferred = 0;
   share_type           sbd_paid_as_interest = 0;
   uint32_t             accounts_created = 0;
   uint32_t             total_posts = 0;
   uint32_t             top_level_posts = 0;
   uint32_t             replies = 0;
   uint32_t             posts_modified = 0;
   uint32_t             posts_deleted = 0;
   uint32_t             total_votes = 0;
   uint32_t             new_votes = 0;
   uint32_t             changed_votes = 0;
   uint32_t             payouts = 0;
   share_type           sbd_paid_to_comments = 0;
   share_type           vests_paid_to_comments = 0;
   share_type           vests_paid_to_curators = 0;
   share_type           liquidity_rewards_paid = 0;
   uint32_t             transfers_to_vesting = 0;
   share_type           steem_vested = 0;
   uint32_t             vesting_withdrawals_processed = 0;
   share_type           vests_withdrawn = 0;
   share_type           vests_transferred = 0;
   uint32_t             limit_orders_created = 0;
   uint32_t             limit_orders_filled = 0;
   uint32_t             limit_orders_cancelled = 0;
   uint32_t             total_pow = 0;
   uint128_t            estimated_hashpower = 0;
};

struct by_bucket;
typedef multi_index_container<
   bucket_object,
   indexed_by<
      hashed_unique< tag< by_id >, member< object, object_id_type, &object::id > >,
      ordered_unique< tag< by_bucket >,
         composite_key< bucket_object,
            member< bucket_object, uint32_t, &bucket_object::seconds >,
            member< bucket_object, fc::time_point_sec, &bucket_object::open >
         >
      >
   >
> bucket_object_multi_index_type;


typedef object_id< BLOCKCHAIN_STATISTICS_SPACE_ID, 0, bucket_object >                  bucket_object_id_type;

typedef generic_index< bucket_object, bucket_object_multi_index_type >                 bucket_index;

} } // steemit::blockchain_statistics

FC_REFLECT_DERIVED( steemit::blockchain_statistics::bucket_object, (graphene::db::object),
   (open)
   (seconds)
   (blocks)
   (operations)
   (transactions)
   (transfers)
   (steem_transferred)
   (sbd_transferred)
   (sbd_paid_as_interest)
   (accounts_created)
   (total_posts)
   (top_level_posts)
   (replies)
   (posts_modified)
   (posts_deleted)
   (total_votes)
   (new_votes)
   (changed_votes)
   (payouts)
   (sbd_paid_to_comments)
   (vests_paid_to_comments)
   (vests_paid_to_curators)
   (liquidity_rewards_paid)
   (transfers_to_vesting)
   (steem_vested)
   (vesting_withdrawals_processed)
   (vests_withdrawn)
   (vests_transferred)
   (limit_orders_created)
   (limit_orders_filled)
   (limit_orders_cancelled)
   (total_pow)
   (estimated_hashpower)
)
