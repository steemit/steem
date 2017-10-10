
#include <steem/protocol/nai_data.hpp>

#include <steem/chain/block_summary_object.hpp>

#include <steem/chain/smt_objects/smt_allowed_symbol_object.hpp>
#include <steem/chain/smt_objects/smt_token_object.hpp>

#include <steem/chain/database.hpp>

#ifdef STEEM_ENABLE_SMT

#define SMT_SHIFT_REGISTER_LOOKBACK 128
#define SMT_ALLOWED_SYM_BITS        27
#define SMT_ALLOWED_SYM_BACKLOG     8

static_assert( SMT_MAX_NAI < (1 << SMT_ALLOWED_SYM_BITS), "Must redefine SMT_ALLOWED_SYM_BITS" );

namespace steem { namespace chain {

void update_smt_allowed_symbols( database& db )
{
   // Maintain a list of symbols which may be used to register a new SMT.  To add a new symbol
   // to the list, take the low bit of the block ID of the next 128 blocks, append <n>
   // and execute sha256.  This way, an attacking witness who produces K consecutive blocks
   // can only choose among 2^K possibilities for the SMT to be inserted.

   const auto& qidx = db.get_index<smt_allowed_symbol_qitem_index>().indices().get< by_id >();
   const auto& sidx = db.get_index<smt_allowed_symbol_index>().indices().get< by_symbol >();
   const auto& tidx = db.get_index<smt_token_index>().indices().get< by_symbol >();
   const dynamic_global_property_object& dgpo = db.get_dynamic_global_properties();
   uint32_t head_block_num = dgpo.head_block_number;
   block_id_type head_block_id = dgpo.head_block_id;
   vector<char> shift_reg;
   auto can_add_nai_data = [&]( uint32_t nai_data_digits ) -> bool
   {
      if( (nai_data_digits < SMT_MIN_NAI) || (nai_data_digits > SMT_MAX_NAI) )
         return false;
      // when the symbol is represented by an allowed_symbol_object, the allowed_symbol_object
      // will have 0 decimal places so we can simply check using find()
      asset_symbol_type sym = asset_symbol_from_nai_data( nai_data_digits, 0 );
      if( sidx.find( sym ) != sidx.end() )
         return false;
      // when the symbol generated here collides with an smt_token_object,
      // we will start walking the container at that point, if the first element encountered
      // has the same nai_data then we consider that a collision
      auto it = tidx.lower_bound( sym );
      if( (it != tidx.end()) && (asset_symbol_to_nai_data(it->symbol) == nai_data_digits) )
         return false;
      return true;
   };

   // First, transition qitems for the current block
   while( true )
   {
      auto it = qidx.begin();
      if( it == qidx.end() )
         break;
      if( it->block_num != head_block_num )
      {
         // Since qitems with block_num == head_block_num are converted each block,
         //    we have an invariant that qitems should always be >= head_block_num
         FC_ASSERT( it->block_num > head_block_num, "Illegal smt_allowed_symbols queue state" );
         break;
      }
      if( shift_reg.size() == 0 )
      {
         shift_reg.resize( (SMT_SHIFT_REGISTER_LOOKBACK+7) >> 3 );
         uint16_t lowbits = uint16_t(head_block_num);

         for( int i=0; i<SMT_SHIFT_REGISTER_LOOKBACK; i++ )
         {
            // Take one bit of entropy from the last blocks
            const block_summary_object& bso = db.get< block_summary_object >( lowbits );
            uint8_t last_byte = uint8_t( bso.block_id.data()[ bso.block_id.data_size()-1 ] );
            uint8_t last_bit = last_byte&1;
            ilog( "block_id=${bid}   lastbyte=${lbyte}   lastbit=${lbit}", ("bid", bso.block_id)("lbyte", last_byte)("lbit", last_bit) );
            shift_reg[i >> 3] |= last_bit << (i&7);
            --lowbits;     // overflow is OK here
         }
      }
      uint32_t index_in_block = it->index_in_block;
      fc::sha256::encoder enc;
      enc.write( shift_reg.data(), shift_reg.size() );
      enc.write( (char*) &index_in_block, sizeof( uint32_t ) );
      fc::sha256 s = enc.result();

      uint32_t new_asset_num = 0;
      for( int i=0; i<256/SMT_ALLOWED_SYM_BITS; i++ )
      {
         uint32_t lo = uint32_t(s._hash[0] & ((1 << SMT_ALLOWED_SYM_BITS)-1));
         ilog( "s=${s}   h0=${h0}   lo=${lo}", ("s", s)("h0", s._hash[0])("lo", lo) );
         if( can_add_nai_data( lo ) )
         {
            new_asset_num = lo;
            break;
         }
         else
            s = s >> SMT_ALLOWED_SYM_BITS;
      }
      if( new_asset_num != 0 )
      {
         db.create< smt_allowed_symbol_object >( [&]( smt_allowed_symbol_object& asym )
         {
            asym.symbol = asset_symbol_type::from_asset_num( new_asset_num );
         } );
      }
      db.remove( *it );
   }

   // If there aren't enough, then create them
   uint64_t num_syms   = uint64_t( db.get_index<smt_allowed_symbol_index>().indices().size() );
   uint64_t num_qitems = uint64_t( db.get_index<smt_allowed_symbol_qitem_index>().indices().size() );

   uint32_t index_in_block = 0;
   while( num_syms + num_qitems < SMT_ALLOWED_SYM_BACKLOG )
   {
      db.create< smt_allowed_symbol_qitem_object >( [&]( smt_allowed_symbol_qitem_object& qitem )
      {
         qitem.start_block_id = head_block_id;
         qitem.block_num = head_block_num + SMT_SHIFT_REGISTER_LOOKBACK;
         qitem.index_in_block = index_in_block;
      } );
      ++index_in_block;
      ++num_qitems;
   }
}

} }

#endif
