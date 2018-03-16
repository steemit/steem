#pragma once

#define FC_CONTEXT_STACK_SIZE (2048*1024)

#include <fc/thread/task.hpp>
#include <fc/vector.hpp>
#include <fc/string.hpp>

namespace fc {
  class time_point;
  class microseconds;

   namespace detail
   {
      void* get_thread_specific_data(unsigned slot);
      void set_thread_specific_data(unsigned slot, void* new_value, void(*cleanup)(void*));
      unsigned get_next_unused_task_storage_slot();
      void* get_task_specific_data(unsigned slot);
      void set_task_specific_data(unsigned slot, void* new_value, void(*cleanup)(void*));
   }

  class thread {
    public:
      thread( const std::string& name = "" );
      thread( thread&& m );
      thread& operator=(thread&& t );

      /**
       *  Returns the current thread.
          Note: Creates fc::thread object (but not a boost thread) if no current thread assigned yet 
                (this can happend if current() is called from the main thread of application or from
                an existing "unknown" boost thread). In such cases, thread_d doesn't have access boost::thread object.
       */
      static thread& current();

     
      /**
       *  @brief returns the name given by @ref set_name() for this thread
       */
      const string& name()const;

      /**
       *  @brief associates a name with this thread.
       */
      void        set_name( const string& n );
       
      const char* current_task_desc() const;

      /**
       *  @brief print debug info about the state of every context / promise.
       *
       *  This method is helpful to figure out where your program is 'hung' by listing
       *  every async operation (context) and what it is blocked on (future).  
       *
       *  @note debug info is more useful if you provide a description for your
       *  async tasks and promises.
       */
      void    debug( const fc::string& d );
     
     
      /**
       *  Calls function <code>f</code> in this thread and returns a future<T> that can
       *  be used to wait on the result.  
       *
       *  @param f the operation to perform
       *  @param prio the priority relative to other tasks
       */
      template<typename Functor>
      auto async( Functor&& f, const char* desc FC_TASK_NAME_DEFAULT_ARG, priority prio = priority()) -> fc::future<decltype(f())> {
         typedef decltype(f()) Result;
         typedef typename fc::deduce<Functor>::type FunctorType;
         fc::task<Result,sizeof(FunctorType)>* tsk = 
              new fc::task<Result,sizeof(FunctorType)>( fc::forward<Functor>(f), desc );
         fc::future<Result> r(fc::shared_ptr< fc::promise<Result> >(tsk,true) );
         async_task(tsk,prio);
         return r;
      }
      void poke();
     
     
      /**
       *  Calls function <code>f</code> in this thread and returns a future<T> that can
       *  be used to wait on the result.
       *
       *  @param f the method to be called
       *  @param prio the priority of this method relative to others
       *  @param when determines when this call will happen, as soon as 
       *        possible after <code>when</code>
       */
      template<typename Functor>
      auto schedule( Functor&& f, const fc::time_point& when, 
                     const char* desc FC_TASK_NAME_DEFAULT_ARG, priority prio = priority()) -> fc::future<decltype(f())> {
         typedef decltype(f()) Result;
         fc::task<Result,sizeof(Functor)>* tsk = 
              new fc::task<Result,sizeof(Functor)>( fc::forward<Functor>(f), desc );
         fc::future<Result> r(fc::shared_ptr< fc::promise<Result> >(tsk,true) );
         async_task(tsk,prio,when);
         return r;
      }
     
      /**
       *  This method will cancel all pending tasks causing them to throw cmt::error::thread_quit.
       *
       *  If the <i>current</i> thread is not <code>this</code> thread, then the <i>current</i> thread will
       *  wait for <code>this</code> thread to exit.
       *
       *  This is a blocking wait via <code>boost::thread::join</code>
       *  and other tasks in the <i>current</i> thread will not run while 
       *  waiting for <code>this</code> thread to quit.
       *
       *  @todo make quit non-blocking of the calling thread by eliminating the call to <code>boost::thread::join</code>
       */
      void quit();
     
      /**
       *  @return true unless quit() has been called.
       */
      bool is_running()const;
      bool is_current()const;
     
      priority current_priority()const;
      ~thread();

