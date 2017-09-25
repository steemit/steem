#include <steem/chain/account_object.hpp>

#include <steem/chain/database.hpp>

namespace steem
{
namespace chain
{
   #ifdef STEEM_ENABLE_SMT
   void account_object::finalize_elevation(database& db) const
   {
      FC_ASSERT(is_elevated() == nullptr);

      const smt_token_object& newToken = db.create< smt_token_object >(
         [&]( smt_token_object& token )
         {
         });

      db.modify(*this, [&newToken](account_object& account)
      {
         account.associatedToken = newToken;
      }
   );
   }
   #endif /// STEEM_ENABLE_SMT
} /// namespace chain
} /// namespace steem