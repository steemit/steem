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

   void database::open( const bfs::path& dir, uint32_t flags, size_t shared_file_size )
   {
      bfs::create_directories( dir );
      if( _data_dir != dir ) close();

      _data_dir = dir;

#ifndef ENABLE_STD_ALLOCATOR
      auto abs_path = bfs::absolute( dir / "shared_memory.bin" );

      if( bfs::exists( abs_path ) )
      {
         _file_size = bfs::file_size( abs_path );
         if( shared_file_size > _file_size )
         {
            if( !bip::managed_mapped_file::grow( abs_path.generic_string().c_str(), shared_file_size - _file_size ) )
               BOOST_THROW_EXCEPTION( std::runtime_error( "could not grow database file to requested size." ) );

            _file_size = shared_file_size;
         }

         _segment.reset( new bip::managed_mapped_file( bip::open_only,
                                                       abs_path.generic_string().c_str()
                                                       ) );

         auto env = _segment->find< environment_check >( "environment" );
         if( !env.first || !( *env.first == environment_check()) ) {
            BOOST_THROW_EXCEPTION( std::runtime_error( "database created by a different compiler, build, or operating system" ) );
         }
      } else {
         _file_size = shared_file_size;
         _segment.reset( new bip::managed_mapped_file( bip::create_only,
                                                       abs_path.generic_string().c_str(), shared_file_size
                                                       ) );
         _segment->find_or_construct< environment_check >( "environment" )();
      }

      _flock = bip::file_lock( abs_path.generic_string().c_str() );
      if( !_flock.try_lock() )
         BOOST_THROW_EXCEPTION( std::runtime_error( "could not gain write access to the shared memory file" ) );
#endif
   }

   void database::flush() {
#ifndef ENABLE_STD_ALLOCATOR
      if( _segment )
         _segment->flush();
      if( _meta )
         _meta->flush();
#endif
   }

   void database::close()
   {
#ifndef ENABLE_STD_ALLOCATOR
      _segment.reset();
      _meta.reset();
      _data_dir = bfs::path();
#endif
   }

   void database::wipe( const bfs::path& dir )
   {
#ifndef ENABLE_STD_ALLOCATOR
      _segment.reset();
      _meta.reset();
      bfs::remove_all( dir / "shared_memory.bin" );
      bfs::remove_all( dir / "shared_memory.meta" );
      _data_dir = bfs::path();
#endif
      _index_list.clear();
      _index_map.clear();
   }

   void database::resize( size_t new_shared_file_size )
   {
      if( _undo_session_count )
         BOOST_THROW_EXCEPTION( std::runtime_error( "Cannot resize shared memory file while undo session is active" ) );

      _segment.reset();
      _meta.reset();

      open( _data_dir, 0, new_shared_file_size );

      _index_list.clear();
      _index_map.clear();

      for( auto& index_type : _index_types )
      {
         index_type->add_index( *this );
      }
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

   database::session database::start_undo_session()
   {
      vector< std::unique_ptr<abstract_session> > _sub_sessions;
      _sub_sessions.reserve( _index_list.size() );
      for( auto& item : _index_list ) {
         _sub_sessions.push_back( item->start_undo_session() );
      }
      return session( std::move( _sub_sessions ), _undo_session_count );
   }

}  // namespace chainbase


