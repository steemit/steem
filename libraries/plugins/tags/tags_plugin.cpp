#include <steem/plugins/tags/tags_plugin.hpp>

#include <steem/protocol/config.hpp>

#include <steem/chain/database.hpp>
#include <steem/chain/index.hpp>
#include <steem/chain/operation_notification.hpp>
#include <steem/chain/account_object.hpp>
#include <steem/chain/comment_object.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>
#include <fc/io/json.hpp>
#include <fc/string.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>

namespace steem { namespace plugins { namespace tags {

namespace detail {

using namespace steem::protocol;

class tags_plugin_impl
{
   public:
      tags_plugin_impl();
      virtual ~tags_plugin_impl();

      void pre_operation( const operation_notification& note );
      void on_operation( const operation_notification& note );

      chain::database&     _db;
      boost::signals2::connection   pre_apply_connection;
      boost::signals2::connection   post_apply_connection;
};

tags_plugin_impl::tags_plugin_impl() :
   _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ) {}

tags_plugin_impl::~tags_plugin_impl() {}

struct pre_apply_operation_visitor
{
   pre_apply_operation_visitor( database& db ) : _db( db ) {};
   typedef void result_type;

   database& _db;

   void operator()( const delete_comment_operation& op )const
   {
      const auto& comment = _db.find< comment_object, chain::by_permlink >( boost::make_tuple( op.author, op.permlink ) );

      if( comment == nullptr )
         return;

      const auto& idx = _db.get_index< tag_index, by_author_comment >();
      const auto& auth = _db.get_account( op.author );

      auto tag_itr = idx.lower_bound( boost::make_tuple( auth.id, comment->id ) );
      vector< const tag_object* > to_remove;

      while( tag_itr != idx.end() && tag_itr->author == auth.id && tag_itr->comment == comment->id )
      {
         to_remove.push_back( &(*tag_itr) );
         ++tag_itr;
      }

      for( const auto* tag_ptr : to_remove )
      {
         _db.remove( *tag_ptr );
      }
   }

   template<typename Op>
   void operator()( Op&& )const{} /// ignore all other ops
};

struct operation_visitor
{
   operation_visitor( database& db ):_db(db){};
   typedef void result_type;

   database& _db;

   void remove_stats( const tag_object& tag, const tag_stats_object& stats )const
   {
      _db.modify( stats, [&]( tag_stats_object& s )
      {
           if( tag.parent == comment_id_type() )
           {
              s.top_posts--;
           }
           else
           {
              s.comments--;
           }
           s.total_trending -= static_cast<uint32_t>(tag.trending);
           s.net_votes   -= tag.net_votes;
      });
   }

   void add_stats( const tag_object& tag, const tag_stats_object& stats )const
   {
      _db.modify( stats, [&]( tag_stats_object& s )
      {
           if( tag.parent == comment_id_type() )
           {
              s.top_posts++;
           }
           else
           {
              s.comments++;
           }
           s.total_trending += static_cast<uint32_t>(tag.trending);
           s.net_votes   += tag.net_votes;
      });
   }

   void remove_tag( const tag_object& tag )const
   {
      /// TODO: update tag stats object
      _db.remove(tag);

      const auto& idx = _db.get_index<author_tag_stats_index>().indices().get<by_author_tag_posts>();
      auto itr = idx.lower_bound( boost::make_tuple(tag.author,tag.tag) );
      if( itr != idx.end() && itr->author == tag.author && itr->tag == tag.tag )
      {
         _db.modify( *itr, [&]( author_tag_stats_object& stats )
         {
            stats.total_posts--;
         });
      }
   }

   const tag_stats_object& get_stats( const string& tag )const
   {
      const auto& stats_idx = _db.get_index<tag_stats_index>().indices().get<by_tag>();
      auto itr = stats_idx.find( tag );
      if( itr != stats_idx.end() )
         return *itr;

      return _db.create<tag_stats_object>( [&]( tag_stats_object& stats )
      {
         stats.tag = tag;
      });
   }

   comment_metadata filter_tags( const comment_object& c, const comment_content_object& con ) const
   {
      comment_metadata meta;

      if( con.json_metadata.size() )
      {
         try
         {
            meta = fc::json::from_string( to_string( con.json_metadata ) ).as< comment_metadata >();
         }
         catch( const fc::exception& e )
         {
            // Do nothing on malformed json_metadata
         }
      }

      // We need to write the transformed tags into a temporary
      // local container because we can't modify meta.tags concurrently
      // as we iterate over it.
      set< string > lower_tags;

      uint8_t tag_limit = 5;
      uint8_t count = 0;
      for( const string& tag : meta.tags )
      {
         ++count;
         if( count > tag_limit || lower_tags.size() > tag_limit )
            break;
         if( tag == "" )
            continue;
         lower_tags.insert( fc::to_lower( tag ) );
      }

      /// the universal tag applies to everything safe for work or nsfw with a non-negative payout
      if( c.net_rshares >= 0 )
      {
         lower_tags.insert( string() ); /// add it to the universal tag
      }

      meta.tags = lower_tags; /// TODO: std::move???

      return meta;
   }

