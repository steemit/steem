#include <steemit/app/plugin.hpp>

namespace steemit { namespace market_history {

using namespace chain;

namespace detail
{
   class market_history_plugin_impl;
}

class market_history_plugin : public steemit::app::plugin
{
   public:
      market_history_plugin();
      virtual ~market_history_plugin();

      virtual std::string plugin_name()const override { return "market_history"; }
      virtual void plugin_set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg ) override;
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;

   private:
      friend class detail::market_history_plugin_impl;
      std::unique_ptr< detail::market_history_plugin_impl > _my;
};

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
#ifndef ACCOUNT_HISTORY_SPACE_ID
#define ACCOUNT_HISTORY_SPACE_ID 5
#endif

struct bucket_key
{
   bucket_key( uint32_t s, fc::time_point_sec o )
      :seconds(s), open(o) {}
   bucket_key(){}

   uint32_t             seconds = 0;
   fc::time_point_sec   open;

   friend bool operator < ( const bucket_key& a, const bucket_key& b )
   {
      return std::tie( a.seconds, a.open ) < std::tie( b.seconds, b.open );
   }

   friend bool operator == ( const bucket_key& a, const bucket_key& b )
   {
      return std::tie( a.seconds, a.open ) == std::tie( a.seconds, a.open );
   }
};

struct bucket_object : public abstract_object< bucket_object >
{
   static const uint8_t space_id = ACCOUNT_HISTORY_SPACE_ID;
   static const uint8_t type_id = 1;

   price high()const { return asset( high_steem, STEEM_SYMBOL ) / asset( high_sbd, SBD_SYMBOL ); }
   price low()const { return asset( low_steem, STEEM_SYMBOL ) / asset( high_sbd, SBD_SYMBOL ); }

   uint32_t             seconds = 0;
   fc::time_point_sec   open;
   share_type           high_steem;
   share_type           high_sbd;
   share_type           low_steem;
   share_type           low_sbd;
   share_type           open_steem;
   share_type           open_sbd;
   share_type           close_steem;
   share_type           close_sbd;
   share_type           steem_volume;
   share_type           sbd_volume;
};

struct order_history_object : public abstract_object< order_history_object >
{
   uint64_t             sequence;
   fc::time_point_sec   time;
   fill_order_operation op;
};

struct by_id;
struct by_key;
typedef multi_index_container<
   bucket_object,
   indexed_by<
      hashed_unique< tag< by_id >, member< object, object_id_type, &object::id > >,
      ordered_unique< tag< by_key >, member< bucket_object, bucket_key, &bucket_object::key > >
   >
> bucket_object_multi_index_type;

struct by_sequence;
typedef multi_index_container<
   order_history_object,
   indexed_by<
      hashed_unique< tag< by_id >, member< object, object_id_type, &object::id > >,
      ordered_unique< tag< by_sequence >, member< order_history_object, uint64_t, &order_history_object::sequence > >
   >
> order_history_multi_index_type;

typedef generic_index< bucket_object, bucket_object_multi_index_type >        bucket_index;
typedef generic_index< order_history_object, order_history_multi_index_type > order_history_index;

} } // steemit::market_history

FC_REFLECT( steemit::market_history::bucket_key, (seconds)(open) )
FC_REFLECT_DERIVED( steemit::market_history::bucket_object, (graphene::db::object),
                     (key)
                     (high_base)(high_quote)
                     (low_base)(low_quote)
                     (open_base)(open_quote)
                     (close_base)(close_quote)
                     (base_volume)(quote_volume) )
FC_REFLECT_DERIVED( steemit::market_history::order_history_object, (graphene::db::object),
                     (sequence)
                     (time)
                     (op) )