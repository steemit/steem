
#pragma once

#include <steemit/chain/protocol/block_header.hpp>
#include <steemit/chain/protocol/block.hpp>
#include <steemit/chain/protocol/transaction.hpp>
#include <steemit/chain/protocol/types.hpp>
#include <steemit/chain/protocol/steem_operations.hpp>

#include <steemit/chain/global_property_object.hpp>

#include <fc/api.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>

namespace steemit { namespace app {
class application;
} }

namespace steemit { namespace chain {
class db;
} }

namespace steemit { namespace plugin { namespace core_info {

namespace detail { class core_info_api_impl; }

class core_info_api
{
   public:
      core_info_api( const chain::database& db );
      core_info_api( const app::application& app );

      struct get_block_header_args { uint32_t block_num; };
      struct get_block_header_ret  { fc::optional< chain::block_header > block; };

      /**
       * @brief Retrieve a block header
       * @param block_num Height of the block whose header should be returned
       * @return header of the referenced block, or null if no matching block was found
       */
      get_block_header_ret get_block_header( get_block_header_args args )const;

      struct get_block_args { uint32_t block_num; };
      struct get_block_ret  { fc::optional< chain::signed_block > block; };

      /**
       * @brief Retrieve a full, signed block
       * @param block_num Height of the block to be returned
       * @return the referenced block, or null if no matching block was found
       */
      get_block_ret get_block( get_block_args args )const;

      struct get_transaction_args { chain::transaction_id_type txid; };
      struct get_transaction_ret  { fc::optional< chain::annotated_signed_transaction > tx; };

      get_transaction_ret get_transaction( get_transaction_args args )const;

      struct get_config_args {  };
      struct get_config_ret  { fc::variant_object config; };

      /**
       * @brief Retrieve compile-time constants
       */
      get_config_ret get_config( get_config_args args )const;

      struct get_dynamic_global_properties_args {  };
      struct get_dynamic_global_properties_ret  { chain::dynamic_global_property_object dgp; };

      /**
       * @brief Retrieve the current @ref dynamic_global_property_object
       */
      get_dynamic_global_properties_ret get_dynamic_global_properties( get_dynamic_global_properties_args args )const;

      struct get_chain_properties_args {  };
      struct get_chain_properties_ret  { chain::chain_properties params; };

      get_chain_properties_ret get_chain_properties( get_chain_properties_args args )const;

   private:
      std::shared_ptr< detail::core_info_api_impl > _my;
};

} } }

FC_REFLECT( steemit::plugin::core_info::core_info_api::get_block_header_args, (block_num) )
FC_REFLECT( steemit::plugin::core_info::core_info_api::get_block_header_ret, (block) )

FC_REFLECT( steemit::plugin::core_info::core_info_api::get_block_args, (block_num) )
FC_REFLECT( steemit::plugin::core_info::core_info_api::get_block_ret, (block) )

FC_REFLECT( steemit::plugin::core_info::core_info_api::get_transaction_args, (txid) )
FC_REFLECT( steemit::plugin::core_info::core_info_api::get_transaction_ret, (tx) )

FC_REFLECT( steemit::plugin::core_info::core_info_api::get_config_args,  )
FC_REFLECT( steemit::plugin::core_info::core_info_api::get_config_ret, (config) )

FC_REFLECT( steemit::plugin::core_info::core_info_api::get_dynamic_global_properties_args,  )
FC_REFLECT( steemit::plugin::core_info::core_info_api::get_dynamic_global_properties_ret, (dgp) )

FC_REFLECT( steemit::plugin::core_info::core_info_api::get_chain_properties_args,  )
FC_REFLECT( steemit::plugin::core_info::core_info_api::get_chain_properties_ret, (params) )

FC_API( steemit::plugin::core_info::core_info_api,
   (get_block_header)
   (get_block)
   (get_dynamic_global_properties)
   (get_chain_properties)
   )
