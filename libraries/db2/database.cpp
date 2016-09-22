#include <graphene/db2/database.hpp>

namespace graphene { namespace db2 {

   void database::open( const bfs::path& file ) {
      auto abs_path = bfs::absolute( file );
      _segment.reset( new bip::managed_mapped_file( bip::open_or_create, 
                                                    abs_path.generic_string().c_str(), 
                                                    1024*1024*1024*64 ) );
      _mutex = _segment->find_or_construct< bip::interprocess_mutex >( "global_mutex" )();
   }


   database::session database::start_undo_session( bool enabled ) {
      if( enabled ) {
         vector< std::unique_ptr<abstract_session> > _sub_sessions;
         _sub_sessions.reserve( _index_map.size() );
         for( auto& item : _index_list ) {
            _sub_sessions.push_back( item->start_undo_session( enabled ) );
         }
         return session( std::move( _sub_sessions ) );
      } else {
         return session();
      }
   }


} }


