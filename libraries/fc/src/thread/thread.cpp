#include <fc/thread/thread.hpp>
#include <fc/vector.hpp>
#include <fc/io/sstream.hpp>
#include <fc/log/logger.hpp>
#include "thread_d.hpp"

#if defined(_MSC_VER) && !defined(NDEBUG)
# include <windows.h>
const DWORD MS_VC_EXCEPTION=0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
   DWORD dwType; // Must be 0x1000.
   LPCSTR szName; // Pointer to name (in user addr space).
   DWORD dwThreadID; // Thread ID (-1=caller thread).
   DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

static void set_thread_name(const char* threadName)
{
   THREADNAME_INFO info;
   info.dwType = 0x1000;
   info.szName = threadName;
   info.dwThreadID = -1;
   info.dwFlags = 0;

   __try
   {
      RaiseException(MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info);
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
   }
}
#elif defined(__linux__) && !defined(NDEBUG)
# include <pthread.h>
static void set_thread_name(const char* threadName)
{
	pthread_setname_np(pthread_self(), threadName);
}
#elif defined(__APPLE__) && !defined(NDEBUG)
# include <pthread.h>
static void set_thread_name(const char* threadName)
{
	pthread_setname_np(threadName);
}
#else
static void set_thread_name(const char* threadName)
{
	// do nothing in release mode
}
#endif

namespace fc {
  const char* thread_name() {
    return thread::current().name().c_str();
  }
  void* thread_ptr() {
    return &thread::current();
  }

   thread*& current_thread() {
      #ifdef _MSC_VER
         static __declspec(thread) thread* t = NULL;
      #else
         static __thread thread* t = NULL;
      #endif
      return t;
   }

   thread::thread( const std::string& name  ) {
      promise<void>::ptr p(new promise<void>("thread start"));
      boost::thread* t = new boost::thread( [this,p,name]() {
          try {
            set_thread_name(name.c_str()); // set thread's name for the debugger to display
            this->my = new thread_d(*this);
            current_thread() = this;
            p->set_value();
            exec();
          } catch ( fc::exception& e ) {
            wlog( "unhandled exception" );
            p->set_exception( e.dynamic_copy_exception() );
          } catch ( ... ) {
            wlog( "unhandled exception" );
            p->set_exception( std::make_shared<unhandled_exception>( FC_LOG_MESSAGE( warn, "unhandled exception: ${diagnostic}", ("diagnostic",boost::current_exception_diagnostic_information()) ) ) );
            //assert( !"unhandled exception" );
            //elog( "Caught unhandled exception %s", boost::current_exception_diagnostic_information().c_str() );
          }
      } );
      p->wait();
      my->boost_thread = t;
      my->name = name;
      //wlog("name:${n} tid:${tid}", ("n", name)("tid", (uintptr_t)my->boost_thread->native_handle()) );
   }
   thread::thread( thread_d* ) {
     my = new thread_d(*this);
   }

   thread::thread( thread&& m ) {
    my = m.my;
    m.my = 0;
   }

   thread& thread::operator=(thread&& t ) {
      fc_swap(t.my,my);
      return *this;
   }

   thread::~thread() {
      //wlog( "my ${n}", ("n",name()) );
      if( my )
      {
        // wlog( "calling quit() on ${n}",("n",my->name) );
        quit(); // deletes `my`
      }
   }

   thread& thread::current() {
     if( !current_thread() )
       current_thread() = new thread((thread_d*)0);
     return *current_thread();
   }

   const string& thread::name()const
   {
     return my->name;
   }

   void thread::set_name( const fc::string& n )
   {
     if (!is_current())
     {
       async([=](){ set_name(n); }, "set_name").wait();
       return;
     }
     my->name = n;
     set_thread_name(my->name.c_str()); // set thread's name for the debugger to display
   }

   const char* thread::current_task_desc() const
   {
      if (my->current && my->current->cur_task)
         return my->current->cur_task->get_desc();
      return NULL;
   }

   void          thread::debug( const fc::string& d ) { /*my->debug(d);*/ }

