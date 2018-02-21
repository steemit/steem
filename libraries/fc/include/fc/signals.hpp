#pragma once 

#if defined(_MSC_VER) && _MSC_VER >= 1400 
#pragma warning(push) 
#pragma warning(disable:4996) 
#endif 

#include <boost/signals2/signal.hpp>

#if defined(_MSC_VER) && _MSC_VER >= 1400 
#pragma warning(pop) 
#endif 

#include <fc/thread/future.hpp>
#include <fc/thread/thread.hpp>

namespace fc {
#if !defined(BOOST_NO_TEMPLATE_ALIASES) 
   template<typename T>
   using signal = boost::signals2::signal<T>;

   using scoped_connection = boost::signals2::scoped_connection;
#else
  /** Workaround for missing Template Aliases feature in the VS 2012.
      \warning Class defined below cannot have defined constructor (even base class has it)
      since it is impossible to reference directly template class arguments outside this class.
      This code will work until someone will use non-default constructor as it is defined in
      boost::signals2::signal.
  */
  template <class T>
  class signal : public boost::signals2::signal<T>
    {
    public:
    };
#endif

   template<typename T>
   inline T wait( boost::signals2::signal<void(T)>& sig, const microseconds& timeout_us=microseconds::maximum() ) {
     typename promise<T>::ptr p(new promise<T>("fc::signal::wait"));
     boost::signals2::scoped_connection c( sig.connect( [=]( T t ) { p->set_value(t); } )); 
     return p->wait( timeout_us ); 
   }

   inline void wait( boost::signals2::signal<void()>& sig, const microseconds& timeout_us=microseconds::maximum() ) {
     promise<void>::ptr p(new promise<void>("fc::signal::wait"));
     boost::signals2::scoped_connection c( sig.connect( [=]() { p->set_value(); } )); 
     p->wait( timeout_us ); 
   }
} 
