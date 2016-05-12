/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <steemit/app/plugin.hpp>

#include <fc/vector.hpp>

namespace steemit { namespace app {

template< typename PluginOps >
plugin< PluginOps >::plugin()
{
   _app = nullptr;
   return;
}

// Simply for backwards compatability
template<>
plugin< fc::static_variant<> >::plugin()
{
   _app = nullptr;
   return;
}

template< typename PluginOps >
plugin< PluginOps >::~plugin()
{
   return;
}

template<>
plugin< fc::static_variant<> >::~plugin()
{
   return;
}

template< typename PluginOps >
std::string plugin< PluginOps >::plugin_name()const
{
   return "<unknown plugin>";
}

template< typename PluginOps >
void plugin< PluginOps >::plugin_initialize( const boost::program_options::variables_map& options )
{
   return;
}

template< typename PluginOps >
void plugin< PluginOps >::plugin_startup()
{
   return;
}

template< typename PluginOps >
void plugin< PluginOps >::plugin_shutdown()
{
   return;
}

template< typename PluginOps >
void plugin< PluginOps >::plugin_set_app( application* app )
{
   _app = app;
   return;
}

template< typename PluginOps >
void plugin< PluginOps >::plugin_set_program_options(
   boost::program_options::options_description& command_line_options,
   boost::program_options::options_description& config_file_options
)
{
   return;
}

template< typename PluginOps >
void plugin< PluginOps >::plugin_push_op( string json_op )
{
   return;
}

template<> void plugin< fc::static_variant<> >::plugin_push_op( string json_op ) { FC_ASSERT( false, "This plugin has no custom ops" ); return; }

template<>
template< typename EvaluatorType > void plugin< fc::static_variant<> >::plugin_register_evaluator() { return; }

} } // steemit::app
