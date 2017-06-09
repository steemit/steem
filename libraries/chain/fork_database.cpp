#include <steemit/chain/fork_database.hpp>

#include <steemit/chain/database_exceptions.hpp>

namespace steemit { namespace chain {

fork_database::fork_database()
{
}
void fork_database::reset()
{
   _head.reset();
   _index.clear();
}

void fork_database::pop_block()
{
   FC_ASSERT( _head, "cannot pop an empty fork database" );
   auto prev = _head->prev.lock();
   FC_ASSERT( prev, "popping head block would leave fork DB empty" );
   _head = prev;
}

void     fork_database::start_block(signed_block b)
{
   auto item = std::make_shared<fork_item>(std::move(b));
   _index.insert(item);
   _head = item;
}

/**
 * Pushes the block into the fork database and caches it if it doesn't link
 *
 */
shared_ptr<fork_item>  fork_database::push_block(const signed_block& b)
{
   auto item = std::make_shared<fork_item>(b);
   try {
      _push_block(item);
   }
   catch ( const unlinkable_block_exception& e )
   {
      wlog( "Pushing block to fork database that failed to link: ${id}, ${num}", ("id",b.id())("num",b.block_num()) );
      wlog( "Head: ${num}, ${id}", ("num",_head->data.block_num())("id",_head->data.id()) );
      throw;
      _unlinked_index.insert( item );
   }
   return _head;
}

void  fork_database::_push_block(const item_ptr& item)
{
   if( _head ) // make sure the block is within the range that we are caching
   {
      FC_ASSERT( item->num > std::max<int64_t>( 0, int64_t(_head->num) - (_max_size) ),
                 "attempting to push a block that is too old",
                 ("item->num",item->num)("head",_head->num)("max_size",_max_size));
   }

   if( _head && item->previous_id() != block_id_type() )
   {
      auto& index = _index.get<block_id>();
      auto itr = index.find(item->previous_id());
      STEEMIT_ASSERT(itr != index.end(), unlinkable_block_exception, "block does not link to known chain");
      FC_ASSERT(!(*itr)->invalid);
      item->prev = *itr;
   }

   _index.insert(item);
   if( !_head || item->num > _head->num ) _head = item;
}

/**
 *  Iterate through the unlinked cache and insert anything that
 *  links to the newly inserted item.  This will start a recursive
 *  set of calls performing a depth-first insertion of pending blocks as
 *  _push_next(..) calls _push_block(...) which will in turn call _push_next
 */
void fork_database::_push_next( const item_ptr& new_item )
{
    auto& prev_idx = _unlinked_index.get<by_previous>();

    auto itr = prev_idx.find( new_item->id );
    while( itr != prev_idx.end() )
    {
       auto tmp = *itr;
       prev_idx.erase( itr );
       _push_block( tmp );

       itr = prev_idx.find( new_item->id );
    }
}

void fork_database::set_max_size( uint32_t s )
{
   _max_size = s;
   if( !_head ) return;

   { /// index
      auto& by_num_idx = _index.get<block_num>();
      auto itr = by_num_idx.begin();
      while( itr != by_num_idx.end() )
      {
         if( (*itr)->num < std::max(int64_t(0),int64_t(_head->num) - _max_size) )
            by_num_idx.erase(itr);
         else
            break;
         itr = by_num_idx.begin();
      }
   }
   { /// unlinked_index
      auto& by_num_idx = _unlinked_index.get<block_num>();
      auto itr = by_num_idx.begin();
      while( itr != by_num_idx.end() )
      {
         if( (*itr)->num < std::max(int64_t(0),int64_t(_head->num) - _max_size) )
            by_num_idx.erase(itr);
         else
            break;
         itr = by_num_idx.begin();
      }
   }
}

bool fork_database::is_known_block(const block_id_type& id)const
{
   auto& index = _index.get<block_id>();
   auto itr = index.find(id);
   if( itr != index.end() )
      return true;
   auto& unlinked_index = _unlinked_index.get<block_id>();
   auto unlinked_itr = unlinked_index.find(id);
   return unlinked_itr != unlinked_index.end();
}

item_ptr fork_database::fetch_block(const block_id_type& id)const
{
   auto& index = _index.get<block_id>();
   auto itr = index.find(id);
   if( itr != index.end() )
      return *itr;
   auto& unlinked_index = _unlinked_index.get<block_id>();
   auto unlinked_itr = unlinked_index.find(id);
   if( unlinked_itr != unlinked_index.end() )
      return *unlinked_itr;
   return item_ptr();
}

vector<item_ptr> fork_database::fetch_block_by_number(uint32_t num)const
{
   try
   {
   vector<item_ptr> result;
   auto itr = _index.get<block_num>().find(num);
   while( itr != _index.get<block_num>().end() )
   {
      if( (*itr)->num == num )
         result.push_back( *itr );
      else
         break;
      ++itr;
   }
   return result;
   }
   FC_LOG_AND_RETHROW()
}

pair<fork_database::branch_type,fork_database::branch_type>
  fork_database::fetch_branch_from(block_id_type first, block_id_type second)const
{ try {
   // This function gets a branch (i.e. vector<fork_item>) leading
   // back to the most recent common ancestor.
   pair<branch_type,branch_type> result;
   auto first_branch_itr = _index.get<block_id>().find(first);
   FC_ASSERT(first_branch_itr != _index.get<block_id>().end());
   auto first_branch = *first_branch_itr;

   auto second_branch_itr = _index.get<block_id>().find(second);
   FC_ASSERT(second_branch_itr != _index.get<block_id>().end());
   auto second_branch = *second_branch_itr;


   while( first_branch->data.block_num() > second_branch->data.block_num() )
   {
      result.first.push_back(first_branch);
      first_branch = first_branch->prev.lock();
      FC_ASSERT(first_branch);
   }
   while( second_branch->data.block_num() > first_branch->data.block_num() )
   {
      result.second.push_back( second_branch );
      second_branch = second_branch->prev.lock();
      FC_ASSERT(second_branch);
   }
   while( first_branch->data.previous != second_branch->data.previous )
   {
      result.first.push_back(first_branch);
      result.second.push_back(second_branch);
      first_branch = first_branch->prev.lock();
      FC_ASSERT(first_branch);
      second_branch = second_branch->prev.lock();
      FC_ASSERT(second_branch);
   }
   if( first_branch && second_branch )
   {
      result.first.push_back(first_branch);
      result.second.push_back(second_branch);
   }
   return result;
} FC_CAPTURE_AND_RETHROW( (first)(second) ) }

shared_ptr<fork_item> fork_database::walk_main_branch_to_num( uint32_t block_num )const
{
   shared_ptr<fork_item> next = head();
   if( block_num > next->num )
      return shared_ptr<fork_item>();

   while( next.get() != nullptr && next->num > block_num )
      next = next->prev.lock();
   return next;
}

shared_ptr<fork_item> fork_database::fetch_block_on_main_branch_by_number( uint32_t block_num )const
{
   vector<item_ptr> blocks = fetch_block_by_number(block_num);
   if( blocks.size() == 1 )
      return blocks[0];
   if( blocks.size() == 0 )
      return shared_ptr<fork_item>();
   return walk_main_branch_to_num(block_num);
}

void fork_database::set_head(shared_ptr<fork_item> h)
{
   _head = h;
}

void fork_database::remove(block_id_type id)
{
   _index.get<block_id>().erase(id);
}

} } // steemit::chain
