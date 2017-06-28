#include <steemit/plugins/chain_api/chain_api_plugin.hpp>

//#include <steemit/protocol/types.hpp>
#include <steemit/protocol/exceptions.hpp>

#include <fc/io/json.hpp>

#define CALL(api_name, api_handle, api_namespace, call_name) \
{std::string("/v1/" #api_name "/" #call_name), \
   [this, api_handle](string, string body, plugins::http::url_response_callback cb) mutable { \
          try { \
             if (body.empty()) body = "{}"; \
             auto result = api_handle.call_name(fc::json::from_string(body).as<api_namespace::call_name ## _params>()); \
             cb(200, fc::json::to_string(result)); \
          } catch (fc::eof_exception) { \
             cb(400, "Invalid arguments"); \
             elog("Unable to parse arguments: ${args}", ("args", body)); \
          } catch (fc::exception& e) { \
             cb(500, e.what()); \
             elog("Exception encountered while processing ${call}: ${e}", ("call", #api_name "." #call_name)("e", e)); \
          } \
       }}

#define CHAIN_RO_CALL(call_name) CALL(chain, ro_api, plugins::chain::chain_apis::read_only, call_name)
#define CHAIN_RW_CALL(call_name) CALL(chain, rw_api, plugins::chain::chain_apis::read_write, call_name)

namespace steemit { namespace plugins { namespace chain_api {

void chain_api_plugin::plugin_startup()
{
   auto ro_api = app().get_plugin< plugins::chain::chain_plugin >().get_read_only_api();
   auto rw_api = app().get_plugin< plugins::chain::chain_plugin >().get_read_write_api();

   app().get_plugin< plugins::http::http_plugin >().add_api(
   {
      CHAIN_RO_CALL(get_info),
      CHAIN_RO_CALL(get_block),
      CHAIN_RW_CALL(push_block),
      CHAIN_RW_CALL(push_transaction)
   });
}

void chain_api_plugin::plugin_shutdown() {}

} } } // steemit::plugins::chain_api
