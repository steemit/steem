
#pragma once

#include <steemit/protocol/types.hpp>

#include <fc/api.hpp>
#include <fc/crypto/sha256.hpp>

#include <string>

namespace steemit { namespace app {
   struct api_context;
} }

namespace steemit { namespace plugin { namespace auth_util {

namespace detail {
class auth_util_api_impl;
}

struct check_authority_signature_params
{
   std::string                          account_name;
   std::string                          level;
   fc::sha256                           dig;
   std::vector< protocol::signature_type > sigs;
};

struct check_authority_signature_result
{
   std::vector< protocol::public_key_type > keys;
};

class auth_util_api
{
   public:
      auth_util_api( const steemit::app::api_context& ctx );

      void on_api_startup();

      check_authority_signature_result check_authority_signature( check_authority_signature_params args );

   private:
      std::shared_ptr< detail::auth_util_api_impl > my;
};

} } }

FC_REFLECT( steemit::plugin::auth_util::check_authority_signature_params,
   (account_name)
   (level)
   (dig)
   (sigs)
   )
FC_REFLECT( steemit::plugin::auth_util::check_authority_signature_result,
   (keys)
   )

FC_API( steemit::plugin::auth_util::auth_util_api,
   (check_authority_signature)
   )
