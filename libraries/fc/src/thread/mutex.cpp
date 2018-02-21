#include <fc/thread/mutex.hpp>
#include <fc/thread/thread.hpp>
#include <fc/thread/unique_lock.hpp>
#include <fc/log/logger.hpp>
#include "context.hpp"
#include "thread_d.hpp"

namespace fc {

  mutex::mutex() :
    m_blist(0),
    recursive_lock_count(0)
  {}

  mutex::~mutex() {
    if( m_blist ) 
    {
      fc::thread::current().debug("~mutex");
      BOOST_ASSERT( false && "Attempt to free mutex while others are blocking on lock." );
    }
  }

  /**
   *  @param  last_context - is set to the next context to get the lock (the next-to-last element of the list)
   *  @return the last context (the one with the lock)
   */
  static fc::context* get_tail( fc::context* list_head, fc::context*& context_to_unblock ) {
    context_to_unblock = 0;
    fc::context* list_context_iter = list_head;
    if( !list_context_iter ) 
      return list_context_iter;
    while( list_context_iter->next_blocked_mutex ) 
    { 
      context_to_unblock = list_context_iter;
      list_context_iter = list_context_iter->next_blocked_mutex;
    }
    return list_context_iter;
  }

  static fc::context* remove( fc::context* head, fc::context* target ) {
    fc::context* context_iter = head;
    fc::context* previous = 0;
    while( context_iter ) 
    {
      if( context_iter == target ) 
      {
        if( previous ) 
        { 
          previous->next_blocked_mutex = context_iter->next_blocked_mutex; 
          return head; 
        }
        return context_iter->next_blocked_mutex;
      }
      previous = context_iter;
      context_iter = context_iter->next_blocked_mutex;
    }
    return head;
  }
  static void cleanup( fc::mutex& m, fc::spin_yield_lock& syl, fc::context*& bl, fc::context* cc ) {
    {  
      fc::unique_lock<fc::spin_yield_lock> lock(syl);
      if( cc->next_blocked_mutex ) {
        bl = remove(bl, cc); 
        cc->next_blocked_mutex = nullptr;
        return;
      }
    }
    m.unlock();
  }

  /**
   *  A mutex is considered to hold the lock when
   *  the current context is the tail in the wait queue.
   */
  bool mutex::try_lock() {
    assert(false); // this is currently broken re: recursive mutexes
    fc::thread* ct = &fc::thread::current();
    fc::context* cc = ct->my->current;
    fc::context* n  = 0;

    fc::unique_lock<fc::spin_yield_lock> lock(m_blist_lock, fc::try_to_lock_t());
    if( !lock  ) 
      return false;

    if( !m_blist ) { 
      m_blist = cc;
      return true;
    }
    // allow recursive locks.
    return ( get_tail( m_blist, n ) == cc );
  }

  bool mutex::try_lock_until( const fc::time_point& abs_time ) {
    assert(false); // this is currently broken re: recursive mutexes
    fc::context* n  = 0;
    fc::context* cc = fc::thread::current().my->current;

    { // lock scope
      fc::unique_lock<fc::spin_yield_lock> lock(m_blist_lock,abs_time);
      if( !lock ) return false;

      if( !m_blist ) { 
        m_blist = cc;
        return true;
      }

      // allow recusive locks
      if ( get_tail( m_blist, n ) == cc ) 
        return true;

      cc->next_blocked_mutex = m_blist;
      m_blist = cc;
    } // end lock scope

    
    std::exception_ptr e;
    try {
        fc::thread::current().my->yield_until( abs_time, false );
        return( 0 == cc->next_blocked_mutex );
    } catch (...) {
      e = std::current_exception();
    }
    assert(e);
    cleanup( *this, m_blist_lock, m_blist, cc);
    std::rethrow_exception(e);
  }

  void mutex::lock() {
    fc::context* current_context = fc::thread::current().my->current;
    if( !current_context ) 
      current_context = fc::thread::current().my->current = new fc::context( &fc::thread::current() );

    {
      fc::unique_lock<fc::spin_yield_lock> lock(m_blist_lock);
      if( !m_blist ) 
      { 
        // nobody else owns the mutex, so we get it; add our context as the last and only element on the mutex's list
        m_blist = current_context;
        assert(recursive_lock_count == 0);
        recursive_lock_count = 1;
        assert(!current_context->next_blocked_mutex);
        return;
      }

      // allow recusive locks
      fc::context* dummy_context_to_unblock  = 0;
      if ( get_tail( m_blist, dummy_context_to_unblock ) == current_context ) {
        assert(recursive_lock_count > 0);
        ++recursive_lock_count;
        return;
      }
      // add ourselves to the head of the list
      current_context->next_blocked_mutex = m_blist;
      m_blist = current_context;

#if 0
      int cnt = 0;
      auto i = m_blist;
      while( i ) {
        i = i->next_blocked_mutex;
        ++cnt;
      }
      //wlog( "wait queue len %1%", cnt );
#endif
    }

    std::exception_ptr e; // cleanup calls yield so we need to move the exception outside of the catch block
    try 
    {
      fc::thread::current().yield(false);
      // if yield() returned normally, we should now own the lock (we should be at the tail of the list)
      BOOST_ASSERT( current_context->next_blocked_mutex == 0 );
      assert(recursive_lock_count == 0);
      recursive_lock_count = 1;
    }
    catch ( ... ) 
    {
      e = std::current_exception();
    }
    if( e ) {
      cleanup( *this, m_blist_lock, m_blist, current_context);
      std::rethrow_exception(e);
    }
  }

  void mutex::unlock() 
  {
    fc::context* context_to_unblock = 0;

    fc::unique_lock<fc::spin_yield_lock> lock(m_blist_lock);
    assert(recursive_lock_count > 0);
    --recursive_lock_count;
    if (recursive_lock_count != 0)
      return;

    get_tail(m_blist, context_to_unblock);
    if( context_to_unblock ) 
    {
      context_to_unblock->next_blocked_mutex = 0;
      context_to_unblock->ctx_thread->my->unblock( context_to_unblock );
    } 
    else
      m_blist = 0;
  }

} // fc


