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
#pragma once
#include <steemit/plugins/chain/chain_plugin.hpp>

#define STEEM_ACCOUNT_HISTORY_PLUGIN_NAME "account_history"

namespace steemit { namespace plugins { namespace account_history {

namespace detail { class account_history_plugin_impl; }

using namespace appbase;
using steemit::protocol::account_name_type;

//
// Plugins should #define their SPACE_ID's so plugins with
// conflicting SPACE_ID assignments can be compiled into the
// same binary (by simply re-assigning some of the conflicting #defined
// SPACE_ID's in a build script).
//
// Assignment of SPACE_ID's cannot be done at run-time because
// various template automagic depends on them being known at compile
// time.
//
#ifndef STEEM_ACCOUNT_HISTORY_SPACE_ID
#define STEEM_ACCOUNT_HISTORY_SPACE_ID 5
#endif

/**
 *  This plugin is designed to track a range of operations by account so that one node
 *  doesn't need to hold the full operation history in memory.
 */
class account_history_plugin : public plugin< account_history_plugin >
{
   public:
      account_history_plugin();
      virtual ~account_history_plugin();

      APPBASE_PLUGIN_REQUIRES( (steemit::plugins::chain::chain_plugin) )

      static const std::string& name() { static std::string name = STEEM_ACCOUNT_HISTORY_PLUGIN_NAME; return name; }

      void set_program_options(
         options_description& cli,
         options_description& cfg );
      void plugin_initialize( const variables_map& options );
      void plugin_startup();
      void plugin_shutdown();

      flat_map< account_name_type, account_name_type > tracked_accounts()const; /// map start_range to end_range

      std::unique_ptr< detail::account_history_plugin_impl > my;
};

} } } //steemit::plugins::account_history

