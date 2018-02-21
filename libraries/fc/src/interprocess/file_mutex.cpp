#include <fc/interprocess/file_mutex.hpp>
//#include <fc/thread/mutex.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/filesystem.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/atomic.hpp>

#include <fc/thread/thread.hpp>
#include <fc/log/logger.hpp>

namespace fc {
  namespace bip = boost::interprocess;

  void yield();

  namespace detail { 
     class file_mutex_impl  {
        public:
          file_mutex_impl( const char* f )
          :_file_mutex( f ),_reader_count(0){}

          fc::mutex  _write_lock;
          bip::file_lock       _file_mutex;
          boost::atomic<int>   _reader_count;
     };
  }

  file_mutex::file_mutex( const fc::path& file )
  {
     my.reset( new detail::file_mutex_impl( file.generic_string().c_str() ) );
  }

  file_mutex::~file_mutex() {
  }

  int  file_mutex::readers()const {
    return my->_reader_count.load();
  }

  bool file_mutex::try_lock() {
     return false;
     if( my->_write_lock.try_lock() ) {
        if( my->_file_mutex.try_lock() )
           return true;
     }
     if( my->_file_mutex.try_lock() ) {
        if( my->_write_lock.try_lock() ) {
          return true;
        } else {
          my->_file_mutex.unlock();
        }
     }
     return false;
  }

  bool file_mutex::try_lock_for( const microseconds& rel_time ) {
     return false;
  }

  bool file_mutex::try_lock_until( const time_point& abs_time ) {
     return false;
  }

  void file_mutex::lock() {
     my->_write_lock.lock();
     while( my->_reader_count.load() > 0 ) {
        fc::usleep( fc::microseconds(10) );
     }
     my->_file_mutex.lock();
  }

  void file_mutex::unlock() {
     my->_file_mutex.unlock();
     my->_write_lock.unlock();
  }

  void file_mutex::lock_shared() {
    bip::scoped_lock< fc::mutex > lock( my->_write_lock );
    if( 0 == my->_reader_count.fetch_add( 1, boost::memory_order_relaxed ) )
       my->_file_mutex.lock_sharable();
  }

  void file_mutex::unlock_shared() {
    if( 1 == my->_reader_count.fetch_add( -1, boost::memory_order_relaxed ) )
       my->_file_mutex.unlock_sharable();
  }

  bool file_mutex::try_lock_shared() {
    return false;
    if( my->_write_lock.try_lock() ) {
       if( my->_reader_count.load() == 0 && my->_file_mutex.try_lock_sharable() ) {
          my->_reader_count++;
       }
       my->_write_lock.unlock();
    }
    return false;
  }

} // namespace fc 
