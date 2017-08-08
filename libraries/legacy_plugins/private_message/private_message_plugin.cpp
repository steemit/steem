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

#include <steemit/private_message/private_message_evaluators.hpp>
#include <steemit/private_message/private_message_operations.hpp>
#include <steemit/private_message/private_message_plugin.hpp>

#include <steemit/app/impacted.hpp>

#include <steemit/protocol/config.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/chain/index.hpp>
#include <steemit/chain/generic_custom_operation_interpreter.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

namespace steemit { namespace private_message {

namespace detail
{

class private_message_plugin_impl
{
   public:
      private_message_plugin_impl(private_message_plugin& _plugin);
      virtual ~private_message_plugin_impl();

      steemit::chain::database& database()
      {
         return _self.database();
      }

      private_message_plugin&                                                             _self;
      std::shared_ptr< generic_custom_operation_interpreter< steemit::private_message::private_message_plugin_operation > >   _custom_operation_interpreter;
      flat_map<string,string>                                                             _tracked_accounts;
};

private_message_plugin_impl::private_message_plugin_impl( private_message_plugin& _plugin )
   : _self( _plugin )
{
   _custom_operation_interpreter = std::make_shared< generic_custom_operation_interpreter< steemit::private_message::private_message_plugin_operation > >( database() );

   _custom_operation_interpreter->register_evaluator< private_message_evaluator >( &_self );

   database().set_custom_operation_interpreter( _self.plugin_name(), _custom_operation_interpreter );
   return;
}

private_message_plugin_impl::~private_message_plugin_impl()
{
   return;
}

} // end namespace detail

void private_message_evaluator::do_apply( const private_message_operation& pm )
{
   database& d = db();

   const flat_map<string, string>& tracked_accounts = _plugin->my->_tracked_accounts;

   auto to_itr   = tracked_accounts.lower_bound(pm.to);
   auto from_itr = tracked_accounts.lower_bound(pm.from);

   FC_ASSERT( pm.from != pm.to );
   FC_ASSERT( pm.from_memo_key != pm.to_memo_key );
   FC_ASSERT( pm.sent_time != 0 );
   FC_ASSERT( pm.encrypted_message.size() >= 32 );

   if( !tracked_accounts.size() ||
       (to_itr != tracked_accounts.end() && pm.to >= to_itr->first && pm.to <= to_itr->second) ||
       (from_itr != tracked_accounts.end() && pm.from >= from_itr->first && pm.from <= from_itr->second) )
   {
      d.create<message_object>( [&]( message_object& pmo )
      {
         pmo.from               = pm.from;
         pmo.to                 = pm.to;
         pmo.from_memo_key      = pm.from_memo_key;
         pmo.to_memo_key        = pm.to_memo_key;
         pmo.checksum           = pm.checksum;
         pmo.sent_time          = pm.sent_time;
         pmo.receive_time       = d.head_block_time();
         pmo.encrypted_message.resize( pm.encrypted_message.size() );
         std::copy( pm.encrypted_message.begin(), pm.encrypted_message.end(), pmo.encrypted_message.begin() );
      } );
   }
}

private_message_plugin::private_message_plugin( application* app )
   : plugin( app ), my( new detail::private_message_plugin_impl(*this) )
{
}

private_message_plugin::~private_message_plugin()
{
}

std::string private_message_plugin::plugin_name()const
{
   return "private_message";
}

void private_message_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
   )
{
   cli.add_options()
         ("pm-account-range", boost::program_options::value<std::vector<std::string>>()->composing()->multitoken(), "Defines a range of accounts to private messages to/from as a json pair [\"from\",\"to\"] [from,to)")
         ;
   cfg.add(cli);
}

void private_message_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   ilog("Intializing private message plugin" );
   chain::database& db = database();
   add_plugin_index< message_index >(db);

   app().register_api_factory<private_message_api>("private_message_api");

   typedef pair<string,string> pairstring;
   LOAD_VALUE_SET(options, "pm-accounts", my->_tracked_accounts, pairstring);
}

vector< message_api_obj > private_message_api::get_inbox( string to, time_point newest, uint16_t limit )const {
   FC_ASSERT( limit <= 100 );
   vector< message_api_obj > result;
   const auto& idx = _app->chain_database()->get_index< message_index >().indices().get< by_to_date >();
   auto itr = idx.lower_bound( std::make_tuple( to, newest ) );
   while( itr != idx.end() && limit && itr->to == to ) {
      result.push_back(*itr);
      ++itr;
      --limit;
   }

   return result;
}

vector< message_api_obj > private_message_api::get_outbox( string from, time_point newest, uint16_t limit )const {
   FC_ASSERT( limit <= 100 );
   vector< message_api_obj > result;
   const auto& idx = _app->chain_database()->get_index< message_index >().indices().get< by_from_date >();

   auto itr = idx.lower_bound( std::make_tuple( from, newest ) );
   while( itr != idx.end() && limit && itr->from == from ) {
      result.push_back(*itr);
      ++itr;
      --limit;
   }
   return result;
}

void private_message_plugin::plugin_startup()
{
}

flat_map<string,string> private_message_plugin::tracked_accounts() const
{
   return my->_tracked_accounts;
}

} }

STEEMIT_DEFINE_PLUGIN( private_message, steemit::private_message::private_message_plugin )

DEFINE_OPERATION_TYPE( steemit::private_message::private_message_plugin_operation )
