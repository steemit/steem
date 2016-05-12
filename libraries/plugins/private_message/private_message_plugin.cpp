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

#include <steemit/private_message/private_message_plugin.hpp>

#include <steemit/app/impacted.hpp>

#include <steemit/chain/config.hpp>
#include <steemit/chain/database.hpp>
#include <steemit/chain/history_object.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

namespace steemit { namespace private_message {

namespace detail
{

class private_message_plugin_impl
{
   public:
      private_message_plugin_impl(private_message_plugin& _plugin)
         : _self( _plugin )
      { }
      virtual ~private_message_plugin_impl();

      steemit::chain::database& database()
      {
         return _self.database();
      }

      void on_operation( const operation_object& op_obj );

      private_message_plugin& _self;
      flat_map<string,string> _tracked_accounts;
};

private_message_plugin_impl::~private_message_plugin_impl()
{
   return;
}

void private_message_plugin_impl::on_operation( const operation_object& op_obj ) {
   steemit::chain::database& db = database();

   try {
      optional<private_message_operation> opm;

      if( op_obj.op.which() == operation::tag<custom_operation>::value ) {
         const custom_operation& cop = op_obj.op.get<custom_operation>();
         if( cop.id == STEEMIT_PRIVATE_MESSAGE_COP_ID )  {
            opm = fc::raw::unpack<private_message_operation>( cop.data );
            FC_ASSERT( cop.required_auths.find( opm->from ) != cop.required_auths.end(), "sender didn't sign message" );
         }
      } else if( op_obj.op.which() == operation::tag<custom_json_operation>::value ) {
         const custom_json_operation& cop = op_obj.op.get<custom_json_operation>();
         if( cop.id == "private_message" )  {
            opm = fc::json::from_string(cop.json).as<private_message_operation>();
            FC_ASSERT( cop.required_auths.find( opm->from ) != cop.required_auths.end() ||
                       cop.required_posting_auths.find( opm->from ) != cop.required_posting_auths.end()
                       , "sender didn't sign message" );
         }
      }

      if( opm ) {
         const auto& pm = *opm;

         auto to_itr   = _tracked_accounts.lower_bound(pm.to);
         auto from_itr = _tracked_accounts.lower_bound(pm.from);

         FC_ASSERT( pm.from != pm.to );
         FC_ASSERT( pm.from_memo_key != pm.to_memo_key );
         FC_ASSERT( pm.sent_time != 0 );
         FC_ASSERT( pm.encrypted_message.size() >= 32 );

         if( !_tracked_accounts.size() ||
             (to_itr != _tracked_accounts.end() && pm.to >= to_itr->first && pm.to <= to_itr->second) ||
             (from_itr != _tracked_accounts.end() && pm.from >= from_itr->first && pm.from <= from_itr->second) )
         {
            db.create<message_object>( [&]( message_object& pmo ) {
               pmo.from               = pm.from;
               pmo.to                 = pm.to;
               pmo.from_memo_key      = pm.from_memo_key;
               pmo.to_memo_key        = pm.to_memo_key;
               pmo.checksum           = pm.checksum;
               pmo.sent_time          = pm.sent_time;
               pmo.receive_time       = db.head_block_time();
               pmo.encrypted_message  = pm.encrypted_message;
            });
         }
      }

   } catch ( const fc::exception& ) {
      if( db.is_producing() ) throw;
   }
}

} // end namespace detail

private_message_plugin::private_message_plugin() :
   my( new detail::private_message_plugin_impl(*this) )
{
   //ilog("Loading account history plugin" );
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
   database().on_applied_operation.connect( [&]( const operation_object& b){ my->on_operation(b); } );
   database().add_index< primary_index< private_message_index  > >();

   app().register_api_factory<private_message_api>("private_message_api");

   typedef pair<string,string> pairstring;
   LOAD_VALUE_SET(options, "pm-accounts", my->_tracked_accounts, pairstring);
}

vector<message_object> private_message_api::get_inbox( string to, time_point newest, uint16_t limit )const {
   FC_ASSERT( limit <= 100 );
   vector<message_object> result;
   const auto& idx = _app->chain_database()->get_index_type<private_message_index>().indices().get<by_to_date>();
   auto itr = idx.lower_bound( std::make_tuple( to, newest ) );
   while( itr != idx.end() && limit && itr->to == to ) {
      result.push_back(*itr);
      ++itr;
      --limit;
   }

   return result;
}

vector<message_object> private_message_api::get_outbox( string from, time_point newest, uint16_t limit )const {
   FC_ASSERT( limit <= 100 );
   vector<message_object> result;
   const auto& idx = _app->chain_database()->get_index_type<private_message_index>().indices().get<by_from_date>();

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
