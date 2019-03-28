
#include <steem/chain/steem_fwd.hpp>

#include <steem/plugins/reputation/reputation_plugin.hpp>
#include <steem/plugins/reputation/reputation_objects.hpp>

#include <steem/chain/util/impacted.hpp>

#include <steem/protocol/config.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/index.hpp>
#include <steem/chain/account_object.hpp>
#include <steem/chain/comment_object.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <memory>

namespace steem { namespace plugins { namespace reputation {

using namespace steem::protocol;

namespace detail {

class reputation_plugin_impl
{
   public:
      reputation_plugin_impl( reputation_plugin& _plugin ) :
         _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ),
         _self( _plugin ) {}
      ~reputation_plugin_impl() {}

      void pre_operation( const operation_notification& op_obj );
      void post_operation( const operation_notification& op_obj );

      chain::database&              _db;
      reputation_plugin&            _self;
      boost::signals2::connection   _pre_apply_operation_conn;
      boost::signals2::connection   _post_apply_operation_conn;
};

struct pre_operation_visitor
{
   reputation_plugin_impl& _plugin;

   pre_operation_visitor( reputation_plugin_impl& plugin )
      : _plugin( plugin ) {}

   typedef void result_type;

   template< typename T >
   void operator()( const T& )const {}

   void operator()( const vote_operation& op )const
   {
      try
      {
         auto& db = _plugin._db;
         const auto& c = db.get_comment( op.author, op.permlink );

         if( db.calculate_discussion_payout_time( c ) == fc::time_point_sec::maximum() ) return;

         const auto& cv_idx = db.get_index< comment_vote_index >().indices().get< by_comment_voter >();
         auto cv = cv_idx.find( boost::make_tuple( c.id, db.get_account( op.voter ).id ) );

         if( cv != cv_idx.end() )
         {
            auto rep_delta = ( cv->rshares >> 6 );

            const auto& rep_idx = db.get_index< reputation_index >().indices().get< by_account >();
            auto voter_rep = rep_idx.find( op.voter );
            auto author_rep = rep_idx.find( op.author );

            if( author_rep != rep_idx.end() )
            {
               // Rule #1: Must have non-negative reputation to effect another user's reputation
               if( voter_rep != rep_idx.end() && voter_rep->reputation < 0 ) return;

               // Rule #2: If you are down voting another user, you must have more reputation than them to impact their reputation
               if( cv->rshares < 0 && !( voter_rep != rep_idx.end() && voter_rep->reputation > author_rep->reputation - rep_delta ) ) return;

               if( rep_delta == author_rep->reputation )
               {
                  db.remove( *author_rep );
               }
               else
               {
                  db.modify( *author_rep, [&]( reputation_object& r )
                  {
                     r.reputation -= ( cv->rshares >> 6 ); // Shift away precision from vests. It is noise
                  });
               }
            }
         }
      }
      catch( const fc::exception& e ) {}
   }
};

struct post_operation_visitor
{
   reputation_plugin_impl& _plugin;

   post_operation_visitor( reputation_plugin_impl& plugin )
      : _plugin( plugin ) {}

   typedef void result_type;

   template< typename T >
   void operator()( const T& )const {}

   void operator()( const vote_operation& op )const
   {
      try
      {
         auto& db = _plugin._db;
         const auto& comment = db.get_comment( op.author, op.permlink );

         if( db.calculate_discussion_payout_time( comment ) == fc::time_point_sec::maximum() )
            return;

         const auto& cv_idx = db.get_index< comment_vote_index >().indices().get< by_comment_voter >();
         auto cv = cv_idx.find( boost::make_tuple( comment.id, db.get_account( op.voter ).id ) );

         const auto& rep_idx = db.get_index< reputation_index >().indices().get< by_account >();
         auto voter_rep = rep_idx.find( op.voter );
         auto author_rep = rep_idx.find( op.author );

         // Rules are a plugin, do not effect consensus, and are subject to change.
         // Rule #1: Must have non-negative reputation to effect another user's reputation
         if( voter_rep != rep_idx.end() && voter_rep->reputation < 0 ) return;

         if( author_rep == rep_idx.end() )
         {
            // Rule #2: If you are down voting another user, you must have more reputation than them to impact their reputation
            // User rep is 0, so requires voter having positive rep
            if( cv->rshares < 0 && !( voter_rep != rep_idx.end() && voter_rep->reputation > 0 )) return;

            db.create< reputation_object >( [&]( reputation_object& r )
            {
               r.account = op.author;
               r.reputation = ( cv->rshares >> 6 ); // Shift away precision from vests. It is noise
            });
         }
         else
         {
            // Rule #2: If you are down voting another user, you must have more reputation than them to impact their reputation
            if( cv->rshares < 0 && !( voter_rep != rep_idx.end() && voter_rep->reputation > author_rep->reputation ) ) return;

            db.modify( *author_rep, [&]( reputation_object& r )
            {
               r.reputation += ( cv->rshares >> 6 ); // Shift away precision from vests. It is noise
            });
         }
      }
      FC_CAPTURE_AND_RETHROW()
   }
};

void reputation_plugin_impl::pre_operation( const operation_notification& note )
{
   try
   {
      note.op.visit( pre_operation_visitor( *this ) );
   }
   catch( const fc::assert_exception& )
   {
      if( _db.is_producing() ) throw;
   }
}

void reputation_plugin_impl::post_operation( const operation_notification& note )
{
   try
   {
      note.op.visit( post_operation_visitor( *this ) );
   }
   catch( fc::assert_exception )
   {
      if( _db.is_producing() ) throw;
   }
}

} // detail

reputation_plugin::reputation_plugin() {}

reputation_plugin::~reputation_plugin() {}

void reputation_plugin::set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
   )
{}

void reputation_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   try
   {
      ilog("Intializing reputation plugin" );

      my = std::make_unique< detail::reputation_plugin_impl >( *this );

      my->_pre_apply_operation_conn = my->_db.add_pre_apply_operation_handler( [&]( const operation_notification& note ){ my->pre_operation( note ); }, *this, 0 );
      my->_post_apply_operation_conn = my->_db.add_post_apply_operation_handler( [&]( const operation_notification& note ){ my->post_operation( note ); }, *this, 0 );
      add_plugin_index< reputation_index        >( my->_db );

      appbase::app().get_plugin< chain::chain_plugin >().report_state_options( name(), fc::variant_object() );
   }
   FC_CAPTURE_AND_RETHROW()
}

void reputation_plugin::plugin_startup() {}

void reputation_plugin::plugin_shutdown()
{
   chain::util::disconnect_signal( my->_pre_apply_operation_conn );
   chain::util::disconnect_signal( my->_post_apply_operation_conn );
}

} } } // steem::plugins::reputation
