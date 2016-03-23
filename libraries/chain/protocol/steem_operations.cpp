#include <steemit/chain/protocol/steem_operations.hpp>
#include <fc/io/json.hpp>

#include <locale>

#include <fc/utf8.hpp>

namespace steemit { namespace chain {

   void inline validate_permlink( string permlink )
   {
      FC_ASSERT( permlink.size() > 0 && permlink.size() < 256 );
      FC_ASSERT( fc::is_utf8( permlink ) );
      FC_ASSERT( fc::to_lower( permlink ) == permlink );
      FC_ASSERT( fc::trim_and_normalize_spaces( permlink ) == permlink );

      for ( auto c : permlink )
      {
         FC_ASSERT( c > 0 );
         switch( c )
         {
            case ' ':
            case '\t':
            case '\n':
            case ':':
            case '#':
            case '/':
            case '\\':
            case '%':
            case '=':
            case '@':
            case '~':
            case '.':
               FC_ASSERT( !"Invalid permlink character:", "${s}", ("s", std::string() + c ) );
         }
      }
   }

   void account_create_operation::validate() const
   {
      FC_ASSERT(is_valid_account_name( new_account_name ));
      owner.validate();
      active.validate();

      if ( json_metadata.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(json_metadata) );
         FC_ASSERT( fc::json::is_valid(json_metadata) );
      }
      FC_ASSERT( fee >= asset( 0, STEEM_SYMBOL ) );
   }

   void account_update_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( account ) );
      if( owner )
         owner->validate();
      if (active)
         active->validate();

      if ( json_metadata.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(json_metadata) );
         FC_ASSERT( fc::json::is_valid(json_metadata) );
      }
   }

   void comment_operation::validate() const
   {
      FC_ASSERT( title.size() < 256 );
      FC_ASSERT( fc::is_utf8( title ) );
      FC_ASSERT( body.size() > 0 );
      FC_ASSERT( fc::is_utf8( body ) );

      FC_ASSERT( !parent_author.size() || is_valid_account_name( parent_author ) );
      FC_ASSERT( is_valid_account_name( author ) );
      validate_permlink( permlink );
      if ( parent_author.size() > 0 )
         validate_permlink( parent_permlink );

      if( json_metadata.size() > 0 )
      {
         FC_ASSERT( fc::json::is_valid(json_metadata) );
      }
   }

   void vote_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( voter ) );
      FC_ASSERT( is_valid_account_name( author ) );
      validate_permlink( permlink );
      FC_ASSERT( abs(weight) <= STEEMIT_100_PERCENT );
      FC_ASSERT( weight != 0 );
   }

   void transfer_operation::validate() const
   { try {
      FC_ASSERT( is_valid_account_name( from ) );
      FC_ASSERT( is_valid_account_name( to ) );
      FC_ASSERT( amount.amount > 0);
      FC_ASSERT( memo.size() < STEEMIT_MAX_MEMO_SIZE );
      FC_ASSERT( fc::is_utf8( memo ) );
   } FC_CAPTURE_AND_RETHROW( (*this) ) }

   void transfer_to_vesting_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( from ) );
      if ( !to.empty() ) FC_ASSERT( is_valid_account_name( to ) );
      FC_ASSERT( amount > asset( 0, STEEM_SYMBOL ) );
   }

   void withdraw_vesting_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( account ) );
      FC_ASSERT( vesting_shares.amount > 0 );
   }

   void witness_update_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( owner ) );
      FC_ASSERT( url.size() > 0 );
      FC_ASSERT( fc::is_utf8( url ) );
      FC_ASSERT( fee >= asset( 0, STEEM_SYMBOL ) );
      props.validate();
   }

   void account_witness_vote_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( account ) );
      FC_ASSERT( is_valid_account_name( witness ) );
   }
   void account_witness_proxy_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( account ) );
      if( proxy.size() )
         FC_ASSERT( is_valid_account_name( proxy ) );
      FC_ASSERT( proxy != account );
   }

   void custom_operation::validate() const {
      /// required auth accounts are the ones whose bandwidth is consumed
      FC_ASSERT( required_auths.size() > 0, "at least on account must be specified" );
   }

   fc::sha256 pow_operation::work_input()const
   {
      auto hash = fc::sha256::hash( block_id );
      hash._hash[0] = nonce;
      return fc::sha256::hash( hash );
   }
   void pow_operation::validate()const
   {
      props.validate();
      FC_ASSERT( is_valid_account_name( worker_account ) );
      FC_ASSERT( work_input() == work.input );
      work.validate();
   }

   void pow::create( const fc::ecc::private_key& w, const digest_type& i ) {
      worker = w.get_public_key();
      input  = i;
      signature = w.sign_compact(input);

      auto sig_hash            = fc::sha256::hash( signature );
      public_key_type recover  = fc::ecc::public_key( signature, sig_hash );

      work = fc::sha256::hash(recover);
   }

   void pow::validate()const
   {
      FC_ASSERT( work != fc::sha256() );
      FC_ASSERT( public_key_type(fc::ecc::public_key( signature, input, false )) == worker );
      auto sig_hash = fc::sha256::hash( signature );
      public_key_type recover  = fc::ecc::public_key( signature, sig_hash, false );
      FC_ASSERT( work == fc::sha256::hash(recover) );
   }

   void feed_publish_operation::validate()const {
      FC_ASSERT( is_valid_account_name( publisher ) );
      FC_ASSERT( exchange_rate.base.symbol == STEEM_SYMBOL );
      FC_ASSERT( exchange_rate.quote.symbol == SBD_SYMBOL );
      exchange_rate.validate();
   }

   void limit_order_create_operation::validate()const {
      FC_ASSERT( is_valid_account_name( owner ) );
      (amount_to_sell / min_to_receive).validate();
   }

   void limit_order_cancel_operation::validate()const {
      FC_ASSERT( is_valid_account_name( owner ) );
   }

   void convert_operation::validate()const {
      FC_ASSERT( is_valid_account_name( owner ) );
      /// only allow conversion from SBD to STEEM, allowing the opposite can enable traders to abuse
      /// market fluxuations through converting large quantities without moving the price.
      FC_ASSERT( /*amount.symbol == STEEM_SYMBOL ||*/ amount.symbol == SBD_SYMBOL );
      FC_ASSERT( amount.amount > 0 );
   }

   void report_over_production_operation::validate()const {
      FC_ASSERT( is_valid_account_name( reporter ) );
      FC_ASSERT( is_valid_account_name( first_block.witness ) );
      FC_ASSERT( first_block.witness   == second_block.witness );
      FC_ASSERT( first_block.timestamp == second_block.timestamp );
      FC_ASSERT( first_block.signee()  == second_block.signee() );
   }


} } // steemit::chain
