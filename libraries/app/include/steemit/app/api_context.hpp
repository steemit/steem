#pragma once

#include <map>
#include <string>
#include <utility>

#include <fc/api.hpp>

namespace steemit { namespace app {

class application;

/**
 * Contains state shared by all API's on the same connection.
 */

struct api_connection_context
{
   std::map< std::string, fc::api_ptr >     api_map;
};

/**
 * Contains information needed by API class constructors.
 */

struct api_context
{
   api_context( application& _app, const std::string& _api_name, std::weak_ptr< api_connection_context > _connection );

   application&                              app;
   std::string                               api_name;
   std::shared_ptr< api_connection_context > connection;
};

} }
