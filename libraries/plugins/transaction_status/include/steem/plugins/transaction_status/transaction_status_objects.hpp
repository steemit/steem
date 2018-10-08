#pragma once
#include <steem/protocol/transaction.hpp>
//#include <steem/plugins/chain/chain_plugin.hpp>
#include <steem/chain/steem_object_types.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>

namespace steem { namespace plugins { namespace transaction_status {

using namespace steem::chain;
using namespace boost::multi_index;

#ifndef STEEM_TRANSACTION_STATUS_SPACE_ID
#define STEEM_TRANSACTION_STATUS_SPACE_ID 18
#endif

enum transaction_status_object_type
{
   transaction_status_object_type = ( STEEM_TRANSACTION_STATUS_SPACE_ID << 8 )
};

enum transaction_status
{
   unknown,    // Expiration time in future, transaction not included in block or mempool
   processing, // Transaction in mempool
   accepted,   // Transaction has been included in block, block not irreversible
   pending,    // Transaction has been included in block, block is irreversible
   // Transaction has expired, transaction is not irreversible (transaction could be in a fork)
   // Transaction has expired, transaction is irreversible (transaction cannot be in a fork)
   expired   // Transaction is too old, I don't know about it
};

class transaction_status_object : public object< transaction_status_object_type, transaction_status_object >
{
   transaction_status_object() = delete;

public:
   template< typename Constructor, typename Allocator >
   transaction_status_object( Constructor&& c, allocator< Allocator > a )
   {
      c( *this );
   }

   id_type                     id;
   transaction_id_type         trx_id;
   time_point_sec              expiration;
   uint32_t                    block_num;
   transaction_status          status;
};

typedef oid< transaction_status_object > transaction_status_object_id_type;

struct by_trx_id;
struct by_block_num;
struct by_expiration;

typedef multi_index_container<
   transaction_status_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< transaction_status_object, transaction_status_object_id_type, &transaction_status_object::id > >,
      hashed_unique< tag< by_trx_id >, BOOST_MULTI_INDEX_MEMBER(transaction_status_object, transaction_id_type, trx_id), std::hash<transaction_id_type> >,
      ordered_non_unique< tag< by_expiration >, member<transaction_status_object, time_point_sec, &transaction_status_object::expiration > >,
      ordered_non_unique< tag< by_block_num >, member< transaction_status_object, uint32_t, &transaction_status_object::block_num > >
   >,
   allocator< transaction_status_object >
> transaction_status_index;



} } } // steem::plugins::transaction_status

FC_REFLECT_ENUM( steem::plugins::transaction_status::transaction_status,
                (unknown)
                (processing)
                (accepted)
                (pending)
                (expired) )

FC_REFLECT( steem::plugins::transaction_status::transaction_status_object, (id)(trx_id)(expiration)(block_num)(status) )
CHAINBASE_SET_INDEX_TYPE( steem::plugins::transaction_status::transaction_status_object, steem::plugins::transaction_status::transaction_status_index )
