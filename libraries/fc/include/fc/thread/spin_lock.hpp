#pragma once

namespace fc {
  class microseconds;
  class time_point;

  /**
   *  @class spin_lock
   *  @brief modified spin-lock that yields on failure, but becomes a 'spin lock' 
   *         if there are no other tasks to yield to.
   *
   *  This kind of lock is lighter weight than a full mutex, but potentially slower
   *  than a staight spin_lock.
   *
   *  This spin_lock does not block the current thread, but instead attempts to use
   *  an atomic operation to aquire the lock.  If unsuccessful, then it yields to
   *  other tasks before trying again. If there are no other tasks then yield is
   *  a no-op and spin_lock becomes a spin-lock.  
   */
  class spin_lock {
    public:
      spin_lock();
      bool try_lock();
      bool try_lock_for( const microseconds& rel_time );
      bool try_lock_until( const time_point& abs_time );
      void lock(); 
      void unlock(); 
      
    private:
      enum lock_store {locked,unlocked};
      int _lock;
  };

} // namespace fc

