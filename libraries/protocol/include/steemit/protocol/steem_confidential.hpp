#pragma once

#include <steemit/protocol/types.hpp>

namespace steemit { namespace protocol {

struct balance_slot
{
   enum balance_slots
   {
      none = 0,
      checking = 1,
      savings = 2
   };
};

/**
 *  @class blind_output
 *  @brief Defines data required to create a new blind commitment
 *  @ingroup stealth
 *
 *  The blinded output that must be proven to be greater than 0
 */
struct blind_output
{
   blind_output(){}
   blind_output( fc::ecc::commitment_type c, range_proof_type r, account_name_type o, uint8_t s, stealth_confirmation sc )
   :commitment(c),range_proof(r),owner(o),savings(s),stealth_memo(sc) {}

   fc::ecc::commitment_type                commitment;
   /** only required if there is more than one blind output */
   range_proof_type                        range_proof;
   account_name_type                       owner;
   /**
    * blind outputs held as savings can only be spent with a 3 day delay during which they may be canceled
    */
   uint8_t                                 balance_slot = 0;
   vector<char>                            stealth_memo;
};


struct public_amount
{
   account_name_type             account;
   asset                         amount;
   uint8_t                       balance_slot = none;
};

/**
 *  @ingroup stealth
 *  @brief Transfers from blind to blind
 *
 *  If any of the inputs are in state 'savings' then the inputs and outputs will be marked 'pending' and
 *  a new confirmation object will be created.  This operation can be canceled using the commitment ID of
 *  the first input.
 */
struct blind_transfer_operation : public base_operation
{
   enum available_flags
   {
      defer = 1
   }

   public_amount                     from;
   optional< public_amount >         to;

   vector<fc::ecc::commitment_type>  inputs;  /// must belong to from
   vector<blind_output>              outputs; /// can belong to many different people

   uint8_t                           flags = 0;

   void  validate()const;
   void  get_required_active_authorities( flat_set<account_name_type>& a )const
   {
      a.insert(from);
   }

   asset net_public()const
   {
      if( !to.valid() )
         return -from.amount;
      return to->amount - from.amount;
   }
};

struct cancel_blind_transfer_operation : public base_operation
{
   account_name_type                 from;
   fc::ecc::commitment_type          pending_input; ///< first input commitment in the blind_transfer operation

   void get_required_active_authorities( flat_set<account_name_type>& a )const
   {
      a.insert(from);
   }
};

} } // steemit::protocol

FC_REFLECT( steemit::protocol::blind_output, (commitment)(range_proof)(owner)(balance_slot)(stealth_memo))
FC_REFLECT( steemit::protocol::stealth_confirmation, (one_time_key)(to)(encrypted_memo) )
FC_REFLECT( steemit::protocol::public_amount, (account)(amount)(balance_slot) )

FC_REFLECT( steemit::protocol::blind_transfer_operation, (from)(to)(inputs)(outputs)(flags) )
FC_REFLECT( steemit::protocol::cancel_blind_transfer_operation, (from)(pending_input) )

