#pragma once
#include <fc/time.hpp>
#include <fc/thread/spin_yield_lock.hpp>

#include <memory>

namespace fc {
  class microseconds;
  class time_point;
  class path;
  struct context;

  namespace detail { class file_mutex_impl; }

  /**
   *  The purpose of this class is to support synchronization of
   *  processes, threads, and coop-threads.
   *
   *  Before grabbing the lock for a thread or coop, a file_mutex will first
   *  grab a process-level lock.  After grabbing the process level lock, it will
   *  synchronize in the same way as a local process lock.
   */
  class file_mutex {
     public:
        file_mutex( const fc::path& filename );
        ~file_mutex();

        bool try_lock();
        bool try_lock_for( const microseconds& rel_time );
        bool try_lock_until( const time_point& abs_time );
        void lock();
        void unlock();

        void lock_shared();
        void unlock_shared();
        bool try_lock_shared();

        int  readers()const;

     private:
        std::unique_ptr<detail::file_mutex_impl> my;
  };

} // namespace fc
