#pragma once

#include <map>
#include <string>
#include <utility>

#include <fc/api.hpp>

namespace fc { namespace rpc {

class websocket_api_connection;

} }

namespace steemit { namespace app {

class application;

/**
 * Contains state shared by all API's on the same connection.
 * Anything in here is owned by FC and cleaned up when the connection dies.
 */

struct api_session_data
{
   std::shared_ptr< fc::rpc::websocket_api_connection >        wsc;
   std::map< std::string, fc::api_ptr >                        api_map;
};

/**
 * Contains information needed by API class constructors.
 */

struct api_context
{
   api_context( application& _app, const std::string& _api_name, std::weak_ptr< api_session_data > _session );

   application&                              app;
   std::string                               api_name;
   std::weak_ptr< api_session_data >         session;
};

} }
