#pragma once
#include <functional>
#include <memory>
#include <fc/vector.hpp>
#include <fc/log/logger.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/unique_lock.hpp>
#ifdef emit
#undef emit
#endif

namespace fc {
  
  template<typename Signature>
  class signal  {
    private:
       typedef std::function<Signature> func_type;
       typedef std::vector<std::shared_ptr<func_type>> list_type;
    public:
      typedef void* connection_id_type;

      template<typename Functor>
      connection_id_type connect( Functor&& f ) {
         fc::unique_lock<fc::mutex> lock(_mutex);
         //auto c = new std::function<Signature>( fc::forward<Functor>(f) ); 
         _handlers.push_back( std::make_shared<func_type>(f) );
         return reinterpret_cast<connection_id_type>(_handlers.back().get());
      }
#ifdef WIN32
      template<typename Arg>
      void emit( Arg&& arg ) {
        list_type handlers = getHandlers();
        for( size_t i = 0; i < handlers.size(); ++i ) {
          (*handlers[i])( fc::forward<Arg>(arg) );
        }
      }
      void operator()() {
        list_type handlers = getHandlers();
        for( size_t i = 0; i < handlers.size(); ++i ) {
          (*handlers[i])();
        }
      }
      template<typename Arg>
      void operator()( Arg&& arg ) {
        list_type handlers = getHandlers();
        for( size_t i = 0; i < handlers.size(); ++i ) {
          (*handlers[i])( fc::forward<Arg>(arg) );
        }
      }
      template<typename Arg,typename Arg2>
      void emit( Arg&& arg, Arg2&& arg2 ) {
        list_type handlers = getHandlers();
        for( size_t i = 0; i < handlers.size(); ++i ) {
          (*handlers[i])( fc::forward<Arg>(arg), fc::forward<Arg2>(arg2) );
        }
      }
      template<typename Arg, typename Arg2>
      void operator()( Arg&& arg, Arg2&& arg2 ) {
        list_type handlers = getHandlers();
        for( size_t i = 0; i < handlers.size(); ++i ) {
          (*handlers[i])( fc::forward<Arg>(arg), fc::forward<Arg2>(arg2) );
        }
      }
      template<typename Arg, typename Arg2, typename Arg3>
      void emit( Arg&& arg, Arg2&& arg2, Arg3&& arg3 ) {
        list_type handlers = getHandlers();
        for( size_t i = 0; i < handlers.size(); ++i ) {
          (*handlers[i])( fc::forward<Arg>(arg), fc::forward<Arg2>(arg2), fc::forward<Arg3>(arg3) );
        }
      }
      template<typename Arg, typename Arg2, typename Arg3>
      void operator()( Arg&& arg, Arg2&& arg2, Arg3&& arg3 ) {
        list_type handlers = getHandlers();
        for( size_t i = 0; i < handlers.size(); ++i ) {
          (*handlers[i])( fc::forward<Arg>(arg), fc::forward<Arg2>(arg2), fc::forward<Arg3>(arg3) );
        }
      }
#else
      template<typename... Args>
      void emit( Args&&... args ) {
        list_type handlers = getHandlers();
        for( size_t i = 0; i < handlers.size(); ++i ) {
          (*handlers[i])( fc::forward<Args>(args)... );
        }
      }
      template<typename... Args>
      void operator()( Args&&... args ) {
        list_type handlers = getHandlers();
        for( size_t i = 0; i < handlers.size(); ++i ) {
          (*handlers[i])( fc::forward<Args>(args)... );
        }
      }
#endif

      void disconnect( connection_id_type cid ) {
        fc::unique_lock<fc::mutex> lock(_mutex);
        auto itr = _handlers.begin();
        while( itr != _handlers.end() ) {
          if( reinterpret_cast<connection_id_type>(itr->get()) == cid ) {
            _handlers.erase(itr);
          }
          ++itr;
        }
      }
      signal()
      {
         _handlers.reserve(4);
      }
      //~signal()
      //{
      //   for( auto itr = _handlers.begin(); itr != _handlers.end(); ++itr )
      //   {
      //      delete *itr;
      //   }
      //   _handlers.clear();
      //}

    private:
      fc::mutex _mutex;
      list_type _handlers;

      list_type getHandlers()
      {
         fc::unique_lock<fc::mutex> lock(_mutex);
         list_type handlers(_handlers);
         return handlers;
      }
  };

}
