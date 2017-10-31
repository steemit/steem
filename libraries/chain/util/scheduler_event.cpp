
#include <steem/chain/util/scheduler_event.hpp>

#include <steem/chain/database.hpp>

namespace steem { namespace chain { namespace util {

launch_action::launch_action()
{

}

launch_action::~launch_action()
{

}

int launch_action::run( database& db ) const
{
   //Main action - SMT launching.
   return 0;
}

refund_action::refund_action()
{

}

refund_action::~refund_action()
{

}

int refund_action::run( database& db ) const
{
   //Second main action - SMT refunding.
   return 0;
}

scheduler_event::scheduler_event( const asset_symbol_type& _symbol )
               : symbol( _symbol )
{

}

scheduler_event::~scheduler_event()
{

}

bool scheduler_event::removing_allowed() const
{
   return true;
}

#if defined IS_TEST_NET && defined STEEM_ENABLE_SMT

const smt_token_object* scheduler_event::get_smt_token_object( database& db ) const
{
   const smt_token_object* ret = db.find< smt_token_object, by_symbol >( symbol );
   FC_ASSERT( ret );

   return ret;
}

void scheduler_event::change_phase( database& db, const smt_token_object* token, smt_token_object::smt_phase phase ) const
{
   FC_ASSERT( token );

   db.modify( *token, [&]( smt_token_object& token )
   {
      token.phase = phase;
   });
}

#endif

contribution_begin_scheduler_event::contribution_begin_scheduler_event( const asset_symbol_type& _symbol )
                              : scheduler_event( _symbol )
{

}

contribution_begin_scheduler_event::~contribution_begin_scheduler_event()
{

}

int contribution_begin_scheduler_event::run( database& db ) const
{
#if defined IS_TEST_NET && defined STEEM_ENABLE_SMT

   const smt_token_object* obj = scheduler_event::get_smt_token_object( db );
   FC_ASSERT( obj );

   //From now contributions are allowed.
   change_phase( db, obj, smt_token_object::smt_phase::contribution_begin_time_completed );
   db.scheduler_add( obj->generation_end_time, util::contribution_end_scheduler_event( symbol ), true/*buffered*/ );

#endif

   return 0;
}

contribution_end_scheduler_event::contribution_end_scheduler_event( const asset_symbol_type& _symbol )
                              : scheduler_event( _symbol )
{

}

contribution_end_scheduler_event::~contribution_end_scheduler_event()
{

}

int contribution_end_scheduler_event::run( database& db ) const
{
#if defined IS_TEST_NET && defined STEEM_ENABLE_SMT

   const smt_token_object* obj = scheduler_event::get_smt_token_object( db );
   FC_ASSERT( obj );

   //From now contributions are forbidden.
   change_phase( db, obj, smt_token_object::smt_phase::contribution_end_time_completed );
   db.scheduler_add( obj->announced_launch_time, util::launch_scheduler_event( symbol ), true/*buffered*/ );

#endif

   return 0;
}

launch_scheduler_event::launch_scheduler_event( const asset_symbol_type& _symbol )
                              : scheduler_event( _symbol )
{

}

launch_scheduler_event::~launch_scheduler_event()
{

}

void launch_scheduler_event::deactivate()
{
   deactivated = true;
}

bool launch_scheduler_event::removing_allowed() const
{
   return removing_allowed_status;
}

bool launch_scheduler_event::is_min_steem_reached( database& db ) const
{
   //Check if we have enough STEEM-s.
   return false;
}

bool launch_scheduler_event::are_hidden_elements_revealed( database& db ) const
{
   //Check if hard cap and min steem are both revealed.
   return false;
}

int launch_scheduler_event::run( database& db ) const
{
   if( deactivated )
   {
      removing_allowed_status = true;
      return 0;
   }

   int ret = 0;

#if defined IS_TEST_NET && defined STEEM_ENABLE_SMT

   const smt_token_object* obj = scheduler_event::get_smt_token_object( db );
   FC_ASSERT( obj );

   bool _are_hidden_elements_revealed = are_hidden_elements_revealed( db );
   if( _are_hidden_elements_revealed )
   {
      bool _is_min_steem_reached = is_min_steem_reached( db );
      if( _is_min_steem_reached )
      {
         removing_allowed_status = true;
         change_phase( db, obj, smt_token_object::smt_phase::launch_time_completed );

         ret = launcher.run( db );
      }
      else
         ret = refunder.run( db );
   }

   if( first_check )
   {
      first_check = false;
      db.scheduler_add( obj->launch_expiration_time, util::launch_expiration_scheduler_event( symbol, const_cast< launch_scheduler_event& >( *this ) ), true/*buffered*/ );
   }

#endif

   return ret;
}

launch_expiration_scheduler_event::launch_expiration_scheduler_event( const asset_symbol_type& _symbol, launch_scheduler_event& _launch_event )
                              : scheduler_event( _symbol ), launch_event( _launch_event )
{

}

launch_expiration_scheduler_event::~launch_expiration_scheduler_event()
{

}

int launch_expiration_scheduler_event::run( database& db ) const
{
   int ret = 0;

#if defined IS_TEST_NET && defined STEEM_ENABLE_SMT

   const smt_token_object* obj = scheduler_event::get_smt_token_object( db );
   FC_ASSERT( obj );

   change_phase( db, obj, smt_token_object::smt_phase::launch_expiration_time_completed );

   if( obj->phase != smt_token_object::smt_phase::launch_time_completed )
      ret = refunder.run( db );

   launch_event.deactivate();

#endif

   return ret;
}

scheduler_event_visitor::scheduler_event_visitor( database& _db )
                     : db( _db ), removing_allowed_status( true )
{
}

void scheduler_event_visitor::operator()( const contribution_begin_scheduler_event& contribution_begin ) const
{
   int status = contribution_begin.run( db );
   FC_ASSERT( status == 0 );
   removing_allowed_status = contribution_begin.removing_allowed();
}

void scheduler_event_visitor::operator()( const contribution_end_scheduler_event& contribution_end ) const
{
   int status = contribution_end.run( db );
   FC_ASSERT( status == 0 );
   removing_allowed_status = contribution_end.removing_allowed();
}

void scheduler_event_visitor::operator()( const launch_scheduler_event& launch ) const
{
   int status = launch.run( db );
   FC_ASSERT( status == 0 );
   removing_allowed_status = launch.removing_allowed();
}

void scheduler_event_visitor::operator()( const launch_expiration_scheduler_event& expiration_launch ) const
{
   int status = expiration_launch.run( db );
   FC_ASSERT( status == 0 );
   removing_allowed_status = expiration_launch.removing_allowed();
}

template < typename T >
void scheduler_event_visitor::operator()( const T& obj ) const
{
   FC_ASSERT("Not supported yet.");
}

} } } // steem::chain::util
