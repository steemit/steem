#pragma once
#include <fc/time.hpp>
#include <fc/thread/spin_yield_lock.hpp>

namespace fc { 
  class microseconds;
  class time_point;
  struct context;
  
  /**
   *  @brief mutex
   *
   *  This mutex has an advantage over boost::mutex in that it does
   *  not involve any system calls, even in contention.    
   *
   *  Uncontensted access is a simple compare and swap, no delay.
   *
   *  Contested access by different fibers in the same thread simply 
   *    yields the thread until the lock is available. Actual delay
   *    is subject to the cooperative nature of other tasks in the
   *    fiber's thread.
   *
   *  Contested access by different threads requires a spin lock
   *    while the task is enqueued. Because the enqueue action is
   *    well-defined and 'short-lived' time spent 'spinning' should
   *    be minimal.
   *
   *  Cooperatively multi-tasked code must still worry about 
   *  reentrancy.  Suppose you have a thread sending a message across a socket,
   *  the socket members are thread safe, but the <code>write_message()</code> operation is
   *  not rentrant because the context could yield while waiting for a partial
   *  write to complete.
   *
   *  If while it has yielded another task in the same thread attempts to write
   *  a second message then you will get garbage out as both fibers take
   *  turns writing parts of their messages out of the socket.
   *
   *  Example problem:
   *  @code
   *    async(write_message);
   *    async(write_message);
   *    void write_message() {
   *      sock->write(part1); // may yield
   *      sock->write(part2); // may yield
   *      sock->write(part3); // may yield
   *    }
   *  @endcode
   *
   *  The output could look something like:
   *
   *  @code
   *    part1
   *    part2
   *    part1
   *    part3
   *    part2
   *    part3
   *  @endcode
   *
   *  What you want to happen is this:
   *
   *  @code
   *    void write_message() {
   *      boost::unique_lock<fc::mutex> lock(sock->write_lock);
   *      sock->write(part1); // may yield
   *      sock->write(part2); // may yield
   *      sock->write(part3); // may yield
   *    }
   *  @endcode
   *
   *  Now if while writing the first message, someone attempts to
   *  write a second message second write will 'queue' behind the
   *  first by 'blocking' on the mutex.  
   *
   *  As a result we now have to extend the normal discussion on thread-safe vs reentrant.
   *
   *  - prempt-thread-safe : the code may be called by multiple os threads at the same time.
   *  - coop-thread-safe   : the code may be called by multiple contexts within the same thread.
   *  - thread-unsafe      : the code may only be called by one context at a time for a set of data. 
   *
   *  In the example above, before we added the mutex the code was thread-unsafe
   *  After we added the mutex the code became coop-thread-safe, and potentially prempt-thread-safe
   *
   *  To be preempt-thread-safe any operations must be atomic or protected by a lock because
   *  the OS could switch you out between any two instructions.
   *
   *  To be coop-thread-safe all operations are 'atomic' unless they span a 'yield'.  If they
   *  span a yield (such as writing parts of a message), then a mutex is required.
   *
   */
  class mutex {
    public:
      mutex();
      ~mutex();

      bool try_lock();
      bool try_lock_for( const microseconds& rel_time );
      bool try_lock_until( const time_point& abs_time );
      void lock();
      void unlock();

    private:
      fc::spin_yield_lock   m_blist_lock;
      fc::context*          m_blist;
      unsigned              recursive_lock_count;
  };
  
} // namespace fc

