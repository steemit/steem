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
#include <graphene/db/object_database.hpp>
#include <graphene/db/undo_database.hpp>
#include <fc/reflect/variant.hpp>

namespace graphene { namespace db {

void undo_database::enable()  { _disabled = false; }
void undo_database::disable() { _disabled = true; }

undo_database::session undo_database::start_undo_session( bool force_enable )
{
   if( _disabled && !force_enable ) return session(*this);
   bool disable_on_exit = _disabled  && force_enable;
   if( force_enable ) 
      _disabled = false;

   while( size() > max_size() )
      _stack.pop_front();

   _stack.emplace_back();
   ++_active_sessions;
   return session(*this, disable_on_exit );
}
void undo_database::on_create( const object& obj )
{
   if( _disabled ) return;

   if( _stack.empty() )
      _stack.emplace_back();
   auto& state = _stack.back();
   auto index_id = object_id_type( obj.id.space(), obj.id.type(), 0 );
   auto itr = state.old_index_next_ids.find( index_id );
   if( itr == state.old_index_next_ids.end() )
      state.old_index_next_ids[index_id] = obj.id;
   state.new_ids.insert(obj.id);
}
void undo_database::on_modify( const object& obj )
{
   if( _disabled ) return;

   if( _stack.empty() )
      _stack.emplace_back();
   auto& state = _stack.back();
   if( state.new_ids.find(obj.id) != state.new_ids.end() )
      return;
   auto itr =  state.old_values.find(obj.id);
   if( itr != state.old_values.end() ) return;
   state.old_values[obj.id] = obj.clone();
}
void undo_database::on_remove( const object& obj )
{
   if( _disabled ) return;

   if( _stack.empty() )
      _stack.emplace_back();
   undo_state& state = _stack.back();
   if( state.new_ids.count(obj.id) )
   {
      state.new_ids.erase(obj.id);
      return;
   }
   if( state.old_values.count(obj.id) )
   {
      state.removed[obj.id] = std::move(state.old_values[obj.id]);
      state.old_values.erase(obj.id);
      return;
   }
   if( state.removed.count(obj.id) ) return;
   state.removed[obj.id] = obj.clone();
}

void undo_database::undo()
{ try {
   FC_ASSERT( !_disabled );
   FC_ASSERT( _active_sessions > 0 );
   disable();

   auto& state = _stack.back();
   for( auto& item : state.old_values )
   {
      _db.modify( _db.get_object( item.second->id ), [&]( object& obj ){ obj.move_from( *item.second ); } );
   }

   for( auto ritr = state.new_ids.begin(); ritr != state.new_ids.end(); ++ritr  )
   {
      _db.remove( _db.get_object(*ritr) );
   }

   for( auto& item : state.old_index_next_ids )
   {
      _db.get_mutable_index( item.first.space(), item.first.type() ).set_next_id( item.second );
   }

   for( auto& item : state.removed )
      _db.insert( std::move(*item.second) );

   _stack.pop_back();
   if( _stack.empty() )
      _stack.emplace_back();
   enable();
   --_active_sessions;
} FC_CAPTURE_AND_RETHROW() }

void undo_database::merge()
{
   FC_ASSERT( _active_sessions > 0 );
   FC_ASSERT( _stack.size() >=2 );
   auto& state = _stack.back();
   auto& prev_state = _stack[_stack.size()-2];

   // An object's relationship to a state can be:
   // in new_ids            : new
   // in old_values (was=X) : upd(was=X)
   // in removed (was=X)    : del(was=X)
   // not in any of above   : nop
   //
   // When merging A=prev_state and B=state we have a 4x4 matrix of all possibilities:
   //
   //                   |--------------------- B ----------------------|
   //
   //                +------------+------------+------------+------------+
   //                | new        | upd(was=Y) | del(was=Y) | nop        |
   //   +------------+------------+------------+------------+------------+
   // / | new        | N/A        | new       A| nop       C| new       A|
   // | +------------+------------+------------+------------+------------+
   // | | upd(was=X) | N/A        | upd(was=X)A| del(was=X)C| upd(was=X)A|
   // A +------------+------------+------------+------------+------------+
   // | | del(was=X) | N/A        | N/A        | N/A        | del(was=X)A|
   // | +------------+------------+------------+------------+------------+
   // \ | nop        | new       B| upd(was=Y)B| del(was=Y)B| nop      AB|
   //   +------------+------------+------------+------------+------------+
   //
   // Each entry was composed by labelling what should occur in the given case.
   //
   // Type A means the composition of states contains the same entry as the first of the two merged states for that object.
   // Type B means the composition of states contains the same entry as the second of the two merged states for that object.
   // Type C means the composition of states contains an entry different from either of the merged states for that object.
   // Type N/A means the composition of states violates causal timing.
   // Type AB means both type A and type B simultaneously.
   //
   // The merge() operation is defined as modifying prev_state in-place to be the state object which represents the composition of
   // state A and B.
   //
   // Type A (and AB) can be implemented as a no-op; prev_state already contains the correct value for the merged state.
   // Type B (and AB) can be implemented by copying from state to prev_state.
   // Type C needs special case-by-case logic.
   // Type N/A can be ignored or assert(false) as it can only occur if prev_state and state have illegal values
   // (a serious logic error which should never happen).
   //

   // We can only be outside type A/AB (the nop path) if B is not nop, so it suffices to iterate through B's three containers.

   // *+upd
   for( auto& obj : state.old_values )
   {
      if( prev_state.new_ids.find(obj.second->id) != prev_state.new_ids.end() )
      {
         // new+upd -> new, type A
         continue;
      }
      if( prev_state.old_values.find(obj.second->id) != prev_state.old_values.end() )
      {
         // upd(was=X) + upd(was=Y) -> upd(was=X), type A
         continue;
      }
      // del+upd -> N/A
      assert( prev_state.removed.find(obj.second->id) == prev_state.removed.end() );
      // nop+upd(was=Y) -> upd(was=Y), type B
      prev_state.old_values[obj.second->id] = std::move(obj.second);
   }

   // *+new, but we assume the N/A cases don't happen, leaving type B nop+new -> new
   for( auto id : state.new_ids )
      prev_state.new_ids.insert(id);

   // old_index_next_ids can only be updated, iterate over *+upd cases
   for( auto& item : state.old_index_next_ids )
   {
      if( prev_state.old_index_next_ids.find( item.first ) == prev_state.old_index_next_ids.end() )
      {
         // nop+upd(was=Y) -> upd(was=Y), type B
         prev_state.old_index_next_ids[item.first] = item.second;
         continue;
      }
      else
      {
         // upd(was=X)+upd(was=Y) -> upd(was=X), type A
         // type A implementation is a no-op, as discussed above, so there is no code here
         continue;
      }
   }

   // *+del
   for( auto& obj : state.removed )
   {
      if( prev_state.new_ids.find(obj.second->id) != prev_state.new_ids.end() )
      {
         // new + del -> nop (type C)
         prev_state.new_ids.erase(obj.second->id);
         continue;
      }
      auto it = prev_state.old_values.find(obj.second->id);
      if( it != prev_state.old_values.end() )
      {
         // upd(was=X) + del(was=Y) -> del(was=X)
         prev_state.removed[obj.second->id] = std::move(it->second);
         prev_state.old_values.erase(obj.second->id);
         continue;
      }
      // del + del -> N/A
      assert( prev_state.removed.find( obj.second->id ) == prev_state.removed.end() );
      // nop + del(was=Y) -> del(was=Y)
      prev_state.removed[obj.second->id] = std::move(obj.second);
   }
   _stack.pop_back();
   --_active_sessions;
}
void undo_database::commit()
{
   FC_ASSERT( _active_sessions > 0 );
   --_active_sessions;
}

void undo_database::pop_commit()
{
   FC_ASSERT( _active_sessions == 0 );
   FC_ASSERT( !_stack.empty() );

   disable();
   try {
      auto& state = _stack.back();

      for( auto& item : state.old_values )
      {
         _db.modify( _db.get_object( item.second->id ), [&]( object& obj ){ obj.move_from( *item.second ); } );
      }

      for( auto ritr = state.new_ids.begin(); ritr != state.new_ids.end(); ++ritr  )
      {
         _db.remove( _db.get_object(*ritr) );
      }

      for( auto& item : state.old_index_next_ids )
      {
         _db.get_mutable_index( item.first.space(), item.first.type() ).set_next_id( item.second );
      }

      for( auto& item : state.removed )
         _db.insert( std::move(*item.second) );

      _stack.pop_back();
   }
   catch ( const fc::exception& e )
   {
      elog( "error popping commit ${e}", ("e", e.to_detail_string() )  );
      enable();
      throw;
   }
   enable();
}
const undo_state& undo_database::head()const
{
   FC_ASSERT( !_stack.empty() );
   return _stack.back();
}

} } // graphene::db
