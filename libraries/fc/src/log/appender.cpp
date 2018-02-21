#include <fc/log/appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/thread/unique_lock.hpp>
#include <unordered_map>
#include <string>
#include <fc/thread/spin_lock.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <fc/log/console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/gelf_appender.hpp>
#include <fc/variant.hpp>
#include <fc/macros.hpp>
#include "console_defines.h"


namespace fc {

   std::unordered_map<std::string,appender::ptr>& get_appender_map() {
     static std::unordered_map<std::string,appender::ptr> lm;
     return lm;
   }
   std::unordered_map<std::string,appender_factory::ptr>& get_appender_factory_map() {
     static std::unordered_map<std::string,appender_factory::ptr> lm;
     return lm;
   }
   appender::ptr appender::get( const fc::string& s ) {
     static fc::spin_lock appender_spinlock;
      scoped_lock<spin_lock> lock(appender_spinlock);
      return get_appender_map()[s];
   }
   bool appender::register_appender( const fc::string& type, const appender_factory::ptr& f )
   {
      get_appender_factory_map()[type] = f;
      return true;
   }
   appender::ptr appender::create( const fc::string& name, const fc::string& type, const variant& args  )
   {
      auto fact_itr = get_appender_factory_map().find(type);
      if( fact_itr == get_appender_factory_map().end() ) {
         //wlog( "Unknown appender type '%s'", type.c_str() );
         return appender::ptr();
      }
      auto ap = fact_itr->second->create( args );
      get_appender_map()[name] = ap;
      return ap;
   }

   /*
    Assiging a function return to a static variable allows code exectution on
    initialization. In a perfect world, we would not do this. This results in an
    unused variable warning. Passing the pointer to the variable in to the lambda
    allows us to safely mark the variable as unused. (void)var does not work
    because we cannot execute in this block.
   */
   static bool reg_console_appender = []( __attribute__((unused)) bool* )->bool
   {
      return appender::register_appender<console_appender>( "console" );
   }( &reg_console_appender );

   static bool reg_file_appender = []( __attribute__((unused)) bool* )->bool
   {
      return appender::register_appender<file_appender>( "file" );
   }( &reg_file_appender );

   static bool reg_gelf_appender = []( __attribute__((unused)) bool* )->bool
   {
      return appender::register_appender<gelf_appender>( "gelf" );
   }( &reg_gelf_appender );

} // namespace fc
