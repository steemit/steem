#pragma once

#include <steem/chain/database.hpp>
#include <fstream>

namespace steem { namespace chain {

   namespace u_types
   {
      enum op_type { create, modify, remove };
   };

   template< typename Object >
   class undo_scenario
   {
      private:

         chain::database& db;

         std::list< Object > old_values;

         template< typename CALL >
         const Object* run_impl( const Object* old_obj, u_types::op_type op, CALL call )
         {
            switch( op )
            {
               case u_types::create:
                  return &db.create< Object >( call );
               break;

               case u_types::modify:
                  assert( old_obj );
                  db.modify( *old_obj, call );
                  return old_obj;
               break;

               case u_types::remove:
                  assert( old_obj );
                  db.remove( *old_obj );
                  return nullptr;
               break;

               default:
                  assert( 0 && "Unknown operation" );
               break;
            }
            return nullptr;
         }

      protected:
      public:

         undo_scenario( chain::database& _db ): db( _db )
         {
         }

         virtual ~undo_scenario(){}
        
         template< typename CALL >
         const Object& create( CALL call )
         {
            try
            {
               return *run_impl( nullptr, u_types::create, call );
            }
            FC_LOG_AND_RETHROW()
         }

         template< typename CALL >
         const Object& modify( const Object& old_obj, CALL call )
         {
            try
            {
               return *run_impl( &old_obj, u_types::modify, call );
            }
            FC_LOG_AND_RETHROW()
         }

         const void remove( const Object& old_obj )
         {
            try
            {
               static std::function< void( Object& ) > empty;
               run_impl( &old_obj, u_types::remove, empty );
            }
            FC_LOG_AND_RETHROW()
         }

         template< typename Index >
         void remember_old_values()
         {
            old_values.clear();

            const auto& idx = db.get_index< Index >().indices().get< by_id >();
            auto it = idx.begin();

            int32_t cnt = 0;
            while( it != idx.end() )
            {
               old_values.emplace_back( *( it++ ) );
               ++cnt;
            }
         }

         template< typename Index >
         uint32_t size()
         {
            const auto& idx = db.get_index< Index >().indices().get< by_id >();
            return idx.size();
         }

         template< typename Index >
         bool check()
         {
            try
            {
               const auto& idx = db.get_index< Index >().indices().get< by_id >();

               uint32_t idx_size = idx.size();
               uint32_t old_size = old_values.size();
               if( idx_size != old_size )
                  return false;

               auto it = idx.begin();
               auto it_end = idx.end();

               auto it_old = old_values.begin();

               while( it != it_end )
               {
                  const Object& actual = *it;
                  const Object& old = *it_old;
                  if( actual.id != old.id )
                     return false;

                  ++it;
                  ++it_old;
               }
            }
            FC_LOG_AND_RETHROW()

            return true;
         }

   };

   class undo_db
   {
      private:

         database::session* session = nullptr;
         chain::database& db;

         void undo_session()
         {
            try
            {
               if( session )
                  session->undo();

               if( session )
               {
                  delete session;
                  session = nullptr;
               }
            }
            FC_LOG_AND_RETHROW()
         }

      protected:
      public:
      
         undo_db( chain::database& _db ): db( _db )
         {
         }

         void undo_start()
         {
            if( session )
            {
               delete session;
               session = nullptr;
            }

            session = new database::session( db.start_undo_session( true ) );
         }

         void undo_end()
         {
            if( session )
               session->undo();
         }
   };

} }
