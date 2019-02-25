#pragma once

#include <steem/chain/database.hpp>
#include <boost/container/flat_set.hpp>

namespace steem { namespace chain {

using boost::container::flat_set;

class sps_helper
{
   private:

      template< bool IS_CHECK >
      static void remove_proposals_impl( database& db, const flat_set<int64_t>& proposal_ids, const account_name_type& proposal_owner );

   public:

      static void remove_proposals( database& db, const flat_set<int64_t>& proposal_ids, const account_name_type& proposal_owner );
      static void remove_proposals( database& db, const flat_set<int64_t>& proposal_ids );
};

} } // steem::chain
