#pragma once

#include <steem/chain/util/asset.hpp>
#include <steem/chain/steem_objects.hpp>

#include <steem/protocol/asset.hpp>
#include <steem/protocol/config.hpp>
#include <steem/protocol/types.hpp>
#include <steem/protocol/misc_utilities.hpp>
#include <steem/protocol/block.hpp>

#include <fc/reflect/reflect.hpp>

#include <fc/uint128.hpp>

namespace steem { namespace chain { namespace util {

using steem::protocol::asset;
using steem::protocol::price;
using steem::protocol::share_type;

using namespace steem::protocol;

using fc::uint128_t;

struct _data
{
   fc::time_point begin_time;
   std::string begin_message;

   bool allowed_status = false;

   uint64_t max_delay;
   uint64_t step_delay = 0;

   uint64_t max_freq;
   uint64_t step_freq = 0;

   _data( uint64_t _max_delay, uint64_t _max_freq )
   : max_delay( _max_delay ), max_freq( _max_freq )
   {

   }

   void inc()
   {
      if( allowed_status )
      {
         if( max_delay != 0 )
         {
            ++step_delay;
            if( ( step_delay % max_delay ) == 0 )
            {
               allowed_status = false;
            }
         }
      }
      else
      {
         ++step_freq;
         if( ( step_freq % max_freq ) == 0 )
            allowed_status = true;
      }
   }

   void set_allowed( bool val )
   {
      allowed_status = val;
   }

   bool allowed()
   {
      return allowed_status;
   }

};

class prof
{
   fc::time_point time;

   _data meths;
   _data blocks;
   _data infos;
   
   static std::unique_ptr< prof > self;
   std::ofstream s_meths;
   std::ofstream s_blocks;
   std::ofstream s_info;

   using items_type = flat_map< std::string, uint64_t >;

   items_type items;

   prof()
   : s_meths( "__methods.txt" ), s_blocks( "__s_blocks.txt" ), s_info( "__info.txt" ), meths( 0, 20000 ), blocks( 0, 100000 ), infos( 1, 100 )
   {
      time = fc::time_point::now();
   }

   uint64_t add( std::string& key, uint64_t val )
   {
      auto ret = items.insert( std::make_pair( key, val ) );
      if( !ret.second )
         ret.first->second +=  val;

      return ret.first->second;
   }

   public:

      ~prof()
      {
         s_meths.flush();
         s_meths.close();
         
         s_blocks.flush();
         s_blocks.close();

         s_info.flush();
         s_info.close();
      }

      static std::unique_ptr< prof >& instance()
      {
         if( !self )
            self = std::unique_ptr< prof >( new prof() );

         return self;
      }

      void info_start( const signed_block& next_block )
      {
         meths.inc();
         infos.inc();
         blocks.inc();

         if( infos.allowed() )
         {
            uint64_t nr_operations = 0;
            for( const auto& trx : next_block.transactions )
            {
               nr_operations += trx.operations.size();
            }

            s_info<<"nr trxs: " << next_block.transactions.size() << " nr operations: " << nr_operations << "\n";
            s_info.flush();
         }
      }

      void info_end()
      {
         if( meths.allowed() )
         {
            meths.set_allowed( false );
            s_meths<<"*****\n";
            s_meths.flush();
         }
      }

      void info_end2()
      {
         if( blocks.allowed() )
         {
            blocks.set_allowed( false );
            s_blocks<<"*****\n";
            s_blocks.flush();
         }
      }

      void begin( const char* message )
      {
         if( meths.allowed() )
         {
            meths.begin_message = message;
            meths.begin_time = fc::time_point::now();
         }
      }

      void end()
      {
         if( meths.allowed() )
         {
            fc::microseconds us = fc::time_point::now() - meths.begin_time;
            uint64_t sum = add( meths.begin_message, us.count() );

            s_meths<<meths.begin_message<<" : "<< sum / 1000 <<" ms\n";
            s_meths.flush();
         }
      }

      void begin2( const char* message )
      {
         if( blocks.allowed() )
         {
            blocks.begin_message = message;
            blocks.begin_time = fc::time_point::now();
         }
      }

      void end2()
      {
         if( blocks.allowed() )
         {
            fc::microseconds us = fc::time_point::now() - blocks.begin_time;
            uint64_t sum = add( blocks.begin_message, us.count() );

            s_blocks<<blocks.begin_message<<" : "<< sum / 1000 <<" ms\n";
            s_blocks.flush();
         }
      }
};

struct comment_reward_context
{
   share_type rshares;
   uint16_t   reward_weight = 0;
   asset      max_sbd;
   uint128_t  total_reward_shares2;
   asset      total_reward_fund_steem;
   price      current_steem_price;
   protocol::curve_id   reward_curve = protocol::quadratic;
   uint128_t  content_constant = STEEM_CONTENT_CONSTANT_HF0;
};

uint64_t get_rshare_reward( const comment_reward_context& ctx );

inline uint128_t get_content_constant_s()
{
   return STEEM_CONTENT_CONSTANT_HF0; // looking good for posters
}

uint128_t evaluate_reward_curve( const uint128_t& rshares, const protocol::curve_id& curve = protocol::quadratic, const uint128_t& content_constant = STEEM_CONTENT_CONSTANT_HF0 );

inline bool is_comment_payout_dust( const price& p, uint64_t steem_payout )
{
   return to_sbd( p, asset( steem_payout, STEEM_SYMBOL ) ) < STEEM_MIN_PAYOUT_SBD;
}

} } } // steem::chain::util

FC_REFLECT( steem::chain::util::comment_reward_context,
   (rshares)
   (reward_weight)
   (max_sbd)
   (total_reward_shares2)
   (total_reward_fund_steem)
   (current_steem_price)
   (reward_curve)
   (content_constant)
   )
