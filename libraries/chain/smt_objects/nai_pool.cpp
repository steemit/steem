#include <steem/chain/database.hpp>
#include <steem/protocol/asset_symbol.hpp>
#include <steem/chain/smt_objects.hpp>
#include <steem/chain/util/nai_generator.hpp>

#ifdef STEEM_ENABLE_SMT

#define NAI_GENERATION_SEED_BLOCK_ID_HASH_INDEX 4

namespace steem { namespace chain {

/**
 * Refill the NAI pool with newly generated values
 *
 * We will attempt to fill the pool with randomly generated NAIs up until
 * the pool is full (SMT_MAX_NAI_POOL_COUNT) or we encounter the maximum
 * acceptable collisions (SMT_MAX_NAI_GENERATION_TRIES). It will once again
 * replenish when this function is called and the database has a new head
 * block ID.
 */
void replenish_nai_pool( database& db )
{
   try
   {
      static block_id_type last_block_id = db.head_block_id();
      static uint32_t collisions_per_block = 0;
      static uint32_t attempts_per_block = 0;

      // If this is the first time we're encountering this block, reset our variables and set the last block */
      if ( last_block_id != db.head_block_id() )
      {
         last_block_id = db.head_block_id();
         collisions_per_block = 0;
         attempts_per_block = 0;
      }

      /*
       * No reason to attempt NAI generation or inform the user again if we have already
       * reached the maximum acceptable collisions for this particular block.
       */
      if ( collisions_per_block >= SMT_MAX_NAI_GENERATION_TRIES )
         return;

      const nai_pool_object& npo = db.get< nai_pool_object >();
      while ( npo.nai_pool.size() < SMT_MAX_NAI_POOL_COUNT )
      {
         asset_symbol_type next_sym;
         for (;;)
         {
            if ( collisions_per_block >= SMT_MAX_NAI_GENERATION_TRIES )
            {
               ilog( "Encountered ${collisions} collisions while attempting to generate NAI, generation will resume at the next block",
                  ("collisions", collisions_per_block) );
               return;
            }

            next_sym = nai_generator::generate( last_block_id._hash[ NAI_GENERATION_SEED_BLOCK_ID_HASH_INDEX ] + attempts_per_block );

            attempts_per_block++;

            // We must ensure the NAI is not an SMT, and it is not already contained within the NAI pool
            if ( db.find< smt_token_object, by_symbol >( next_sym.to_nai() ) == nullptr && !npo.contains( next_sym ) )
               break;

            collisions_per_block++;
         }

         db.modify( npo, [&]( nai_pool_object& obj )
         {
            obj.nai_pool.push_back( next_sym );
         } );
      }
   }
   FC_CAPTURE_AND_RETHROW()
}

void remove_from_nai_pool( database &db, const asset_symbol_type& a )
{
   const nai_pool_object& npo = db.get< nai_pool_object >();
   const auto& nai_pool = npo.nai_pool;
   auto it = std::find( nai_pool.begin(), nai_pool.end(), asset_symbol_type::from_asset_num( a.get_stripped_precision_smt_num() ) );
   if ( it != nai_pool.end() )
   {
      db.modify( npo, [&] ( nai_pool_object& obj )
      {
         obj.nai_pool.erase( it );
      } );
   }
}

} } // steem::chain

#endif
