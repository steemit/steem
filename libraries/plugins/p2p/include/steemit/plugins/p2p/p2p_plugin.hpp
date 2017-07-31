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

#include <appbase/application.hpp>

#define P2P_PLUGIN_NAME "p2p"

namespace steemit { namespace plugins { namespace p2p {
namespace bpo = boost::program_options;

class p2p_plugin : public appbase::plugin<p2p_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES((plugins::chain::chain_plugin))

   p2p_plugin();
   virtual ~p2p_plugin();

   virtual void set_program_options(bpo::options_description &,
                                    bpo::options_description &config_file_options) override;

   virtual void plugin_initialize(const bpo::variables_map& options);
   virtual void plugin_startup();
   virtual void plugin_shutdown();

   void broadcast_block( const steemit::protocol::signed_block& block );
   void broadcast_transaction( const steemit::protocol::signed_transaction& tx );
   void set_block_production( bool producing_blocks );

private:
   std::unique_ptr<class p2p_plugin_impl> my;
};

} } } // steemit::plugins::p2p
