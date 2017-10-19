
#include <steem/plugins/token_api/dict.hpp>

#include <steem/chain/comment_object.hpp>
#include <steem/chain/account_object.hpp>

#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

namespace steem { namespace plugins { namespace token_api {

catcher::catcher()
      : key_counter( 0 ), val_counter( 0 ),total_size( 0 )
{

}

void catcher::fill_dict( const std::string& line, content::pcontent& post )
{
   std::vector< std::string > words;
   boost::split( words, line, boost::is_any_of(", .\\/!?\"()#'$&%+-{}") );

   auto& vals_idx = dict.get<by_item_val>();

   for( std::string& actual : words )
   {
      auto found = vals_idx.find( actual );
      if( found == vals_idx.end() )
      {
         vals_idx.emplace( std::move( item( actual, key_counter ) ) );
         post->keys.push_back( key_counter );

         ++key_counter;
         val_counter += actual.size();
      }
      else
         post->keys.push_back( found->id );
   }
}

void catcher::scan()
{
   for( auto& actual_post : posts )
   for( std::string& actual : actual_post->vals )
   {
      total_size += actual.size();
      fill_dict( actual, actual_post );
   }
}

void catcher::summary( std::ofstream& f )
{
   types::key_type sum_key = sizeof( types::key_type ) * key_counter;

   uint64_t _tmp_sum = sum_key + val_counter;

   f << " A:   Used memory( input content )           :"<< total_size << "\n\n";
   f << " B:   Nr of elements in dict                 :"<< key_counter << " \n";
   f << " C:   Nr of elements in dict * sizeof( key ) :"<< sum_key << " \n";
   f << " D:   Size of elements in dict               :"<< val_counter << " \n";
   f << " E:   Used memory( C + D )                   :"<< _tmp_sum << " \n";
   if( _tmp_sum != 0 )
      f << " F:   Ratio( A / E  )                        :"<< (double)total_size / _tmp_sum << " \n";
}

void catcher::summary()
{
   std::ofstream f( "summary.txt" );

   summary( f );

   f.flush();
   f.close();
}

file_catcher::file_catcher( const std::string _directory )
            : directory( _directory )
{

}

void file_catcher::read_files()
{
   std::string line;

   boost::filesystem::path p( directory.c_str() );

   for( boost::filesystem::directory_iterator it(p); it != boost::filesystem::directory_iterator(); ++it )
   {
      boost::filesystem::path pit = it->path();
      std::ifstream f( pit.string(), std::ifstream::in );

      if( f.is_open() )
      {
         content::pcontent post( new file_content() );

         while( getline( f, line ) )
            post->vals.push_back( line );

         post->set_file_name( std::move( "o" + pit.string() ) );
         posts.push_back( post );
      }
      f.close();
   }
}

void file_catcher::save_files()
{
   auto& vals_idx = dict.get<by_item_id>();

   for( auto& actual_post : posts )
   {
      const std::string& path = actual_post->get_file_name();
      if( path.size() )
      {
         std::ofstream f( path );
         for( auto& actual : actual_post->keys )
         {
            auto found = vals_idx.find( actual );
            if( found != vals_idx.end() )
               f << found->val;
         }
         f.flush();
         f.close();
      }
   }
}

void file_catcher::get_content()
{
   read_files();
}

void file_catcher::save_content()
{
   save_files();
}

db_catcher::db_catcher( chain::database* _db )
         : db( _db )
{

}

void db_catcher::read_db()
{
   const auto& items = db->get_index< steem::chain::comment_content_index, steem::chain::by_comment >();

   std::string body;
   std::string exc;

   uint32_t idx = 0;

   std::ofstream f_counter( "counter.txt" );
   f_counter << items.size() <<"\n";
   f_counter.flush();

   for( auto& item : items )
   {
      const steem::chain::comment_content_object& c = item;
      try
      {
         ++idx;
         if( ( idx % 100000 ) == 0 )
         {                                                                                                                                                                                                                                                                                                                                                                                                                 
            f_counter << idx <<"\n";
            summary( f_counter );
            f_counter.flush();
         }                                                                                                           

         content::pcontent post( new file_content() );

         total_size += strlen( c.body.c_str() );

         fill_dict( c.body.c_str(), post );
      }
      catch( fc::exception e )
      {
      }
      catch( std::exception e )
      {
      }

   }

   f_counter.close();
}

void db_catcher::get_content()
{
   read_db();
}

void db_catcher::scan()
{
   //Empty
}

tokenizer::tokenizer( catcher& _c )
         :  c( _c )
{
}

void tokenizer::preprocess()
{
   c.get_content();
   c.scan();
}

void tokenizer::save_content()
{
   c.save_content();
}

void tokenizer::summary()
{
   c.summary();
}

} } } // steem::plugins::token_api

