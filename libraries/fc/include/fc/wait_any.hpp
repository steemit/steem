#pragma once
#include <fc/vector.hpp>
#include <fc/thread/thread.hpp>

namespace fc {
   template<typename T1, typename T2>
   int wait_any( fc::future<T1>& f1, fc::future<T2>& f2, const microseconds& timeout_us = microseconds::max() ) {
     fc::vector<promise_base::ptr> p(2);
     p[0] = static_pointer_cast<promise_base*>(f1.promise());
     p[1] = static_pointer_cast<promise_base*>(f2.promise());
     return wait( fc::move(p), timeout_us );   
   }
}
