#pragma once
#include <steem/plugins/chain/chain_plugin.hpp>

#include <steem/chain/steem_object_types.hpp>

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
#ifndef STEEM_MARKET_HISTORY_SPACE_ID
#define STEEM_MARKET_HISTORY_SPACE_ID 7
#endif

#ifndef STEEM_MARKET_HISTORY_PLUGIN_NAME
#define STEEM_MARKET_HISTORY_PLUGIN_NAME "market_history"
#endif


namespace steem { namespace plugins { namespace market_history {

using namespace steem::chain;
using namespace appbase;

enum market_history_object_types
{
   bucket_object_type        = ( STEEM_MARKET_HISTORY_SPACE_ID << 8 ),
   order_history_object_type = ( STEEM_MARKET_HISTORY_SPACE_ID << 8 ) + 1
};

namespace detail { class market_history_plugin_impl; }

class market_history_plugin : public plugin< market_history_plugin >
{
   public:
      market_history_plugin();
      virtual ~market_history_plugin();

      APPBASE_PLUGIN_REQUIRES( (steem::plugins::chain::chain_plugin) )

      static const std::string& name() { static std::string name = STEEM_MARKET_HISTORY_PLUGIN_NAME; return name; }

      flat_set< uint32_t > get_tracked_buckets() const;
      uint32_t get_max_history_per_bucket() const;

      virtual void set_program_options(
         options_description& cli,
         options_description& cfg ) override;

   protected:
      virtual void plugin_initialize( const variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;

   private:
      friend class detail::market_history_plugin_impl;
      std::unique_ptr< detail::market_history_plugin_impl > my;
};

struct bucket_object_details
{
   share_type           high;
   share_type           low;
   share_type           open;
   share_type           close;
   share_type           volume;

   void fill( const share_type& val )
   {
      high = val;
      low = val;
      open = val;
      close = val;
      volume = val;
   }
};

struct bucket_object : public object< bucket_object_type, bucket_object >
{
   template< typename Constructor, typename Allocator >
   bucket_object( Constructor&& c, allocator< Allocator > a )
   {
      c( *this );
   }

   bucket_object() {}

   id_type              id;

   fc::time_point_sec   open;
   uint32_t             seconds = 0;

   bucket_object_details steem;

#ifdef STEEM_ENABLE_SMT
   asset_symbol_type symbol = SBD_SYMBOL;

   bucket_object_details sbd_smt;

   price high()const { return asset( sbd_smt.high, symbol ) / asset( steem.high, STEEM_SYMBOL ); }
   price low()const { return asset( sbd_smt.low, symbol ) / asset( steem.low, STEEM_SYMBOL ); }
#else
   bucket_object_details sbd;

   price high()const { return asset( sbd.high, SBD_SYMBOL ) / asset( steem.high, STEEM_SYMBOL ); }
   price low()const { return asset( sbd.low, SBD_SYMBOL ) / asset( steem.low, STEEM_SYMBOL ); }
#endif
};

typedef oid< bucket_object > bucket_id_type;


struct order_history_object : public object< order_history_object_type, order_history_object >
{
   template< typename Constructor, typename Allocator >
   order_history_object( Constructor&& c, allocator< Allocator > a )
   {
      c( *this );
   }

   id_type                          id;

   fc::time_point_sec               time;
   protocol::fill_order_operation   op;
};

typedef oid< order_history_object > order_history_id_type;


struct by_bucket;
typedef multi_index_container<
   bucket_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< bucket_object, bucket_id_type, &bucket_object::id > >,
      ordered_unique< tag< by_bucket >,
         composite_key< bucket_object,
            member< bucket_object, uint32_t, &bucket_object::seconds >,
            member< bucket_object, fc::time_point_sec, &bucket_object::open >
         >,
         composite_key_compare< std::less< uint32_t >, std::less< fc::time_point_sec > >
      >
   >,
   allocator< bucket_object >
> bucket_index;

struct by_time;
typedef multi_index_container<
   order_history_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< order_history_object, order_history_id_type, &order_history_object::id > >,
      ordered_non_unique< tag< by_time >, member< order_history_object, time_point_sec, &order_history_object::time > >
   >,
   allocator< order_history_object >
> order_history_index;

} } } // steem::plugins::market_history

FC_REFLECT( steem::plugins::market_history::bucket_object_details,
            (high)
            (low)
            (open)
            (close)
            (volume) )

FC_REFLECT( steem::plugins::market_history::bucket_object,
                     (id)
                     (open)(seconds)
#if defined STEEM_ENABLE_SMT
                     (steem)(symbol)(sbd_smt)
#else
                     (steem)(sbd)
#endif
         )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::market_history::bucket_object, steem::plugins::market_history::bucket_index )

FC_REFLECT( steem::plugins::market_history::order_history_object,
                     (id)
                     (time)
                     (op) )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::market_history::order_history_object, steem::plugins::market_history::order_history_index )
