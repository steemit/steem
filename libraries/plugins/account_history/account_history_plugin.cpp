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

#include <steemit/account_history/account_history_plugin.hpp>

#include <steemit/app/impacted.hpp>

#include <steemit/chain/config.hpp>
#include <steemit/chain/database.hpp>
#include <steemit/chain/history_object.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

namespace steemit { namespace account_history {

namespace detail
{

class account_history_plugin_impl
{
   public:
      account_history_plugin_impl(account_history_plugin& _plugin)
         : _self( _plugin )
      { }
      virtual ~account_history_plugin_impl();

      steemit::chain::database& database()
      {
         return _self.database();
      }

      void on_operation( const operation_object& op_obj );

      account_history_plugin& _self;
      flat_map<string,string> _tracked_accounts;
};

account_history_plugin_impl::~account_history_plugin_impl()
{
   return;
}

struct operation_visitor {
   operation_visitor( database& db, const operation_object& op, const operation_object*& n, string i ):_db(db),op_obj(op),new_obj(n),item(i){};
   typedef void result_type;

   database& _db;
   const operation_object& op_obj;
   const operation_object*& new_obj;
   string item;

   /// ignore these ops
   /*
   void operator()( const comment_operation& ) {}
   void operator()( const vote_operation& ) {}
   void operator()( const delete_comment_operation& ){} 
   void operator()( const custom_json_operation& ) {}
   void operator()( const custom_operation& ) {}
   void operator()( const curate_reward_operation& ) {}
   */


   template<typename Op>
   void operator()( Op&& )const{

         const auto& hist_idx = _db.get_index_type<account_history_index>().indices().get<by_account>();
         if( !new_obj ) {
            new_obj = &_db.create<operation_object>( [&]( operation_object& obj ){
               obj.trx_id       = op_obj.trx_id;
               obj.block        = op_obj.block;
               obj.trx_in_block = op_obj.trx_in_block;
               obj.op_in_trx    = op_obj.op_in_trx;
               obj.virtual_op   = op_obj.virtual_op;
               obj.timestamp    = _db.head_block_time();
               obj.op           = op_obj.op;
            });
         }

         auto hist_itr = hist_idx.lower_bound( boost::make_tuple( item, uint32_t(-1) ) );
         uint32_t sequence = 0;
         if( hist_itr != hist_idx.end() && hist_itr->account == item )
            sequence = hist_itr->sequence + 1;

         /*const auto& ahist = */_db.create<account_history_object>( [&]( account_history_object& ahist ){
              ahist.account  = item;
              ahist.sequence = sequence;
              ahist.op       = new_obj->id;
         });
   }
};

void account_history_plugin_impl::on_operation( const operation_object& op_obj ) {
   flat_set<string> impacted;
   steemit::chain::database& db = database();

   const auto& hist_idx = db.get_index_type<account_history_index>().indices().get<by_account>();
   const operation_object* new_obj = nullptr;
   app::operation_get_impacted_accounts( op_obj.op, impacted );

   for( const auto& item : impacted ) {
      auto itr = _tracked_accounts.lower_bound( item );
      if( !_tracked_accounts.size() || (itr != _tracked_accounts.end() && itr->first <= item && itr->second < item) ) {
         op_obj.op.visit( operation_visitor(db, op_obj, new_obj, item) );
      }
   }
}

} // end namespace detail

account_history_plugin::account_history_plugin( application* app )
   : plugin( app ), my( new detail::account_history_plugin_impl(*this) )
{
   //ilog("Loading account history plugin" );
}

account_history_plugin::~account_history_plugin()
{
}

std::string account_history_plugin::plugin_name()const
{
   return "account_history";
}

void account_history_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
   )
{
   cli.add_options()
         ("track-account-range", boost::program_options::value<std::vector<std::string>>()->composing()->multitoken(), "Defines a range of accounts to track as a json pair [\"from\",\"to\"] [from,to)")
         ;
   cfg.add(cli);
}

void account_history_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   //ilog("Intializing account history plugin" );
   database().pre_apply_operation.connect( [&]( const operation_object& b){ my->on_operation(b); } );
   database().add_index< primary_index< operation_index  > >();
   database().add_index< primary_index< account_history_index  > >();

   typedef pair<string,string> pairstring;
   LOAD_VALUE_SET(options, "tracked-accounts", my->_tracked_accounts, pairstring);
}

void account_history_plugin::plugin_startup()
{
}

flat_map<string,string> account_history_plugin::tracked_accounts() const
{
   return my->_tracked_accounts;
}

} }

STEEMIT_DEFINE_PLUGIN( account_history, steemit::account_history::account_history_plugin )
