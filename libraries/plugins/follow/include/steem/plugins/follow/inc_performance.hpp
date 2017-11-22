#pragma once

#include <steem/protocol/types.hpp>
#include <steem/chain/steem_object_types.hpp>
#include <steem/chain/database.hpp>

namespace steem { namespace plugins{ namespace follow {

using namespace steem::chain;
using steem::chain::database;
 
using steem::protocol::account_name_type;

class performance_impl;

class performance
{

   private:

      std::unique_ptr< performance_impl > my;

   public:
   
      performance( database& _db );
      ~performance();

      template< typename MultiContainer, typename Index >
      void delete_old_objects( const account_name_type& start_account, uint32_t next_id, uint32_t max_size ) const;
};

} } } //steem::follow
