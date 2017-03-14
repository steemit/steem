#include <chainbase/chainbase.hpp>
#include <boost/array.hpp>

#include <iostream>

namespace chainbase {

   struct environment_check {
      environment_check() {
         memset( &compiler_version, 0, sizeof( compiler_version ) );
         memcpy( &compiler_version, __VERSION__, std::min<size_t>( strlen(__VERSION__), 256 ) );
#ifndef NDEBUG
         debug = true;
#endif
#ifdef __APPLE__
         apple = true;
#endif
#ifdef WIN32
         windows = true;
#endif
      }
      friend bool operator == ( const environment_check& a, const environment_check& b ) {
         return std::make_tuple( a.compiler_version, a.debug, a.apple, a.windows )
            ==  std::make_tuple( b.compiler_version, b.debug, b.apple, b.windows );
      }

      boost::array<char,256>  compiler_version;
      bool                    debug = false;
      bool                    apple = false;
      bool                    windows = false;
   };

   void database::open( const bfs::path& dir, uint32_t flags, uint64_t shared_file_size ) {

      bool write = flags & database::read_write;

      if( !bfs::exists( dir ) ) {
         if( !write ) BOOST_THROW_EXCEPTION( std::runtime_error( "database file not found at " + dir.native() ) );
      }

      bfs::create_directories( dir );
      if( _data_dir != dir ) close();

      _data_dir = dir;
      auto abs_path = bfs::absolute( dir / "shared_memory.bin" );

      if( bfs::exists( abs_path ) )
      {
         if( write )
         {
            auto existing_file_size = bfs::file_size( abs_path );
            if( shared_file_size > existing_file_size )
            {
               if( !bip::managed_mapped_file::grow( abs_path.generic_string().c_str(), shared_file_size - existing_file_size ) )
                  BOOST_THROW_EXCEPTION( std::runtime_error( "could not grow database file to requested size." ) );
            }

            _segment.reset( new bip::managed_mapped_file( bip::open_only,
                                                          abs_path.generic_string().c_str()
                                                          ) );
         } else {
            _segment.reset( new bip::managed_mapped_file( bip::open_read_only,
                                                          abs_path.generic_string().c_str()
                                                          ) );
            _read_only = true;
         }

         auto env = _segment->find< environment_check >( "environment" );
         if( !env.first || !( *env.first == environment_check()) ) {
            BOOST_THROW_EXCEPTION( std::runtime_error( "database created by a different compiler, build, or operating system" ) );
         }
      } else {
         _segment.reset( new bip::managed_mapped_file( bip::create_only,
                                                       abs_path.generic_string().c_str(), shared_file_size
                                                       ) );
         _segment->find_or_construct< environment_check >( "environment" )();
      }


      abs_path = bfs::absolute( dir / "shared_memory.meta" );

      if( bfs::exists( abs_path ) )
      {
         _meta.reset( new bip::managed_mapped_file( bip::open_only, abs_path.generic_string().c_str()
                                                    ) );

         _rw_manager = _meta->find< read_write_mutex_manager >( "rw_manager" ).first;
         if( !_rw_manager )
            BOOST_THROW_EXCEPTION( std::runtime_error( "could not find read write lock manager" ) );
      }
      else
      {
         _meta.reset( new bip::managed_mapped_file( bip::create_only,
                                                    abs_path.generic_string().c_str(), sizeof( read_write_mutex_manager ) * 2
                                                    ) );

         _rw_manager = _meta->find_or_construct< read_write_mutex_manager >( "rw_manager" )();
      }

      if( write )
      {
         _flock = bip::file_lock( abs_path.generic_string().c_str() );
         if( !_flock.try_lock() )
            BOOST_THROW_EXCEPTION( std::runtime_error( "could not gain write access to the shared memory file" ) );
      }
   }

   void database::flush() {
      if( _segment )
         _segment->flush();
      if( _meta )
         _meta->flush();
   }

   void database::close()
   {
      _segment.reset();
      _meta.reset();
      _data_dir = bfs::path();
   }

   void database::wipe( const bfs::path& dir )
   {
      _segment.reset();
      _meta.reset();
      bfs::remove_all( dir / "shared_memory.bin" );
      bfs::remove_all( dir / "shared_memory.meta" );
      _data_dir = bfs::path();
      _index_list.clear();
      _index_map.clear();
   }

   void database::set_require_locking( bool enable_require_locking )
   {
#ifdef CHAINBASE_CHECK_LOCKING
      _enable_require_locking = enable_require_locking;
#endif
   }

#ifdef CHAINBASE_CHECK_LOCKING
   void database::require_lock_fail( const char* method, const char* lock_type, const char* tname )const
   {
      std::string err_msg = "database::" + std::string( method ) + " require_" + std::string( lock_type ) + "_lock() failed on type " + std::string( tname );
      std::cerr << err_msg << std::endl;
      BOOST_THROW_EXCEPTION( std::runtime_error( err_msg ) );
   }
#endif

   void database::undo()
   {
      for( auto& item : _index_list )
      {
         item->undo();
      }
   }

   void database::squash()
   {
      for( auto& item : _index_list )
      {
         item->squash();
      }
   }

   void database::commit( int64_t revision )
   {
      for( auto& item : _index_list )
      {
         item->commit( revision );
      }
   }

   void database::undo_all()
   {
      for( auto& item : _index_list )
      {
         item->undo_all();
      }
   }

   database::session database::start_undo_session( bool enabled )
   {
      if( enabled ) {
         vector< std::unique_ptr<abstract_session> > _sub_sessions;
         _sub_sessions.reserve( _index_list.size() );
         for( auto& item : _index_list ) {
            _sub_sessions.push_back( item->start_undo_session( enabled ) );
         }
         return session( std::move( _sub_sessions ) );
      } else {
         return session();
      }
   }

}  // namespace chainbase


