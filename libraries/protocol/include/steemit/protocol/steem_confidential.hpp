#include <steemit/protocol/types.hpp>

namespace steemit { namespace protocol {

/**
 *  @ingroup stealth
 */
struct blind_input
{
   fc::ecc::commitment_type      commitment;
   account_name_type             owner;
};


/**
 *  When sending a stealth tranfer we assume users are unable to scan
 *  the full blockchain; therefore, payments require confirmation data
 *  to be passed out of band.   We assume this out-of-band channel is
 *  not secure and therefore the contents of the confirmation must be
 *  encrypted. 
 */
struct stealth_confirmation
{
   struct memo_data
   {
      public_key_type           from;
      asset                     amount;
      fc::sha256                blinding_factor;
      fc::ecc::commitment_type  commitment;
      string                    message;
      uint32_t                  check = 0;
   };

   /**
    *  Packs *this then encodes as base58 encoded string.
    */
   operator string()const;
   /**
    * Unpacks from a base58 string
    */
   stealth_confirmation( const std::string& base58 );
   stealth_confirmation(){}

   public_key_type           one_time_key; 
   public_key_type           to; ///< should match memo key of receiver at time of transfer
   vector<char>              encrypted_memo;
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
   fc::ecc::commitment_type                commitment;
   /** only required if there is more than one blind output */
   range_proof_type                        range_proof;
   account_name_type                       owner;
   stealth_confirmation                    stealth_memo;
};


/**
 *  @ingroup stealth
 *  @brief Transfers from blind to blind
 *
 *  There are two ways to transfer value while maintaining privacy:
 *  1. account to account with amount kept secret
 *  2. stealth transfers with amount sender/receiver kept secret
 *
 *  When doing account to account transfers, everyone with access to the
 *  memo key can see the amounts, but they will not have access to the funds.
 *
 *  When using stealth transfers the same key is used for control and reading
 *  the memo.
 *
 *  This operation is more expensive than a normal transfer and has
 *  a fee proportional to the size of the operation.
 *
 *  All assets in a blind transfer must be of the same type: fee.asset_id
 *  The fee_payer is the temp account and can be funded from the blinded values.
 *
 *  Using this operation you can transfer from an account and/or blinded balances
 *  to an account and/or blinded balances.
 *
 *  Stealth Transfers:
 *
 *  Assuming Receiver has key pair R,r and has shared public key R with Sender
 *  Assuming Sender has key pair S,s
 *  Generate one time key pair  O,o  as s.child(nonce) where nonce can be inferred from transaction
 *  Calculate secret V = o*R
 *  blinding_factor = sha256(V)
 *  memo is encrypted via aes of V
 *  owner = R.child(sha256(blinding_factor))
 *
 *  Sender gives Receiver output ID to complete the payment.
 *
 *  This process can also be used to send money to a cold wallet without having to
 *  pre-register any accounts.
 *
 *  Outputs are assigned the same IDs as the inputs until no more input IDs are available,
 *  in which case a the return value will be the *first* ID allocated for an output.  Additional
 *  output IDs are allocated sequentially thereafter.   If there are fewer outputs than inputs
 *  then the input IDs are freed and never used again.
 */
struct blind_transfer_operation : public base_operation
{
   account_name_type                 from;
   account_name_type                 to;
   asset                             to_public_amount;
   vector<fc::ecc::commitment_type>  inputs; /// must belong to from
   vector<blind_output>              outputs; /// can belong to many different people
    
   void  validate()const;
   void  get_required_active_authorities( flat_set<account_name_type>& a )const
   {
      a.insert(from);
   }
};

struct transfer_to_blind_operation : public base_operation {
   account_name_type            from;
   asset                        amount;
   fc::ecc::blind_factor_type   blinding_factor;
   vector<blind_output>         outputs;

   void  validate()const;
   void  get_required_active_authorities( flat_set<account_name_type>& a )const
   {
      a.insert(from);
   }
};

} } // steemit::protocol

FC_REFLECT( steemit::protocol::blind_transfer_operation, (from)(to)(to_public_amount)(inputs)(outputs) )
FC_REFLECT( steemit::protocol::blind_output, (commitment)(range_proof)(owner)(stealth_memo))
FC_REFLECT( steemit::protocol::stealth_confirmation, (one_time_key)(to)(encrypted_memo) )
FC_REFLECT( steemit::protocol::transfer_to_blind_operation, (from)(amount)(blinding_factor)(outputs) )

