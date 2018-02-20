#pragma once

namespace fc {
   template<typename T>
   class scoped_lock {
      public:
         scoped_lock( T& l ):_lock(l) { _lock.lock();   }
         ~scoped_lock()               { _lock.unlock(); }
         T& _lock;
   };
}
