#pragma once
#include <boost/interprocess/sync/interprocess_upgradable_mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <fc/array.hpp>

#ifndef CHAINBASE_NUM_RW_LOCKS
   #define CHAINBASE_NUM_RW_LOCKS 10
#endif

namespace graphene { namespace db2 {

   typedef boost::interprocess::interprocess_upgradable_mutex read_write_mutex;
   typedef boost::shared_lock< read_write_mutex > read_lock;
   typedef boost::unique_lock< read_write_mutex > write_lock;

   /**
    * The purpose of this class is to implement a shared read write lock for use
    * in the memory mapped file. The requirements of the lock are as follows.
    *
    * 1. There must only be one writer at a time or multiple readers.
    * 2. Requesting a write lock must preempt future readers.
    * 3. If a read process dies mid read the lock must automatically release.
    * 4. If a write process dies mid write, the database may be corrupted and
    *    future access should be prohibited.
    *
    * This lock is implemented as an exclusive write mutex and shared read semaphore.
    * To implement timing, a write process waiting on existing readers will "pseudo" busy
    * wait for the time to expire and then will be able to take full control of the lock.
    */
   class read_write_mutex_manager
   {
      public:
         read_write_mutex_manager()
         {
            _current_lock = 0;
         }

         ~read_write_mutex_manager(){}

         void next_lock()
         {
            _current_lock++;
            new( &_locks[ _current_lock ] ) read_write_mutex();
         }

         read_write_mutex& current_lock()
         {
            return _locks[ _current_lock ];
         }

      private:
         fc::array< read_write_mutex, CHAINBASE_NUM_RW_LOCKS > _locks;
         std::atomic< uint32_t >                               _current_lock;
   };


   /**
    * This class is designed to be a local wrapper around the shared_timed_rw_lock
    * that implements both reentrant locking but allows for interleaving of read
    * and write locking withing a single thread. This class is not thread safe and
    * should not be accessed concurrently.
    */
   /*class timed_rw_lock
   {
      public:
         timed_rw_lock( shared_timed_rw_lock* lock ) :_lock( lock ){}

         timed_rw_lock(){}


         void write_lock()
         {
            if( !_current_writes )
               _lock->write_lock();

            _current_writes++;
         }

         bool try_write_lock()
         {
            if( !_current_writes )
               if( _lock->try_write_lock() )
                  _current_writes++;

            return _current_writes > 0;
         }

         void write_unlock()
         {
            FC_ASSERT( _current_writes, "Cannot unlock a write lock that has not been acquired" );
            _current_writes--;

            if( !_current_writes )
            {
               if( _current_reads )
                  _lock->read_lock();

               _lock->write_unlock();
            }
         }

         void read_lock()
         {
            if( !_current_reads && !_current_writes )
               _lock->read_lock();

            _current_reads++;
         }

         bool try_read_lock()
         {
            if( !_current_reads && !_current_writes )
               if( _lock->try_read_lock() )
                  _current_reads++;

            return _current_reads > 0;
         }

         void read_unlock()
         {
            FC_ASSERT( _current_reads, "Cannot unlock a read lock that has not been acquired" );
            _current_reads--;

            if( !_current_reads && !_current_writes )
               _lock->read_unlock();
         }

         void reset()
         {
            _lock->reset();
            _current_reads = 0;
            _current_writes = 0;
         }

      private:
         shared_timed_rw_lock* _lock = nullptr;
         uint16_t              _current_reads = 0;
         uint16_t              _current_writes = 0;
   };*/


   /**
    * This class is a scoped read lock guard on timed_rw_lock
    * It guarantees the lock will be unlocked with the guard goes
    * out of scope.
    */
   /*struct read_guard
   {
      read_guard( timed_rw_lock& rw ) : _rw( rw )
      {
         _rw.read_lock();
      }

      ~read_guard()
      {
         _rw.read_unlock();
      }

      timed_rw_lock& _rw;
   };
   */

   /**
    * This class is a scoped write lock guard on timed_rw_lock
    * It guarantees the lock will be unlocked with the guard goes
    * out of scope.
    */
   /*struct write_guard
   {
      write_guard( timed_rw_lock& rw ) : _rw( rw )
      {
         _rw.write_lock();
      }

      ~write_guard()
      {
         _rw.write_unlock();
      }

      timed_rw_lock& _rw;
   };
   */

} } // graphene::db2