   void update_tag( const tag_object& current, const comment_object& comment, double hot, double trending )const
   {
       const auto& stats = get_stats( current.tag );
       remove_stats( current, stats );

       if( comment.cashout_time != fc::time_point_sec::maximum() ) {
          _db.modify( current, [&]( tag_object& obj ) {
             obj.active            = comment.active;
             obj.cashout           = _db.calculate_discussion_payout_time( comment );
             obj.children          = comment.children;
             obj.net_rshares       = comment.net_rshares.value;
             obj.net_votes         = comment.net_votes;
             obj.hot               = hot;
             obj.trending          = trending;
             if( obj.cashout == fc::time_point_sec() )
               obj.promoted_balance = 0;
         });
         add_stats( current, stats );
       } else {
          _db.remove( current );
       }
   }

   void create_tag( const string& tag, const comment_object& comment, double hot, double trending )const
   {
      comment_id_type parent;
      account_id_type author = _db.get_account( comment.author ).id;

      if( comment.parent_author.size() )
         parent = _db.get_comment( comment.parent_author, comment.parent_permlink ).id;

      const auto& tag_obj = _db.create<tag_object>( [&]( tag_object& obj )
      {
          obj.tag               = tag;
          obj.comment           = comment.id;
          obj.parent            = parent;
          obj.created           = comment.created;
          obj.active            = comment.active;
          obj.cashout           = comment.cashout_time;
          obj.net_votes         = comment.net_votes;
          obj.children          = comment.children;
          obj.net_rshares       = comment.net_rshares.value;
          obj.author            = author;
          obj.hot               = hot;
          obj.trending          = trending;
      });
      add_stats( tag_obj, get_stats( tag ) );


      const auto& idx = _db.get_index<author_tag_stats_index>().indices().get<by_author_tag_posts>();
      auto itr = idx.lower_bound( boost::make_tuple(author,tag) );
      if( itr != idx.end() && itr->author == author && itr->tag == tag )
      {
         _db.modify( *itr, [&]( author_tag_stats_object& stats )
         {
            stats.total_posts++;
         });
      }
      else
      {
         _db.create<author_tag_stats_object>( [&]( author_tag_stats_object& stats )
         {
            stats.author = author;
            stats.tag    = tag;
            stats.total_posts = 1;
         });
      }
   }


   /**
    * https://medium.com/hacking-and-gonzo/how-reddit-ranking-algorithms-work-ef111e33d0d9#.lcbj6auuw
    */
   template< int64_t S, int32_t T >
   double calculate_score( const share_type& score, const time_point_sec& created ) const
   {
      /// new algorithm
      auto mod_score = score.value / S;

      /// reddit algorithm
      double order = log10( std::max<int64_t>( std::abs( mod_score ), 1) );
      int sign = 0;
      if( mod_score > 0 ) sign = 1;
      else if( mod_score < 0 ) sign = -1;

      return sign * order + double( created.sec_since_epoch() ) / double( T );
   }

   inline double calculate_hot( const share_type& score, const time_point_sec& created )const
   {
      return calculate_score< 10000000, 10000 >( score, created );
   }

   inline double calculate_trending( const share_type& score, const time_point_sec& created )const
   {
      return calculate_score< 10000000, 480000 >( score, created );
   }

   /** finds tags that have been added or removed or updated */
   void update_tags( const comment_object& c, bool parse_tags = false )const
   {
      try {

      auto hot = calculate_hot( c.net_rshares, c.created );
      auto trending = calculate_trending( c.net_rshares, c.created );

      const auto& comment_idx = _db.get_index< tag_index >().indices().get< by_comment >();

#ifndef IS_LOW_MEM
      if( parse_tags )
      {
         auto meta = filter_tags( c, _db.get< comment_content_object, chain::by_comment >( c.id ) );
         auto citr = comment_idx.lower_bound( c.id );

         map< string, const tag_object* > existing_tags;
         vector< const tag_object* > remove_queue;

         while( citr != comment_idx.end() && citr->comment == c.id )
         {
            const tag_object* tag = &*citr;
            ++citr;

            if( meta.tags.find( tag->tag ) == meta.tags.end()  )
            {
               remove_queue.push_back(tag);
            }
            else
            {
               existing_tags[tag->tag] = tag;
            }
         }

         for( const auto& tag : meta.tags )
         {
            auto existing = existing_tags.find(tag);

            if( existing == existing_tags.end() )
            {
               create_tag( tag, c, hot, trending );
            }
            else
            {
               update_tag( *existing->second, c, hot, trending );
            }
         }

         for( const auto& item : remove_queue )
            remove_tag(*item);
      }
      else
#endif
      {
         auto citr = comment_idx.lower_bound( c.id );

         while( citr != comment_idx.end() && citr->comment == c.id )
         {
            update_tag( *citr, c, hot, trending );
            ++citr;
         }
      }

      if( c.parent_author.size() )
      {
         update_tags( _db.get_comment( c.parent_author, c.parent_permlink ) );
      }
      } FC_CAPTURE_LOG_AND_RETHROW( (c) )
   }