  void thread::quit()
  {
    //if quitting from a different thread, start quit task on thread.
    //If we have and know our attached boost thread, wait for it to finish, then return.
    if( &current() != this )
    {
      async( [=](){quit();}, "thread::quit" );//.wait();
      if( my->boost_thread )
      {
        //wlog("destroying boost thread ${tid}",("tid",(uintptr_t)my->boost_thread->native_handle()));
        my->boost_thread->join();
        delete my;
        my = nullptr;
      }
      return;
    }

    my->done = true;
    //      wlog( "${s}", ("s",name()) );
    // We are quiting from our own thread...

    // break all promises, thread quit!
    while( my->blocked )
    {
      fc::context* cur  = my->blocked;
      while( cur )
      {
        fc::context* n = cur->next;
        // this will move the context into the ready list.
        //cur->prom->set_exception( boost::copy_exception( error::thread_quit() ) );
        //cur->set_exception_on_blocking_promises( thread_quit() );
        cur->set_exception_on_blocking_promises( std::make_shared<canceled_exception>(FC_LOG_MESSAGE(error, "cancellation reason: thread quitting")) );

        cur = n;
      }
      if( my->blocked )
      {
        //wlog( "still blocking... whats up with that?");
        debug( "on quit" );
      }
    }
    BOOST_ASSERT( my->blocked == 0 );
    //my->blocked = 0;

    for (task_base* unstarted_task : my->task_pqueue)
      unstarted_task->set_exception(std::make_shared<canceled_exception>(FC_LOG_MESSAGE(error, "cancellation reason: thread quitting")));
    my->task_pqueue.clear();

    for (task_base* scheduled_task : my->task_sch_queue)
      scheduled_task->set_exception(std::make_shared<canceled_exception>(FC_LOG_MESSAGE(error, "cancellation reason: thread quitting")));
    my->task_sch_queue.clear();



    // move all sleep tasks to ready
    for( uint32_t i = 0; i < my->sleep_pqueue.size(); ++i )
      my->add_context_to_ready_list( my->sleep_pqueue[i] );
    my->sleep_pqueue.clear();

    // move all idle tasks to ready
    fc::context* cur = my->pt_head;
    while( cur )
    {
      fc::context* n = cur->next;
      cur->next = 0;
      my->add_context_to_ready_list( cur );
      cur = n;
    }

    // mark all ready tasks (should be everyone)... as canceled
    for (fc::context* ready_context : my->ready_heap)
      ready_context->canceled = true;

    // now that we have poked all fibers... switch to the next one and
    // let them all quit.
    while (!my->ready_heap.empty())
    {
      my->start_next_fiber(true);
      my->check_for_timeouts();
    }
    my->clear_free_list();
    my->cleanup_thread_specific_data();
  }

   void thread::exec()
   {
      if( !my->current )
        my->current = new fc::context(&fc::thread::current());

      try
      {
        my->process_tasks();
      }
      catch( canceled_exception& e )
      {
        dlog( "thread canceled: ${e}", ("e", e.to_detail_string()) );
      }
      delete my->current;
      my->current = 0;
   }

   bool thread::is_running()const
   {
      return !my->done;
   }

   priority thread::current_priority()const
   {
      BOOST_ASSERT(my);
      if( my->current )
        return my->current->prio;
      return priority();
   }

   void thread::yield(bool reschedule)
   {
      my->check_fiber_exceptions();
      my->start_next_fiber(reschedule);
      my->check_fiber_exceptions();
   }

   void thread::sleep_until( const time_point& tp )
   {
     if( tp <= (time_point::now()+fc::microseconds(10000)) )
       yield(true);
     my->yield_until( tp, false );
   }

   int  thread::wait_any_until( std::vector<promise_base::ptr>&& p, const time_point& timeout) {
       for( size_t i = 0; i < p.size(); ++i )
         if( p[i]->ready() )
           return i;

       if( timeout < time_point::now() )
       {
         fc::stringstream ss;
         for( auto i = p.begin(); i != p.end(); ++i )
           ss << (*i)->get_desc() << ", ";

         FC_THROW_EXCEPTION( timeout_exception, "${task}", ("task",ss.str()) );
       }

       if( !my->current )
         my->current = new fc::context(&fc::thread::current());

       for( uint32_t i = 0; i < p.size(); ++i )
         my->current->add_blocking_promise(p[i].get(),false);

       // if not max timeout, added to sleep pqueue
       if( timeout != time_point::maximum() )
       {
           my->current->resume_time = timeout;
           my->sleep_pqueue.push_back(my->current);
           std::push_heap( my->sleep_pqueue.begin(),
                           my->sleep_pqueue.end(),
                           sleep_priority_less()   );
       }

       my->add_to_blocked( my->current );
       my->start_next_fiber();

       for( auto i = p.begin(); i != p.end(); ++i )
         my->current->remove_blocking_promise(i->get());

       my->check_fiber_exceptions();

       for( uint32_t i = 0; i < p.size(); ++i )
         if( p[i]->ready() )
           return i;

       //BOOST_THROW_EXCEPTION( wait_any_error() );
       return -1;
   }

   void thread::async_task( task_base* t, const priority& p ) {
     async_task( t, p, time_point::min() );
   }

   void thread::poke() {
     boost::unique_lock<boost::mutex> lock(my->task_ready_mutex);
     my->task_ready.notify_one();
   }

   void thread::async_task( task_base* t, const priority& p, const time_point& tp ) {
      assert(my);
      t->_when = tp;
     // slog( "when %lld", t->_when.time_since_epoch().count() );
     // slog( "delay %lld", (tp - fc::time_point::now()).count() );
      task_base* stale_head = my->task_in_queue.load(boost::memory_order_relaxed);
      do { t->_next = stale_head;
      }while( !my->task_in_queue.compare_exchange_weak( stale_head, t, boost::memory_order_release ) );

      // Because only one thread can post the 'first task', only that thread will attempt
      // to aquire the lock and therefore there should be no contention on this lock except
      // when *this thread is about to block on a wait condition.
      if( this != &current() &&  !stale_head ) {
          boost::unique_lock<boost::mutex> lock(my->task_ready_mutex);
          my->task_ready.notify_one();
      }
   }

