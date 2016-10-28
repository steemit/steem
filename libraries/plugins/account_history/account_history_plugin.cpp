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
      bool                    _filter_content = false;
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



struct operation_visitor_filter : operation_visitor {
   operation_visitor_filter( database& db, const operation_object& op, const operation_object*& n, string i ):operation_visitor(db,op,n,i){}

   void operator()( const comment_operation& )const {}
   void operator()( const vote_operation& )const {}
   void operator()( const delete_comment_operation& )const{}
   void operator()( const custom_json_operation& )const {}
   void operator()( const custom_operation& )const {}
   void operator()( const curation_reward_operation& )const {}
   void operator()( const fill_order_operation& )const {}
   void operator()( const limit_order_create_operation& )const {}
   void operator()( const limit_order_cancel_operation& )const {}
   void operator()( const pow_operation& )const {}

   void operator()( const transfer_operation& op )const
   {
      operation_visitor::operator()( op );
   }

   void operator()( const transfer_to_vesting_operation& op )const
   {
      operation_visitor::operator()( op );
   }

   void operator()( const account_create_operation& op )const
   {
      operation_visitor::operator()( op );
   }

   void operator()( const account_update_operation& op )const
   {
      operation_visitor::operator()( op );
   }

   void operator()( const transfer_to_savings_operation& op )const
   {
      operation_visitor::operator()( op );
   }

   void operator()( const transfer_from_savings_operation& op )const
   {
      operation_visitor::operator()( op );
   }

   void operator()( const cancel_transfer_from_savings_operation& op )const
   {
      operation_visitor::operator()( op );
   }

   void operator()( const escrow_transfer_operation& op )const
   {
      operation_visitor::operator()( op );
   }

   void operator()( const escrow_dispute_operation& op )const
   {
      operation_visitor::operator()( op );
   }

   void operator()( const escrow_release_operation& op )const
   {
      operation_visitor::operator()( op );
   }

   void operator()( const escrow_approve_operation& op )const
   {
      operation_visitor::operator()( op );
   }

   template<typename Op>
   void operator()( Op&& op )const{ }
};

void account_history_plugin_impl::on_operation( const operation_object& op_obj ) {
   flat_set<string> impacted;
   steemit::chain::database& db = database();
   auto size = fc::raw::pack_size(op_obj);
   if( size > 1024*1024*128 ) {
      ilog("excluding transaction from history due to size ${s}", ("s",size) );
      return;
   }

   const auto& hist_idx = db.get_index_type<account_history_index>().indices().get<by_account>();
   const operation_object* new_obj = nullptr;
   app::operation_get_impacted_accounts( op_obj.op, impacted );

   for( const auto& item : impacted ) {
      auto itr = _tracked_accounts.lower_bound( item );
      if( !_tracked_accounts.size() || (itr != _tracked_accounts.end() && itr->first <= item && item <= itr->second ) ) {
         if( _filter_content )
            op_obj.op.visit( operation_visitor_filter(db, op_obj, new_obj, item) );
         else
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
         ("track-account-range", boost::program_options::value<std::vector<std::string>>()->composing()->multitoken(), "Defines a range of accounts to track as a json pair [\"from\",\"to\"] [from,to]")
         ("filter-posting-ops", "Ignore posting operations, only track transfers and account updates")
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
   LOAD_VALUE_SET(options, "track-account-range", my->_tracked_accounts, pairstring);
   if( options.count( "filter-posting-ops" ) ) {
      my->_filter_content = true;
   }
}

void account_history_plugin::plugin_startup()
{
   ilog( "account_history plugin: plugin_startup() begin" );

   ilog( "account_history plugin: plugin_startup() end" );
}

flat_map<string,string> account_history_plugin::tracked_accounts() const
{
   return my->_tracked_accounts;
}

} }

STEEMIT_DEFINE_PLUGIN( account_history, steemit::account_history::account_history_plugin )