       template<typename T1, typename T2>
       int wait_any( const fc::future<T1>& f1, const fc::future<T2>& f2, const microseconds& timeout_us = microseconds::maximum()) {
          std::vector<fc::promise_base::ptr> proms(2);
          proms[0] = fc::static_pointer_cast<fc::promise_base>(f1.m_prom);
          proms[1] = fc::static_pointer_cast<fc::promise_base>(f2.m_prom);
          return wait_any_until(fc::move(proms), fc::time_point::now()+timeout_us );
       }
    private:
      thread( class thread_d* );
      friend class promise_base;
      friend class task_base;
      friend class thread_d;
      friend class mutex;
      friend void* detail::get_thread_specific_data(unsigned slot);
      friend void detail::set_thread_specific_data(unsigned slot, void* new_value, void(*cleanup)(void*));
      friend unsigned detail::get_next_unused_task_storage_slot();
      friend void* detail::get_task_specific_data(unsigned slot);
      friend void detail::set_task_specific_data(unsigned slot, void* new_value, void(*cleanup)(void*));
#ifndef NDEBUG
      friend class non_preemptable_scope_check;
#endif
      friend void yield();
      friend void usleep(const microseconds&);
      friend void sleep_until(const time_point&);
      friend void exec();
      friend int wait_any( std::vector<promise_base::ptr>&& v, const microseconds& );
      friend int wait_any_until( std::vector<promise_base::ptr>&& v, const time_point& tp );
      void wait_until( promise_base::ptr && v, const time_point& tp );
      void notify( const promise_base::ptr& v );

      void yield(bool reschedule=true);
      void sleep_until( const time_point& t );
      void  exec();
      int  wait_any_until( std::vector<promise_base::ptr>&& v, const time_point& );

      void async_task( task_base* t, const priority& p );
      void async_task( task_base* t, const priority& p, const time_point& tp );

      void notify_task_has_been_canceled();
      void unblock(fc::context* c);

      class thread_d* my;
  };

  /** 
   *  Yields to other ready tasks before returning.
   */
   void yield();

   /** 
    *  Yields to other ready tasks for u microseconds.
    */
   void usleep( const microseconds& u );

   /** 
    *  Yields until the specified time in the future.
    */
   void sleep_until( const time_point& tp );

   /**
    *  Enters the main loop processing tasks until quit() is called.
    */
   void  exec();

   /**
    * Wait until either f1 or f2 is ready.
    *
    * @return 0 if f1 is ready, 1 if f2 is ready or throw on error.
    */
   template<typename T1, typename T2>
   int wait_any( const fc::future<T1>& f1, const fc::future<T2>& f2, const microseconds timeout_us = microseconds::maximum()) {
      return fc::thread::current().wait_any(f1,f2,timeout_us);
   }
   int wait_any( std::vector<promise_base::ptr>&& v, const microseconds& timeout_us = microseconds::maximum() );
   int wait_any_until( std::vector<promise_base::ptr>&& v, const time_point& tp );

   template<typename Functor>
   auto async( Functor&& f, const char* desc FC_TASK_NAME_DEFAULT_ARG, priority prio = priority()) -> fc::future<decltype(f())> {
      return fc::thread::current().async( fc::forward<Functor>(f), desc, prio );
   }
   template<typename Functor>
   auto schedule( Functor&& f, const fc::time_point& t, const char* desc FC_TASK_NAME_DEFAULT_ARG, priority prio = priority()) -> fc::future<decltype(f())> {
      return fc::thread::current().schedule( fc::forward<Functor>(f), t, desc, prio );
   }

  /**
   * Call f() in thread t and block the current thread until it returns.
   * 
   * If t is null, simply execute f in the current thread.
   */
  template<typename Functor>
  auto sync_call( thread* t, Functor&& f, const char* desc FC_TASK_NAME_DEFAULT_ARG, priority prio = priority()) -> decltype(f())
  {
     if( t == nullptr )
         return f();

     typedef decltype(f()) Result;
     future<Result> r = t->async( f, desc, prio );
     return r.wait();
  }

} // end namespace fc

#ifdef _MSC_VER
struct _EXCEPTION_POINTERS;

namespace fc {
   /* There's something about the setup of the stacks created for fc::async tasks
    * that screws up the global structured exception filters installed by
    * SetUnhandledExceptionFilter().  The only way I've found to catch an 
    * unhaldned structured exception thrown in an async task is to put a 
    * __try/__except block inside the async task.
    * We do just that, and if a SEH escapes outside the function running 
    * in the async task, fc will call an exception filter privided by 
    * set_unhandled_structured_exception_filter(), passing as arguments
    * the result of GetExceptionCode() and GetExceptionInformation().
    *
    * Right now there is only one global exception filter, used for any 
    * async task.
    */
   typedef int (*unhandled_exception_filter_type)(unsigned, _EXCEPTION_POINTERS*);
   void set_unhandled_structured_exception_filter(unhandled_exception_filter_type new_filter);
   unhandled_exception_filter_type get_unhandled_structured_exception_filter();
}
#endif

