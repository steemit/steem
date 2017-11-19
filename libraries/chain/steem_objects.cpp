
#include <steem/chain/steem_objects.hpp>

#include <fc/uint128.hpp>

#include <fc/variant.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/json.hpp>
namespace steem { namespace chain {

} } // steem::chain


namespace chainbase
{
namespace helpers
{

   std::string ra_index_container_logging_dumper<steem::chain::comment_object>::dump(const steem::chain::comment_object& o)
   {
      auto jsonDump = fc::json::to_pretty_string(o);
      return jsonDump;
   }
}
}