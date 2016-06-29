#pragma once

namespace steemit { namespace sidechain {
  using std::string;

#define side_ids 10

  typedef side_object_types {
     side_account_type = 1,
     pending_transaction_object = 2
  };


  /** This operation can be used for internal transfers and withdraw requests */
  struct sidechain_transfer_operation {
     string  sidechain;
     string  from;  /// must be in required active signatures
     string  to;  
     asset   amount;
     string  memo;
  };

  /** This operation can be used for broadcasting signatures to apply to pending
   *  sidechain transactions.
   */
  struct sidechain_sign_operation {
     string                   sidechain;
     string                   signer;  ///< must be in required active signatures 
     transaction_id_type      trx_id;
     set<signature_type>      signatures;
  };

  FC_REFLECT( sidechain_transfer_operation, (sidechain)(from)(to)(amount)(memo) )

  class side_account_object : public abstract_object<side_account_object> {
     public:
       static const uint8_t space_id = side_ids;
       static const uint8_t type_id  = side_account_type;

        string   sidechain; ///< name of the sidechain account
        string   name; /// sub account within the side chain

        asset    steem_balance;
        asset    dollar_balance;
  };

  FC_REFLECT_DERIVED( steemit::sidechain::side_account, (graphene::db::object),
                      (sidechain)(name)(steem_balance)(dollar_balance)
                     );


  /**
   *  This index maintains a database of transactions that are 
   */
  class pending_transaction_object : public abstract_object<pending_transaction_object> {
     public:
        static const uint8_t space_id = side_ids;
        static const uint8_t type_id  = side_account_type;

        string                sidechain; ///< name of the sidechain account
        bool                  approved;   ///< whether or not the transaction has been confirmed
        time_point_sec        expiration; ///< when the transaction expires
        transaction_id_type   trx_id;
        signed_transaction    trx;
  };



} }
