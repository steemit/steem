#include <steemit/account_history/account_history_plugin.hpp>

#include <steemit/app/impacted.hpp>

#include <steemit/protocol/config.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/chain/operation_notification.hpp>
#include <steemit/chain/history_object.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <boost/algorithm/string.hpp>

#define STEEM_NAMESPACE_PREFIX "steemit::protocol::"

namespace steemit { namespace account_history {

namespace detail
{

using namespace steemit::protocol;

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

      void on_operation( const operation_notification& note );

      account_history_plugin& _self;
      flat_map< account_name_type, account_name_type > _tracked_accounts;
      bool                                             _filter_content = false;
      bool                                             _blacklist = false;
      flat_set< string >                               _op_list;
};

account_history_plugin_impl::~account_history_plugin_impl()
{
   return;
}

struct operation_visitor
{
   operation_visitor( database& db, const operation_notification& note, const operation_object*& n, account_name_type i )
      :_db(db), _note(note), new_obj(n), item(i) {}

   typedef void result_type;

   database& _db;
   const operation_notification& _note;
   const operation_object*& new_obj;
   account_name_type item;

   template<typename Op>
   void operator()( Op&& )const
   {
         const auto& hist_idx = _db.get_index<account_history_index>().indices().get<by_account>();
         if( !new_obj )
         {
            new_obj = &_db.create<operation_object>( [&]( operation_object& obj )
            {
               obj.trx_id       = _note.trx_id;
               obj.block        = _note.block;
               obj.trx_in_block = _note.trx_in_block;
               obj.op_in_trx    = _note.op_in_trx;
               obj.virtual_op   = _note.virtual_op;
               obj.timestamp    = _db.head_block_time();
               //fc::raw::pack( obj.serialized_op , _note.op);  //call to 'pack' is ambiguous
               auto size = fc::raw::pack_size( _note.op );
               obj.serialized_op.resize( size );
               fc::datastream< char* > ds( obj.serialized_op.data(), size );
               fc::raw::pack( ds, _note.op );
            });
         }

         auto hist_itr = hist_idx.lower_bound( boost::make_tuple( item, uint32_t(-1) ) );
         uint32_t sequence = 0;
         if( hist_itr != hist_idx.end() && hist_itr->account == item )
            sequence = hist_itr->sequence + 1;

         _db.create<account_history_object>( [&]( account_history_object& ahist )
         {
            ahist.account  = item;
            ahist.sequence = sequence;
            ahist.op       = new_obj->id;
         });
   }
};

struct operation_visitor_filter : operation_visitor
{
   operation_visitor_filter( database& db, const operation_notification& note, const operation_object*& n, account_name_type i, const flat_set< string >& filter, bool blacklist ):
      operation_visitor( db, note, n, i ), _filter( filter ), _blacklist( blacklist ) {}

   const flat_set< string >& _filter;
   bool _blacklist;

   template< typename T >
   void operator()( const T& op )const
   {
      if( _filter.find( fc::get_typename< T >::name() ) != _filter.end() )
      {
         if( !_blacklist )
            operation_visitor::operator()( op );
      }
      else
      {
         if( _blacklist )
            operation_visitor::operator()( op );
      }
   }
};

void account_history_plugin_impl::on_operation( const operation_notification& note )
{
   flat_set<account_name_type> impacted;
   steemit::chain::database& db = database();

   const operation_object* new_obj = nullptr;
   app::operation_get_impacted_accounts( note.op, impacted );

   for( const auto& item : impacted ) {
      auto itr = _tracked_accounts.lower_bound( item );

      /*
       * The map containing the ranges uses the key as the lower bound and the value as the upper bound.
       * Because of this, if a value exists with the range (key, value], then calling lower_bound on
       * the map will return the key of the next pair. Under normal circumstances of those ranges not
       * intersecting, the value we are looking for will not be present in range that is returned via
       * lower_bound.
       *
       * Consider the following example using ranges ["a","c"], ["g","i"]
       * If we are looking for "bob", it should be tracked because it is in the lower bound.
       * However, lower_bound( "bob" ) returns an iterator to ["g","i"]. So we need to decrement the iterator
       * to get the correct range.
       *
       * If we are looking for "g", lower_bound( "g" ) will return ["g","i"], so we need to make sure we don't
       * decrement.
       *
       * If the iterator points to the end, we should check the previous (equivalent to rbegin)
       *
       * And finally if the iterator is at the beginning, we should not decrement it for obvious reasons
       */
      if( itr != _tracked_accounts.begin() &&
          ( ( itr != _tracked_accounts.end() && itr->first != item  ) || itr == _tracked_accounts.end() ) )
      {
         --itr;
      }

      if( !_tracked_accounts.size() || (itr != _tracked_accounts.end() && itr->first <= item && item <= itr->second ) )
      {
         if(_filter_content)
         {
            note.op.visit( operation_visitor_filter( db, note, new_obj, item, _op_list, _blacklist ) );
         }
         else
         {
            note.op.visit( operation_visitor( db, note, new_obj, item ) );
         }
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
         ("track-account-range", boost::program_options::value< vector< string > >()->composing()->multitoken(), "Defines a range of accounts to track as a json pair [\"from\",\"to\"] [from,to] Can be specified multiple times")
         ("history-whitelist-ops", boost::program_options::value< vector< string > >()->composing(), "Defines a list of operations which will be explicitly logged.")
         ("history-blacklist-ops", boost::program_options::value< vector< string > >()->composing(), "Defines a list of operations which will be explicitly ignored.")
         ;
   cfg.add(cli);
}

void account_history_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   //ilog("Intializing account history plugin" );
   database().pre_apply_operation.connect( [&]( const operation_notification& note ){ my->on_operation(note); } );

   typedef pair<account_name_type,account_name_type> pairstring;
   LOAD_VALUE_SET(options, "track-account-range", my->_tracked_accounts, pairstring);

   if( options.count( "history-whitelist-ops" ) )
   {
      my->_filter_content = true;
      my->_blacklist = false;

      for( auto& arg : options.at( "history-whitelist-ops" ).as< vector< string > >() )
      {
         vector< string > ops;
         boost::split( ops, arg, boost::is_any_of( " \t," ) );

         for( const string& op : ops )
         {
            if( op.size() )
               my->_op_list.insert( STEEM_NAMESPACE_PREFIX + op );
         }
      }

      ilog( "Account History: whitelisting ops ${o}", ("o", my->_op_list) );
   }
   else if( options.count( "history-blacklist-ops" ) )
   {
      my->_filter_content = true;
      my->_blacklist = true;
      for( auto& arg : options.at( "history-blacklist-ops" ).as< vector< string > >() )
      {
         vector< string > ops;
         boost::split( ops, arg, boost::is_any_of( " \t," ) );

         for( const string& op : ops )
         {
            if( op.size() )
               my->_op_list.insert( STEEM_NAMESPACE_PREFIX + op );
         }
      }

      ilog( "Account History: blacklisting ops ${o}", ("o", my->_op_list) );
   }
}

void account_history_plugin::plugin_startup()
{
   ilog( "account_history plugin: plugin_startup() begin" );

   ilog( "account_history plugin: plugin_startup() end" );
}

flat_map< account_name_type, account_name_type > account_history_plugin::tracked_accounts() const
{
   return my->_tracked_accounts;
}

} }

STEEMIT_DEFINE_PLUGIN( account_history, steemit::account_history::account_history_plugin )
