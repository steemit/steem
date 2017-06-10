#!/usr/bin/env python3

templates = {
"plugin.json" :
"""{{
   "plugin_name": "{plugin_name}",
   "plugin_project": "{plugin_provider}_{plugin_name}"
}}
""",

"CMakeLists.txt" :
"""file(GLOB HEADERS "include/{plugin_provider}/plugins/{plugin_name}/*.hpp")

add_library( {plugin_provider}_{plugin_name}
             ${{HEADERS}}
             {plugin_name}_plugin.cpp
             {plugin_name}_api.cpp
           )

target_link_libraries( {plugin_provider}_{plugin_name} steemit_app steemit_chain steemit_protocol )
target_include_directories( {plugin_provider}_{plugin_name}
                            PUBLIC "${{CMAKE_CURRENT_SOURCE_DIR}}/include" )
""",

"include/{plugin_provider}/plugins/{plugin_name}/{plugin_name}_api.hpp" :
"""
#pragma once

#include <fc/api.hpp>

namespace steemit {{ namespace app {{
   struct api_context;
}} }}

namespace {plugin_provider} {{ namespace plugin {{ namespace {plugin_name} {{

namespace detail {{
class {plugin_name}_api_impl;
}}

class {plugin_name}_api
{{
   public:
      {plugin_name}_api( const steemit::app::api_context& ctx );

      void on_api_startup();

      // TODO:  Add API methods here

   private:
      std::shared_ptr< detail::{plugin_name}_api_impl > my;
}};

}} }} }}

FC_API( {plugin_provider}::plugin::{plugin_name}::{plugin_name}_api,
   // TODO:  Add method bubble list here
   )
""",

"include/{plugin_provider}/plugins/{plugin_name}/{plugin_name}_plugin.hpp" :
"""
#pragma once

#include <steemit/app/plugin.hpp>

namespace {plugin_provider} {{ namespace plugin {{ namespace {plugin_name} {{

namespace detail {{
class {plugin_name}_plugin_impl;
}}

class {plugin_name}_plugin : public steemit::app::plugin
{{
   public:
      {plugin_name}_plugin( steemit::app::application* app );
      virtual ~{plugin_name}_plugin();

      virtual std::string plugin_name()const override;
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;

   private:
      std::shared_ptr< detail::{plugin_name}_plugin_impl > my;
}};

}} }} }}
""",

"{plugin_name}_api.cpp" :
"""
#include <steemit/app/api_context.hpp>
#include <steemit/app/application.hpp>

#include <{plugin_provider}/plugins/{plugin_name}/{plugin_name}_api.hpp>
#include <{plugin_provider}/plugins/{plugin_name}/{plugin_name}_plugin.hpp>

namespace {plugin_provider} {{ namespace plugin {{ namespace {plugin_name} {{

namespace detail {{

class {plugin_name}_api_impl
{{
   public:
      {plugin_name}_api_impl( steemit::app::application& _app );

      std::shared_ptr< {plugin_provider}::plugin::{plugin_name}::{plugin_name}_plugin > get_plugin();

      steemit::app::application& app;
}};

{plugin_name}_api_impl::{plugin_name}_api_impl( steemit::app::application& _app ) : app( _app )
{{}}

std::shared_ptr< {plugin_provider}::plugin::{plugin_name}::{plugin_name}_plugin > {plugin_name}_api_impl::get_plugin()
{{
   return app.get_plugin< {plugin_name}_plugin >( "{plugin_name}" );
}}

}} // detail

{plugin_name}_api::{plugin_name}_api( const steemit::app::api_context& ctx )
{{
   my = std::make_shared< detail::{plugin_name}_api_impl >(ctx.app);
}}

void {plugin_name}_api::on_api_startup() {{ }}

}} }} }} // {plugin_provider}::plugin::{plugin_name}
""",

"{plugin_name}_plugin.cpp" :
"""

#include <{plugin_provider}/plugins/{plugin_name}/{plugin_name}_api.hpp>
#include <{plugin_provider}/plugins/{plugin_name}/{plugin_name}_plugin.hpp>

#include <string>

namespace {plugin_provider} {{ namespace plugin {{ namespace {plugin_name} {{

namespace detail {{

class {plugin_name}_plugin_impl
{{
   public:
      {plugin_name}_plugin_impl( steemit::app::application& app );
      virtual ~{plugin_name}_plugin_impl();

      virtual std::string plugin_name()const;
      virtual void plugin_initialize( const boost::program_options::variables_map& options );
      virtual void plugin_startup();
      virtual void plugin_shutdown();
      void on_applied_block( const chain::signed_block& b );

      steemit::app::application& _app;
      boost::signals2::scoped_connection _applied_block_conn;
}};

{plugin_name}_plugin_impl::{plugin_name}_plugin_impl( steemit::app::application& app )
  : _app(app) {{}}

{plugin_name}_plugin_impl::~{plugin_name}_plugin_impl() {{}}

std::string {plugin_name}_plugin_impl::plugin_name()const
{{
   return "{plugin_name}";
}}

void {plugin_name}_plugin_impl::plugin_initialize( const boost::program_options::variables_map& options )
{{
}}

void {plugin_name}_plugin_impl::plugin_startup()
{{
   _app.register_api_factory< {plugin_name}_api >( "{plugin_name}_api" );
   _applied_block_conn = _app.chain_database()->applied_block.connect(
      [this](const chain::signed_block& b){{ on_applied_block(b); }});
}}

void {plugin_name}_plugin_impl::plugin_shutdown()
{{
}}

void {plugin_name}_plugin_impl::on_applied_block( const chain::signed_block& b )
{{
}}

}}

{plugin_name}_plugin::{plugin_name}_plugin( steemit::app::application* app )
   : plugin(app)
{{
   FC_ASSERT( app != nullptr );
   my = std::make_shared< detail::{plugin_name}_plugin_impl >( *app );
}}

{plugin_name}_plugin::~{plugin_name}_plugin() {{}}

std::string {plugin_name}_plugin::plugin_name()const
{{
   return my->plugin_name();
}}

void {plugin_name}_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{{
   my->plugin_initialize( options );
}}

void {plugin_name}_plugin::plugin_startup()
{{
   my->plugin_startup();
}}

void {plugin_name}_plugin::plugin_shutdown()
{{
   my->plugin_shutdown();
}}

}} }} }} // {plugin_provider}::plugin::{plugin_name}

STEEMIT_DEFINE_PLUGIN( {plugin_name}, {plugin_provider}::plugin::{plugin_name}::{plugin_name}_plugin )
""",
}

import argparse
import os
import sys

def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("provider", help="Name of plugin provider (steemit for plugins developed by Steemit)")
    parser.add_argument("name", help="Name of plugin to create")
    args = parser.parse_args(argv[1:])
    ctx = {
           "plugin_provider" : args.provider,
           "plugin_name" : args.name,
          }

    outdir = os.path.join("libraries", "plugins", ctx["plugin_name"])
    for t_fn, t_content in templates.items():
        content = t_content.format(**ctx)
        fn = os.path.join(outdir, t_fn.format(**ctx))
        dn = os.path.dirname(fn)
        if not os.path.exists(dn):
            os.makedirs(dn)
        with open(fn, "w") as f:
            f.write(content)

    return

if __name__ == "__main__":
    main(sys.argv)
