#pragma once
#include <steem/protocol/base.hpp>

#include <steem/protocol/asset.hpp>


namespace steem { namespace protocol {

struct create_proposal_operation : public base_operation
{
   account_name_type creator;
   /// Account to be paid if given proposal has sufficient votes.
   account_name_type receiver;

   time_point_sec start_date;
   time_point_sec end_date;

   /// Amount of SBDs to be daily paid to the `receiver` account.
   asset daily_pay;

   string subject;

   /// Given link shall be a valid permlink. Must be posted by creator or at least receiver.
   string permlink;

   void validate()const;

   void get_required_active_authorities( flat_set<account_name_type>& a )const { a.insert( creator ); }
};

struct update_proposal_votes_operation : public base_operation
{
   account_name_type voter;

   /// IDs of proposals to vote for/against. Nonexisting IDs are ignored.
   flat_set<int64_t> proposal_ids;

   bool approve = false;

   void validate()const;

   void get_required_active_authorities( flat_set<account_name_type>& a )const { a.insert( voter ); }
};

/** Allows to remove proposals specified by given IDs.
    Operation can be performed only by proposal owner. 
*/
struct remove_proposal_operation : public base_operation
{
   account_name_type proposal_owner;

   /// IDs of proposals to be removed. Nonexisting IDs are ignored.
   flat_set<int64_t> proposal_ids;

   void validate() const;

   void get_required_active_authorities(flat_set<account_name_type>& a)const { a.insert(proposal_owner); }
};

/** Dedicated operation to be generated during proposal payment phase to left info in AH related to
    funds transfer.
*/
struct proposal_pay_operation : public virtual_operation
{
   proposal_pay_operation() = default;
   proposal_pay_operation(const account_name_type r, const asset& p, transaction_id_type _trx_id, uint16_t _op_in_trx )
                     : receiver(r), payment(p), trx_id(_trx_id), op_in_trx(_op_in_trx) {}

   /// Name of the account which is paid for
   account_name_type receiver;
   /// Amount of SBDs paid.
   asset             payment;

   /// Transaction id + position of operation where appeared a proposal being a source of given operation.
   transaction_id_type trx_id;
   uint16_t op_in_trx = 0;
};

} } // steem::protocol

FC_REFLECT( steem::protocol::create_proposal_operation, (creator)(receiver)(start_date)(end_date)(daily_pay)(subject)(permlink) )
FC_REFLECT( steem::protocol::update_proposal_votes_operation, (voter)(proposal_ids)(approve) )
FC_REFLECT( steem::protocol::remove_proposal_operation, (proposal_owner)(proposal_ids) )

FC_REFLECT(steem::protocol::proposal_pay_operation, (receiver)(payment)(trx_id)(op_in_trx))

