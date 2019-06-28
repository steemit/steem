#pragma once

#include <dpn/protocol/block.hpp>

namespace dpn { namespace chain {

struct transaction_notification
{
   transaction_notification( const dpn::protocol::signed_transaction& tx ) : transaction(tx)
   {
      transaction_id = tx.id();
   }

   dpn::protocol::transaction_id_type          transaction_id;
   const dpn::protocol::signed_transaction&    transaction;
};

} }
