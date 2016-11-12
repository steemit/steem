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
   uint8_t                                 savings = 0; 
   stealth_confirmation                    stealth_memo;
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

struct cancel_blind_transfer_operation : public base_operation {
   account_name_type                 from;
   fc::ecc::commitment_type          pending_input; ///< first input commitment in the blind_transfer operation
   void  get_required_active_authorities( flat_set<account_name_type>& a )const
   {
      a.insert(from);
   }
};

struct transfer_to_blind_operation : public base_operation {
   account_name_type            from;
   asset                        amount;
   fc::ecc::blind_factor_type   blinding_factor; ///< TODO: can we make this a constant of some sort?
   vector<blind_output>         outputs;

   void  validate()const;
   void  get_required_active_authorities( flat_set<account_name_type>& a )const
   {
      a.insert(from);
   }
};

} } // steemit::protocol

FC_REFLECT( steemit::protocol::blind_transfer_operation, (from)(to)(to_public_amount)(inputs)(outputs) )
FC_REFLECT( steemit::protocol::blind_output, (commitment)(range_proof)(owner)(savings)(stealth_memo))
FC_REFLECT( steemit::protocol::stealth_confirmation, (one_time_key)(to)(encrypted_memo) )
FC_REFLECT( steemit::protocol::transfer_to_blind_operation, (from)(amount)(blinding_factor)(outputs) )
FC_REFLECT( steemit::protocol::cancel_blind_transfer_operation, (from)(pending_input) )

