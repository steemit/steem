#include <fc/interprocess/container.hpp>
#include <steemit/chain2/block_objects.hpp>
#include <steemit/chain2/chain_database.hpp>
#include <fc/io/raw.hpp>

namespace steemit { namespace chain2 {
   database::database()
   {
   }
   database::~database() 
   {
   }

   void database::open( const fc::path& dir ) {
      graphene::db2::database::open( dir );

      add_index<block_index>();
//      add_index<transaction_index>();
   }

   const block_object& database::head_block()const {
      const auto& block_idx = get_index<block_index, by_block_num>();
      auto head_block_itr = block_idx.rbegin();
      FC_ASSERT( head_block_itr != block_idx.rend() );
      return *head_block_itr;
   }
   uint32_t  database::head_block_num()const {
      const auto& block_idx = get_index<block_index, by_block_num>();
      auto head_block_itr = block_idx.rbegin();
      if( head_block_itr != block_idx.rend() ) 
         return head_block_itr->block_num;
      return 0;
   }


   void apply( database& db, const signed_block& b, const options_type& opts ) {
      auto undo_session = db.start_undo_session( !(opts & skip_undo_block) );
         db.pre_apply_block( b );

         if( !(opts & skip_validation) ) {
            FC_ASSERT( b.timestamp.sec_since_epoch() % 3 == 0 );
            if( b.block_num() > 1 ) {
               idump((b.block_num())); 
               const auto& head = db.head_block();
               FC_ASSERT( b.block_num() == head.block_num + 1 );
               FC_ASSERT( b.timestamp >= head.timestamp + fc::seconds(3) );
            }
         }

         db.create<block_object>( [&]( block_object& obj ) {
             obj.block_num               = b.block_num();
             obj.block_id                = b.id();
             obj.ref_prefix              = obj.block_id._hash[1];
             obj.previous                = b.previous;
             obj.timestamp               = b.timestamp;
             obj.witness                 = b.witness;
             obj.transaction_merkle_root = b.transaction_merkle_root;
             obj.witness_signature       = b.witness_signature;

             obj.transactions.reserve( b.transactions.size() );
             for( const auto& t : b.transactions ) {
                obj.transactions.emplace_back( t.id() );
             }
         });

         for( const auto& trx : b.transactions ) {
            apply( db, trx, opts );
         }

         db.post_apply_block( b );
      undo_session.push();
   }

   void apply( database& db, const signed_transaction& t, const options_type& opts  ){
      auto undo_session = db.start_undo_session( !(opts & skip_undo_transaction) );
         db.pre_apply_transaction( t );
         for( const auto& op : t.operations ) {
            apply( db, op, opts );
         }
         db.post_apply_transaction( t );
      undo_session.squash();
   }

   struct apply_operation_visitor {
      apply_operation_visitor( database& db, const options_type& opts ):_db(db),_opts(opts){}

      typedef void result_type;

      template<typename T>
      void operator()( T&& op )const {
         apply( _db, std::forward<T>(op), _opts );
      }

      database&            _db;
      const options_type&  _opts;
   };

   void apply( database& db, const operation& o, const options_type& opts ) {
      auto undo_session = db.start_undo_session( !(opts & skip_undo_operation) );
         db.pre_apply_operation( o ); 
         o.visit( apply_operation_visitor( db, opts ) );
         db.post_apply_operation( o ); 
      undo_session.squash();
   }

   struct validate_operation_visitor {
      validate_operation_visitor( const options_type& opts ):_opts(opts){}

      typedef void result_type;

      template<typename T>
      void operator()( T&& op )const {
         validate( std::forward<T>(op), _opts );
      }
      const options_type&  _opts;
   };

   void validate( const operation& o, const options_type& opts ) {
     o.visit( validate_operation_visitor( opts ) );
   }

} } // steemit::chain2
