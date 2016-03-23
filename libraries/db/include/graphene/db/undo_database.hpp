/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <graphene/db/object.hpp>
#include <deque>
#include <fc/exception/exception.hpp>

namespace graphene { namespace db {

   using std::unordered_map;
   using fc::flat_set;
   class object_database;

   struct undo_state
   {
      unordered_map<object_id_type, unique_ptr<object> > old_values;
      unordered_map<object_id_type, object_id_type>      old_index_next_ids;
      std::unordered_set<object_id_type>                 new_ids;
      unordered_map<object_id_type, unique_ptr<object> > removed;
   };


   /**
    * @class undo_database
    * @brief tracks changes to the state and allows changes to be undone
    *
    */
   class undo_database
   {
      public:
         undo_database( object_database& db ):_db(db){}

         class session
         {
            public:
               session( session&& mv )
               :_db(mv._db),_apply_undo(mv._apply_undo)
               {
                  mv._apply_undo = false;
               }
               ~session() {
                  try {
                     if( _apply_undo ) _db.undo();
                  }
                  catch ( const fc::exception& e )
                  {
                     elog( "${e}", ("e",e.to_detail_string() ) );
                     throw; // maybe crash..
                  }
                  if( _disable_on_exit ) _db.disable();
               }
               void commit() { _apply_undo = false; _db.commit();  }
               void undo()   { if( _apply_undo ) _db.undo(); _apply_undo = false; }
               void merge()  { if( _apply_undo ) _db.merge(); _apply_undo = false; }

               session& operator = ( session&& mv )
               { try {
                  if( this == &mv ) return *this;
                  if( _apply_undo ) _db.undo();
                  _apply_undo = mv._apply_undo;
                  mv._apply_undo = false;
                  return *this;
               } FC_CAPTURE_AND_RETHROW() }

            private:
               friend class undo_database;
               session(undo_database& db, bool disable_on_exit = false): _db(db),_disable_on_exit(disable_on_exit) {}
               undo_database& _db;
               bool _apply_undo = true;
               bool _disable_on_exit = false;
         };

         void    disable();
         void    enable();
         bool    enabled()const { return !_disabled; }

         session start_undo_session( bool force_enable = false );
         /**
          * This should be called just after an object is created
          */
         void on_create( const object& obj );
         /**
          * This should be called just before an object is modified
          *
          * If it's a new object as of this undo state, its pre-modification value is not stored, because prior to this
          * undo state, it did not exist. Any modifications in this undo state are irrelevant, as the object will simply
          * be removed if we undo.
          */
         void on_modify( const object& obj );
         /**
          * This should be called just before an object is removed.
          *
          * If it's a new object as of this undo state, its pre-removal value is not stored, because prior to this undo
          * state, it did not exist. Now that it's been removed, it doesn't exist again, so nothing has happened.
          * Instead, remove it from the list of newly created objects (which must be deleted if we undo), as we don't
          * want to re-delete it if this state is undone.
          */
         void on_remove( const object& obj );

         /**
          *  Removes the last committed session,
          *  note... this is dangerous if there are
          *  active sessions... thus active sessions should
          *  track
          */
         void pop_commit();

         std::size_t size()const { return _stack.size(); }
         void set_max_size(size_t new_max_size) { _max_size = new_max_size; }
         size_t max_size()const { return _max_size; }

         const undo_state& head()const;

      private:
         void undo();
         void merge();
         void commit();

         uint32_t                _active_sessions = 0;
         bool                    _disabled = true;
         std::deque<undo_state>  _stack;
         object_database&        _db;
         size_t                  _max_size = 256;
   };

} } // graphene::db
