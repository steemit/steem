#pragma once

#include <steem/chain/database.hpp>
#include <fstream>

namespace steem { namespace chain {

   namespace u_types
   {
      enum op_type { create, modify, remove };
   };

   /*
      The class 'undo_scenario' simplifies writing tests from 'undo_tests' group.
      a) There are implemented wrappers for 3 methods: database::create, database::modify, database::remove.
      b) Methods 'remember_old_values' and 'check' help to compare state before and after running 'undo' mechanism.
   */
   template< typename Object >
   class undo_scenario
   {
      private:

         chain::database& db;

         std::list< Object > old_values;

         //Calls proper method( create, modify, remove ) for given database object.
         template< typename CALL >
         const Object* run_impl( const Object* old_obj, u_types::op_type op, CALL call )
         {
            switch( op )
            {
               case u_types::create:
                  assert( !old_obj );
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

         //Proxy method for `database::create`.
         template< typename CALL >
         const Object& create( CALL call )
         {
            return *run_impl( nullptr, u_types::create, call );
         }

         //Proxy method for `database::modify`.
         template< typename CALL >
         const Object& modify( const Object& old_obj, CALL call )
         {
            return *run_impl( &old_obj, u_types::modify, call );
         }

         //Proxy method for `database::remove`.
         const void remove( const Object& old_obj )
         {
            static std::function< void( Object& ) > empty;
            run_impl( &old_obj, u_types::remove, empty );
         }

         //Save old objects before launching 'undo' mechanism.
         //The objects are always sorted using 'by_id' index.
         template< typename Index >
         void remember_old_values()
         {
            old_values.clear();

            const auto& idx = db.get_index< Index, by_id >();
            auto it = idx.begin();

            while( it != idx.end() )
               old_values.emplace_back( *( it++ ) );
         }

         //Get size of given index.
         template< typename Index >
         uint32_t size()
         {
            const auto& idx = db.get_index< Index, by_id >();
            return idx.size();
         }

         //Reads current objects( for given index ) and compares with objects which have been saved before.
         //The comparision is according to 'id' field.
         template< typename Index >
         bool check()
         {
            try
            {
               const auto& idx = db.get_index< Index, by_id >();

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

   /*
      The class 'undo_scenario' simplifies writing tests from 'undo_tests' group.
      A method 'undo_begin' allows to enable 'undo' mechanism artificially.
      A method 'undo_end' allows to revert all changes.
   */
   class undo_db
   {
      private:

         database::session* session = nullptr;
         chain::database& db;

      protected:
      public:

         undo_db( chain::database& _db ): db( _db )
         {
         }

         //Enable 'undo' mechanism.
         void undo_begin()
         {
            if( session )
            {
               delete session;
               session = nullptr;
            }

            session = new database::session( db.start_undo_session() );
         }

         //Revert all changes.
         void undo_end()
         {
            if( session )
               session->undo();
         }
   };

} }
