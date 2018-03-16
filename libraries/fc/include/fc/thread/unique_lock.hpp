#pragma once
#include <assert.h>

namespace fc {
  struct try_to_lock_t{};
  class time_point;
  
  /**
   *  Including Boost's unique lock drastically increases compile times
   *  for something that is this trivial!
   */
  template<typename T>
  class unique_lock  {
    public:
      unique_lock( T& l, const fc::time_point& abs ):_lock(l) { _locked = _lock.try_lock_until(abs); }
      unique_lock( T& l, try_to_lock_t ):_lock(l) { _locked = _lock.try_lock(); }
      unique_lock( T& l ): _locked(false), _lock(l)                { lock(); }
      ~unique_lock()                              { if (_locked) unlock(); }
      operator bool() const { return _locked; }
      void unlock()                               { assert(_locked);  if (_locked) { _lock.unlock(); _locked = false;} }
      void lock()                                 { assert(!_locked); if (!_locked) { _lock.lock(); _locked = true; } }
    private:
      unique_lock( const unique_lock& );
      unique_lock& operator=( const unique_lock& );
      bool _locked;
      T&  _lock;
  };

}

/**
 *  Emulate java with the one quirk that the open bracket must come before
 *  the synchronized 'keyword'. 
 *  
 *  <code>
    { synchronized( lock_type ) 

    }
 *  </code>
 */
#define synchronized(X)  fc::unique_lock<decltype((X))> __lock(((X)));