   void operator()( const comment_operation& op )const
   {
      update_tags( _db.get_comment( op.author, op.permlink ), op.json_metadata.size() );
   }

   void operator()( const transfer_operation& op )const
   {
      if( op.to == STEEM_NULL_ACCOUNT && op.amount.symbol == SBD_SYMBOL )
      {
         vector<string> part; part.reserve(4);
         auto path = op.memo;
         boost::split( part, path, boost::is_any_of("/") );
         if( part.size() > 1 && part[0].size() && part[0][0] == '@' )
         {
            auto acnt = part[0].substr(1);
            auto perm = part[1];

            auto c = _db.find_comment( acnt, perm );
            if( c && c->parent_author.size() == 0 )
            {
               const auto& comment_idx = _db.get_index<tag_index>().indices().get<by_comment>();
               auto citr = comment_idx.lower_bound( c->id );
               while( citr != comment_idx.end() && citr->comment == c->id )
               {
                  _db.modify( *citr, [&]( tag_object& t )
                  {
                      if( t.cashout != fc::time_point_sec::maximum() )
                          t.promoted_balance += op.amount.amount;
                  });
                  ++citr;
               }
            }
            else
            {
               ilog( "unable to find body" );
            }
         }
      }
   }

   void operator()( const vote_operation& op )const
   {
      update_tags( _db.get_comment( op.author, op.permlink ) );
   }

   void operator()( const comment_reward_operation& op )const
   {
      const auto& c = _db.get_comment( op.author, op.permlink );
      update_tags( c );

#ifndef IS_LOW_MEM
      comment_metadata meta = filter_tags( c, _db.get< comment_content_object, chain::by_comment >( c.id ) );

      for( const string& tag : meta.tags )
      {
         _db.modify( get_stats( tag ), [&]( tag_stats_object& ts )
         {
            ts.total_payout += op.payout;
         });
      }
#endif
   }

   void operator()( const comment_payout_update_operation& op )const
   {
      const auto& c = _db.get_comment( op.author, op.permlink );
      update_tags( c );
   }

   template<typename Op>
   void operator()( Op&& )const{} /// ignore all other ops
};

void tags_plugin_impl::pre_operation( const operation_notification& note )
{
   try
   {
      /// plugins shouldn't ever throw
      note.op.visit( pre_apply_operation_visitor( _db ) );
   }
   catch ( const fc::exception& e )
   {
      edump( (e.to_detail_string()) );
   }
   catch ( ... )
   {
      elog( "unhandled exception" );
   }
}

void tags_plugin_impl::on_operation( const operation_notification& note )
{
   try
   {
      /// plugins shouldn't ever throw
      note.op.visit( operation_visitor( _db ) );
   }
   catch ( const fc::exception& e )
   {
      edump( (e.to_detail_string()) );
   }
   catch ( ... )
   {
      elog( "unhandled exception" );
   }
}

} /// end detail namespace

tags_plugin::tags_plugin() {}
tags_plugin::~tags_plugin() {}

void tags_plugin::set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
   )
{}

void tags_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   ilog("Intializing tags plugin" );
   my = std::make_unique< detail::tags_plugin_impl >();

   my->pre_apply_connection = my->_db.pre_apply_operation.connect(  [&]( const operation_notification& note ){ my->pre_operation( note ); } );
   my->post_apply_connection = my->_db.post_apply_operation.connect( [&]( const operation_notification& note ){ my->on_operation(  note ); } );

   add_plugin_index< tag_index               >( my->_db );
   add_plugin_index< tag_stats_index         >( my->_db );
   add_plugin_index< author_tag_stats_index  >( my->_db );

}


void tags_plugin::plugin_startup() {}
void tags_plugin::plugin_shutdown()
{
   chain::util::disconnect_signal( my->pre_apply_connection );
   chain::util::disconnect_signal( my->post_apply_connection );
}

} } } /// steem::plugins::tags
