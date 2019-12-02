#include <steem/chain/steem_fwd.hpp>
#include <steem/chain/database.hpp>
#include <steem/protocol/asset_symbol.hpp>
#include <steem/chain/smt_objects.hpp>
#include <steem/chain/util/nai_generator.hpp>
#include <steem/chain/util/smt_token.hpp>

#define NAI_GENERATION_SEED_BLOCK_ID_HASH_INDEX 4

namespace steem { namespace chain {

/**
 * Refill the NAI pool with newly generated values
 *
 * We will attempt to fill the pool with randomly generated NAIs up until
 * the pool is full (SMT_MAX_NAI_POOL_COUNT) or we encounter the maximum
 * acceptable collisions (SMT_MAX_NAI_GENERATION_TRIES). If we hit the
 * maximum acceptable collisions, the pool will once again attempt to
 * replenish when called during the next block.
 */
void replenish_nai_pool( database& db )
{
   try
   {
      const nai_pool_object& npo = db.get< nai_pool_object >();
      auto head_block_id = db.head_block_id();

      // If this is the first time we're encountering this block, reset our variables and set the last block */
      if ( npo.last_block_id != head_block_id )
      {
         db.modify( npo, [ head_block_id ]( nai_pool_object& p )
         {
            p.last_block_id = head_block_id;
            p.collisions_per_block = 0;
            p.attempts_per_block = 0;
         });
      }

      /*
       * No reason to attempt NAI generation or inform the user again if we have already
       * reached the maximum acceptable collisions for this particular block.
       */
      if ( npo.collisions_per_block >= SMT_MAX_NAI_GENERATION_TRIES )
         return;

      while ( npo.num_available_nais < SMT_MAX_NAI_POOL_COUNT )
      {
         asset_symbol_type next_sym;
         for (;;)
         {
            if ( npo.collisions_per_block >= SMT_MAX_NAI_GENERATION_TRIES )
            {
               ilog( "Encountered ${collisions} collisions while attempting to generate NAI, generation will resume at the next block",
                  ("collisions", npo.collisions_per_block) );
               return;
            }

            next_sym = util::nai_generator::generate( npo.last_block_id._hash[ NAI_GENERATION_SEED_BLOCK_ID_HASH_INDEX ] + npo.attempts_per_block );

            db.modify( npo, []( nai_pool_object& p )
            {
               p.attempts_per_block++;
            });

            // We must ensure the NAI is not an SMT, and it is not already contained within the NAI pool
            if ( !util::smt::find_token( db, next_sym, true ) && !npo.contains( next_sym ) )
               break;

            db.modify( npo, []( nai_pool_object& p )
            {
               p.collisions_per_block++;
            });
         }

         db.modify( npo, [&]( nai_pool_object& obj )
         {
            obj.nais[ obj.num_available_nais ] = next_sym;
            obj.num_available_nais++;
         });
      }
   }
   FC_CAPTURE_AND_RETHROW()
}

void remove_from_nai_pool( database &db, const asset_symbol_type& a )
{
   const nai_pool_object& npo = db.get< nai_pool_object >();
   const auto& nais = npo.nais;
   const auto end = nais.begin() + npo.num_available_nais;
   auto it = std::find( nais.begin(), end, asset_symbol_type::from_asset_num( a.get_stripped_precision_smt_num() ) );
   if ( it != end )
   {
      auto index = std::distance( nais.begin(), it );
      db.modify( npo, [&] ( nai_pool_object& obj )
      {
         obj.nais[ index ] = asset_symbol_type();
         std::swap( obj.nais[ index ], obj.nais[ obj.num_available_nais - 1 ] );
         obj.num_available_nais--;
      });
   }
}

} } // steem::chain

