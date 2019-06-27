#pragma once

#include <steem/protocol/block.hpp>

namespace steem { namespace chain {

struct transaction_notification
{
   transaction_notification( const steem::protocol::signed_transaction& tx ) : transaction(tx)
   {
      transaction_id = tx.id();
   }

   steem::protocol::transaction_id_type          transaction_id;
   const steem::protocol::signed_transaction&    transaction;
};

} }
