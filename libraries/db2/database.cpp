#include <graphene/db2/database.hpp>
#include <fc/filesystem.hpp>

namespace graphene { namespace db2 {

   void database::open( const bfs::path& file )
   {
      if( _data_dir != file ) close();

      fc::create_directories( file );

      _data_dir = file;
      auto abs_path = bfs::absolute( file / "shared_memory" );
      _segment.reset( new bip::managed_mapped_file( bip::open_or_create,
                                                    abs_path.generic_string().c_str(),
                                                    uint64_t(1024*1024*64) ) );
      _mutex = _segment->find_or_construct< bip::interprocess_mutex >( "global_mutex" )();
   }

   void database::flush()
   {
      if( _segment )
         _segment->flush();
   }

   void database::close()
   {
      _segment.reset();
      _data_dir = bfs::path();
   }

   void database::wipe()
   {
      ilog( "Attempting to wipe database..." );
      elog( "wipe not implemented" );
   }

   void database::reset_indexes()
   {
      ilog( "Attempting to reset indexes..." );
      elog( "reset_indexes not implemented" );
   }

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
   void database::commit( uint64_t revision )
   {
      for( auto& item : _index_list )
      {
         item->commit( revision );
      }
   }

   database::session database::start_undo_session( bool enabled )
   {
      if( enabled )
      {
         vector< std::unique_ptr<abstract_session> > _sub_sessions;
         _sub_sessions.reserve( _index_map.size() );
         for( auto& item : _index_list )
         {
            _sub_sessions.push_back( item->start_undo_session( enabled ) );
         }
         return session( std::move( _sub_sessions ) );
      }
      else
      {
         return session();
      }
   }

} } // graphene::db2
