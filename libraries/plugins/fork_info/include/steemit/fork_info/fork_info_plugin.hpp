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
#include <steemit/chain/steem_objects.hpp>


namespace steemit { namespace plugin { namespace fork_info {
namespace detail { struct fork_info_plugin_impl; }
using steemit::app::application;
class fork_info_plugin : public steemit::app::plugin
{
   public:
      fork_info_plugin( application* app );
      virtual ~fork_info_plugin();

      std::string plugin_name()const override { return "fork_info"; }

      virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
      virtual void plugin_startup() override;

   private:
      std::unique_ptr<steemit::plugin::fork_info::detail::fork_info_plugin_impl> _my;
};

} } } //steemit::plugin::fork_info_info

STEEMIT_DEFINE_PLUGIN( fork_info, steemit::plugin::fork_info::fork_info_plugin )