   void yield() {
      thread::current().yield();
   }
   void usleep( const microseconds& u ) {
      thread::current().sleep_until( time_point::now() + u);
   }
   void sleep_until( const time_point& tp ) {
      thread::current().sleep_until(tp);
   }

   void  exec()
   {
      return thread::current().exec();
   }

   int wait_any( std::vector<promise_base::ptr>&& v, const microseconds& timeout_us  )
   {
      return thread::current().wait_any_until( fc::move(v), time_point::now() + timeout_us );
   }

   int wait_any_until( std::vector<promise_base::ptr>&& v, const time_point& tp )
   {
      return thread::current().wait_any_until( fc::move(v), tp );
   }

   void thread::wait_until( promise_base::ptr&& p, const time_point& timeout )
   {
         if( p->ready() )
           return;

         if( timeout < time_point::now() )
           FC_THROW_EXCEPTION( timeout_exception, "${task}", ("task", p->get_desc()) );

         if( !my->current )
           my->current = new fc::context(&fc::thread::current());

         //slog( "                                 %1% blocking on %2%", my->current, p.get() );
         my->current->add_blocking_promise(p.get(), true);

         // if not max timeout, added to sleep pqueue
         if( timeout != time_point::maximum() )
         {
             my->current->resume_time = timeout;
             my->sleep_pqueue.push_back(my->current);
             std::push_heap( my->sleep_pqueue.begin(),
                             my->sleep_pqueue.end(),
                             sleep_priority_less() );
         }

       //  elog( "blocking %1%", my->current );
         my->add_to_blocked( my->current );
      //   my->debug("swtiching fibers..." );


         my->start_next_fiber();
        // slog( "resuming %1%", my->current );

         //slog( "                                 %1% unblocking blocking on %2%", my->current, p.get() );
         my->current->remove_blocking_promise(p.get());

         my->check_fiber_exceptions();
    }

    void thread::notify( const promise_base::ptr& p )
    {
      //slog( "this %p  my %p", this, my );
      BOOST_ASSERT(p->ready());
      if( !is_current() )
      {
        this->async( [=](){ notify(p); }, "notify", priority::max() );
        return;
      }
      // TODO: store a list of blocked contexts with the promise
      //  to accelerate the lookup.... unless it introduces contention...

      // iterate over all blocked contexts


      fc::context* cur_blocked  = my->blocked;
      fc::context* prev_blocked = 0;
      while( cur_blocked )
      {
        // if the blocked context is waiting on this promise
        if( cur_blocked->try_unblock( p.get() )  )
        {
          // remove it from the blocked list.

          // remove this context from the sleep queue...
          for( uint32_t i = 0; i < my->sleep_pqueue.size(); ++i )
          {
            if( my->sleep_pqueue[i] == cur_blocked )
            {
              my->sleep_pqueue[i]->blocking_prom.clear();
              my->sleep_pqueue[i] = my->sleep_pqueue.back();
              my->sleep_pqueue.pop_back();
              std::make_heap( my->sleep_pqueue.begin(),my->sleep_pqueue.end(), sleep_priority_less() );
              break;
            }
          }
          auto cur = cur_blocked;
          if( prev_blocked )
          {
              prev_blocked->next_blocked = cur_blocked->next_blocked;
              cur_blocked =  prev_blocked->next_blocked;
          }
          else
          {
              my->blocked = cur_blocked->next_blocked;
              cur_blocked = my->blocked;
          }
          cur->next_blocked = 0;
          my->add_context_to_ready_list( cur );
        }
        else
        { // goto the next blocked task
          prev_blocked  = cur_blocked;
          cur_blocked   = cur_blocked->next_blocked;
        }
      }
    }

    bool thread::is_current()const
    {
      return this == &current();
    }

    void thread::notify_task_has_been_canceled()
    {
      async( [=](){ my->notify_task_has_been_canceled(); }, "notify_task_has_been_canceled", priority::max() );
    }

    void thread::unblock(fc::context* c)
    {
      my->unblock(c);
    }


#ifdef _MSC_VER
    /* support for providing a structured exception handler for async tasks */
    namespace detail
    {
      unhandled_exception_filter_type unhandled_structured_exception_filter = nullptr;
    }
    void set_unhandled_structured_exception_filter(unhandled_exception_filter_type new_filter)
    {
       detail::unhandled_structured_exception_filter = new_filter;
    }
    unhandled_exception_filter_type get_unhandled_structured_exception_filter()
    {
       return detail::unhandled_structured_exception_filter;
    }
#endif // _MSC_VER
} // end namespace fc
