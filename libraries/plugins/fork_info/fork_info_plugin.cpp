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

#include <steemit/fork_info/fork_info_plugin.hpp>

#include <steemit/protocol/types.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/app/api.hpp>
#include <steemit/app/database_api.hpp>

#include <fc/network/http/websocket.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/api.hpp>
#include <fc/smart_ref_impl.hpp>

namespace steemit { namespace plugin { namespace fork_info {

namespace detail {
   using namespace steemit::protocol;

   class fork_info_plugin_impl
   {
      public:
         fork_info_plugin_impl( fork_info_plugin& plugin )
            :_self( plugin ) {}
         virtual ~fork_info_plugin_impl() {}

         void on_block( const signed_block& b );

         fork_info_plugin&       _self;
         std::deque<signed_block> _last_blocks;
   };
   void fork_info_plugin_impl::on_block( const signed_block& b )
   {
      auto& db = _self.database();
      _last_blocks.push_front(b);
      if( _last_blocks.size() == 21)
      {
         fc::optional<signed_block> result = db.fetch_block_by_number(_last_blocks.at(20).block_num());
         if( result.valid() ){
            signed_block block = *result;
            if(block.witness_signature != _last_blocks.at(20).witness_signature && block.id() != _last_blocks.at(20).id()){
               idump(("Orphaned Block: ")(_last_blocks.at(20)));
            }
         }
         _last_blocks.pop_back();

      }
   }
}

fork_info_plugin::fork_info_plugin( application* app )
   : plugin( app ), _my( new detail::fork_info_plugin_impl( *this ))
{}

fork_info_plugin::~fork_info_plugin()
{}

void fork_info_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   try
   {
      chain::database& db = database();
      db.applied_block.connect([this](const chain::signed_block& b){ _my->on_block( b ); });
   } FC_CAPTURE_AND_RETHROW()
}

void fork_info_plugin::plugin_startup()
{}
} } }
