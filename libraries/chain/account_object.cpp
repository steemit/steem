#include <steem/chain/account_object.hpp>
#include <steem/chain/database.hpp>
#ifdef STEEM_ENABLE_SMT
#include <steem/chain/smt_objects/smt_token_object.hpp>

namespace steem { namespace chain {

const smt_token_object& account_object::get_controlled_smt(const asset_symbol_type& smt_symbol, database& db) const
{
   const smt_token_object* smt = db.find< smt_token_object, by_symbol >( smt_symbol );
   // The SMT is supposed to be found.
   FC_ASSERT( smt != nullptr, "SMT numerical asset identifier ${smt} not found", ("smt", smt_symbol.to_nai()) );
   // Check against unotherized account trying to access (and alter) SMT. Unotherized means "other than the one that created the SMT".
   FC_ASSERT( smt->control_account == name, "The account ${account} does NOT control the SMT ${smt}",
      ("account", name)("smt", smt_symbol.to_nai()) );
   
   return *smt;
}

} } // steem::chain

#endif