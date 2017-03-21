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

#include <steemit/app/plugin.hpp>
#include <steemit/chain/database.hpp>

#include <fc/thread/future.hpp>

namespace steemit { namespace witness_plugin {

using std::string;
using protocol::public_key_type;
using app::application;
using steemit::protocol::block_id_type;

namespace block_production_condition
{
   enum block_production_condition_enum
   {
      produced = 0,
      not_synced = 1,
      not_my_turn = 2,
      not_time_yet = 3,
      no_private_key = 4,
      low_participation = 5,
      lag = 6,
      consecutive = 7,
      wait_for_genesis = 8,
      exception_producing_block = 9
   };
}

namespace detail
{
   class witness_plugin_impl;
}

class witness_plugin : public steemit::app::plugin
{
public:
   witness_plugin( application* app );
   virtual ~witness_plugin();

   std::string plugin_name()const override;

   virtual void plugin_set_program_options(
      boost::program_options::options_description &command_line_options,
      boost::program_options::options_description &config_file_options
      ) override;

   void set_block_production(bool allow) { _production_enabled = allow; }

   virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;

private:
   void schedule_production_loop();
   block_production_condition::block_production_condition_enum block_production_loop();
   block_production_condition::block_production_condition_enum maybe_produce_block( fc::mutable_variant_object& capture );

   boost::program_options::variables_map _options;
   bool _production_enabled = false;
   uint32_t _required_witness_participation = 33 * STEEMIT_1_PERCENT;
   uint32_t _production_skip_flags = steemit::chain::database::skip_nothing;

   uint64_t         _head_block_num       = 0;
   block_id_type    _head_block_id        = block_id_type();
   uint64_t         _total_hashes         = 0;
   fc::time_point   _hash_start_time;

   std::map<public_key_type, fc::ecc::private_key> _private_keys;
   std::set<string>                                _witnesses;
   fc::future<void>                                _block_production_task;

   friend class detail::witness_plugin_impl;
   std::unique_ptr< detail::witness_plugin_impl > _my;
};

} } //steemit::witness_plugin
