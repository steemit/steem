#pragma once
#include <fc/thread/future.hpp>
#include <fc/thread/priority.hpp>
#include <fc/aligned.hpp>
#include <fc/fwd.hpp>

namespace fc {
  struct context;
  class spin_lock;

   namespace detail
   {
      struct specific_data_info
      {
         void* value;
         void (*cleanup)(void*);
         specific_data_info() :
            value(0),
            cleanup(0)
            {}
         specific_data_info(void* value, void (*cleanup)(void*)) :
            value(value),
            cleanup(cleanup)
         {}
      };
      void* get_task_specific_data(unsigned slot);
      void set_task_specific_data(unsigned slot, void* new_value, void(*cleanup)(void*));
   }

  class task_base : virtual public promise_base {
    public:
              void run(); 
      virtual void cancel(const char* reason FC_CANCELATION_REASON_DEFAULT_ARG) override;

    protected:
      ~task_base();
      /// Task priority looks like unsupported feature.
      uint64_t    _posted_num;
      priority    _prio;
      time_point  _when;
      void        _set_active_context(context*);
      context*    _active_context;
      task_base*  _next;

      // support for task-specific data
      std::vector<detail::specific_data_info> *_task_specific_data;

      friend void* detail::get_task_specific_data(unsigned slot);
      friend void detail::set_task_specific_data(unsigned slot, void* new_value, void(*cleanup)(void*));

      task_base(void* func);
      // opaque internal / private data used by
      // thread/thread_private
      friend class thread;
      friend class thread_d;
      fwd<spin_lock,8> _spinlock;

      // avoid rtti info for every possible functor...
      void*         _promise_impl;
      void*         _functor;
      void          (*_destroy_functor)(void*);
      void          (*_run_functor)(void*, void* );

      void          run_impl(); 

      void cleanup_task_specific_data();
  };

  namespace detail {
    template<typename T>
    struct functor_destructor {
      static void destroy( void* v ) { ((T*)v)->~T(); }
    };
    template<typename T>
    struct functor_run {
      static void run( void* functor, void* prom ) {
        ((promise<decltype((*((T*)functor))())>*)prom)->set_value( (*((T*)functor))() );
      }
    };
    template<typename T>
    struct void_functor_run {
      static void run( void* functor, void* prom ) {
        (*((T*)functor))();
        ((promise<void>*)prom)->set_value();
      }
    };
  }

  template<typename R,uint64_t FunctorSize=64>
  class task : virtual public task_base, virtual public promise<R> {
    public:
      template<typename Functor>
      task( Functor&& f, const char* desc ):promise_base(desc), task_base(&_functor), promise<R>(desc) {
        typedef typename fc::deduce<Functor>::type FunctorType;
        static_assert( sizeof(f) <= sizeof(_functor), "sizeof(Functor) is larger than FunctorSize" );
        new ((char*)&_functor) FunctorType( fc::forward<Functor>(f) );
        _destroy_functor = &detail::functor_destructor<FunctorType>::destroy;

        _promise_impl = static_cast<promise<R>*>(this);
        _run_functor  = &detail::functor_run<FunctorType>::run;
      }
      virtual void cancel(const char* reason FC_CANCELATION_REASON_DEFAULT_ARG) override { task_base::cancel(reason); }

      aligned<FunctorSize> _functor;
    private:
      ~task(){}
  };

  template<uint64_t FunctorSize>
  class task<void,FunctorSize> : virtual public task_base, virtual public promise<void> {
    public:
      template<typename Functor>
      task( Functor&& f, const char* desc ):promise_base(desc), task_base(&_functor), promise<void>(desc) {
        typedef typename fc::deduce<Functor>::type FunctorType;
        static_assert( sizeof(f) <= sizeof(_functor), "sizeof(Functor) is larger than FunctorSize"  );
        new ((char*)&_functor) FunctorType( fc::forward<Functor>(f) );
        _destroy_functor = &detail::functor_destructor<FunctorType>::destroy;

        _promise_impl = static_cast<promise<void>*>(this);
        _run_functor  = &detail::void_functor_run<FunctorType>::run;
      }
      virtual void cancel(const char* reason FC_CANCELATION_REASON_DEFAULT_ARG) override { task_base::cancel(reason); }

      aligned<FunctorSize> _functor;      
    private:
      ~task(){}
  };

}
