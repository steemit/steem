#pragma once
#include <steemit/chain/account_object.hpp>
#include <steemit/chain/block_summary_object.hpp>
#include <steemit/chain/comment_object.hpp>
#include <steemit/chain/global_property_object.hpp>
#include <steemit/chain/history_object.hpp>
#include <steemit/chain/steem_objects.hpp>
#include <steemit/chain/transaction_object.hpp>
#include <steemit/chain/witness_objects.hpp>
#include <steemit/chain/database.hpp>

namespace steemit { namespace plugins { namespace database_ext_api {

using namespace steemit::chain;

struct api_signed_block_object : public signed_block
{
   api_signed_block_object( const signed_block& block ) : signed_block( block )
   {
      block_id = id();
      signing_key = signee();
      transaction_ids.reserve( transactions.size() );
      for( const signed_transaction& tx : transactions )
         transaction_ids.push_back( tx.id() );
   }
   api_signed_block_object() {}

   block_id_type                 block_id;
   public_key_type               signing_key;
   vector< transaction_id_type > transaction_ids;
};

} } } // steemit::plugins::database_api

FC_REFLECT_DERIVED( steemit::plugins::database_ext_api::api_signed_block_object, (steemit::protocol::signed_block),
                     (block_id)
                     (signing_key)
                     (transaction_ids)
                  )
