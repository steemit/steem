#include <steemit/tags/tags_plugin.hpp>

#include <steemit/app/impacted.hpp>

#include <steemit/chain/config.hpp>
#include <steemit/chain/database.hpp>
#include <steemit/chain/history_object.hpp>
#include <steemit/chain/comment_object.hpp>
#include <steemit/chain/account_object.hpp>
#include <steemit/chain/hardfork.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>
#include <fc/io/json.hpp>
#include <fc/string.hpp>

namespace steemit { namespace tags {

namespace detail {

class tags_plugin_impl
{
   public:
      tags_plugin_impl(tags_plugin& _plugin)
         : _self( _plugin )
      { }
      virtual ~tags_plugin_impl();

      steemit::chain::database& database()
      {
         return _self.database();
      }

      void on_operation( const operation_object& op_obj );

      tags_plugin& _self;
};

tags_plugin_impl::~tags_plugin_impl()
{
   return;
}

struct operation_visitor {
   operation_visitor( database& db ):_db(db){};
   typedef void result_type;

   database& _db;

   void remove_stats( const tag_object& tag, const tag_stats_object& stats )const {
      _db.modify( stats, [&]( tag_stats_object& s ) {
           if( tag.parent == comment_id_type() ) {
              s.total_children_rshares2 -= tag.children_rshares2;
              s.top_posts--;
           } else {
              s.comments--;
           }
           s.net_votes   -= tag.net_votes;
           s.total_payout += tag.total_payout;
      });
   }
   void add_stats( const tag_object& tag, const tag_stats_object& stats )const {
      _db.modify( stats, [&]( tag_stats_object& s ) {
           if( tag.parent == comment_id_type() ) {
              s.total_children_rshares2 += tag.children_rshares2;
              s.top_posts++;
           } else {
              s.comments++;
           }
           s.net_votes   += tag.net_votes;
      });
   }

   void remove_tag( const tag_object& tag )const {
      /// TODO: update tag stats object
      _db.remove(tag);
   }

   const tag_stats_object& get_stats( const string& tag )const {
      const auto& stats_idx = _db.get_index_type<tag_stats_index>().indices().get<by_tag>();
      auto itr = stats_idx.find( tag );
      if( itr != stats_idx.end() ) return *itr;

      return _db.create<tag_stats_object>( [&]( tag_stats_object& stats ) {
                  stats.tag = tag;
              });
   }

   void update_tag( const tag_object& current, const comment_object& comment, double hot )const
   {
       const auto& stats = get_stats( current.tag );
       remove_stats( current, stats );
       _db.modify( current, [&]( tag_object& obj ) {
          obj.active            = comment.active;
          obj.cashout           = comment.cashout_time;
          obj.children          = comment.children;
          obj.net_rshares       = comment.net_rshares.value;
          obj.net_votes         = comment.net_votes;
          obj.children_rshares2 = comment.children_rshares2;
          obj.hot               = hot;
          obj.total_payout      = comment.total_payout_value;
      });
      add_stats( current, stats );
   }

   void create_tag( const string& tag, const comment_object& comment, double hot )const {


      comment_id_type parent;
      account_id_type author = _db.get_account( comment.author ).id;

      if( comment.parent_author.size() )
         parent = _db.get_comment( comment.parent_author, comment.parent_permlink ).id;

      const auto& tag_obj = _db.create<tag_object>( [&]( tag_object& obj ) {
          obj.tag               = tag;
          obj.comment           = comment.id;
          obj.parent            = parent;
          obj.created           = comment.created;
          obj.active            = comment.active;
          obj.cashout           = comment.cashout_time;
          obj.net_votes         = comment.net_votes;
          obj.children          = comment.children;
          obj.net_rshares       = comment.net_rshares.value;
          obj.children_rshares2 = comment.children_rshares2;
          obj.total_payout      = comment.total_payout_value;
          obj.author            = author;
          obj.net_votes         = comment.net_votes;
      });
      add_stats( tag_obj, get_stats( tag ) );
   }

   /**
    * https://medium.com/hacking-and-gonzo/how-reddit-ranking-algorithms-work-ef111e33d0d9#.lcbj6auuw
    */
   double calculate_hot( const comment_object& c )const {
      auto s = c.net_votes;
      double order = log10( std::max<int32_t>( abs(s), 1) );
      int sign = 0;
      if( s > 0 ) sign = 1;
      else if( s < 0 ) sign = -1;
      auto seconds = c.created.sec_since_epoch();

      return sign * order + double(seconds) / 45000.0;
   }

   /** finds tags that have been added or removed or updated */
   void update_tags( const comment_object& c )const {
      try {

      auto hot = calculate_hot(c);

      comment_metadata meta;

      if( c.json_metadata.size() ){
         meta = fc::json::from_string( c.json_metadata ).as<comment_metadata>();
      }

      set<string> lower_tags;
      for( const auto& tag : meta.tags )
         lower_tags.insert(fc::to_lower( tag ) );
      if( !_db.has_hardfork( STEEMIT_HARDFORK_0_5 ) )
         lower_tags.insert( c.category );


      /// the universal tag applies to everything safe for work or nsfw with a positive payout
      if( c.net_rshares >= 0 ||
          (lower_tags.find( "spam" ) == lower_tags.end() &&
           lower_tags.find( "nsfw" ) == lower_tags.end() &&
           lower_tags.find( "test" ) == lower_tags.end() )  )
      {
         lower_tags.insert( string() ); /// add it to the universal tag
      }

      meta.tags = lower_tags; /// TODO: std::move???

      const auto& comment_idx = _db.get_index_type<tag_index>().indices().get<by_comment>();
      auto citr = comment_idx.lower_bound( c.id );

      map<string, const tag_object*> existing_tags;
      vector<const tag_object*> remove_queue;
      while( citr != comment_idx.end() && citr->comment == c.id ) {
         const tag_object* tag = &*citr;
         ++citr;
         if( meta.tags.find( tag->tag ) == meta.tags.end() ) {
            remove_queue.push_back(tag);
         } else {
            existing_tags[tag->tag] = tag;
         }
      }

      for( const auto& tag : meta.tags ) {
         auto existing = existing_tags.find(tag);
         if( existing == existing_tags.end() ) {
            create_tag( tag, c, hot );
         } else {
            update_tag( *existing->second, c, hot );
         }
      }

      for( const auto& item : remove_queue )
         remove_tag(*item);

      if( c.parent_author.size() )
      {
         update_tags( _db.get_comment( c.parent_author, c.parent_permlink ) );
      }
     } FC_CAPTURE_LOG_AND_RETHROW( (c) )
   }

   const peer_stats_object& get_or_create_peer_stats( account_id_type voter, account_id_type peer )const {
      const auto& peeridx = _db.get_index_type<peer_stats_index>().indices().get<by_voter_peer>();
      auto itr = peeridx.find( boost::make_tuple( voter, peer ) );
      if( itr == peeridx.end() ) {
         return _db.create<peer_stats_object>( [&]( peer_stats_object& obj ) {
               obj.voter = voter;
               obj.peer  = peer;
         });
      }
      return *itr;
   }
   void update_indirect_vote( account_id_type a, account_id_type b, int positive )const {
      if( a == b ) return;
      const auto& ab = get_or_create_peer_stats( a, b );
      const auto& ba = get_or_create_peer_stats( b, a );
      _db.modify( ab, [&]( peer_stats_object& o ) {
                  o.indirect_positive_votes += positive;
                  o.indirect_votes++;
                  o.update_rank();
                  });
      _db.modify( ba, [&]( peer_stats_object& o ) {
                  o.indirect_positive_votes += positive;
                  o.indirect_votes++;
                  o.update_rank();
                  });
   }

   void update_peer_stats( const account_object& voter, const account_object& author, const comment_object& c, int vote )const {
      if( voter.id == author.id ) return; /// ignore votes for yourself
      if( c.parent_author.size() ) return; /// only count top level posts

      const auto& stat = get_or_create_peer_stats( voter.get_id(), author.get_id() );
      _db.modify( stat, [&]( peer_stats_object& obj ) {
            obj.direct_votes++;
            obj.direct_positive_votes += vote > 0;
            obj.update_rank();
      });

      const auto& voteidx = _db.get_index_type<comment_vote_index>().indices().get<by_comment_voter>();
      auto itr = voteidx.lower_bound( boost::make_tuple( comment_id_type(c.id), account_id_type() ) );
      while( itr != voteidx.end() && itr->comment == c.id ) {
         update_indirect_vote( voter.id, itr->voter, (itr->vote_percent > 0)  == (vote > 0) );
         ++itr;
      }
   }

   void operator()( const comment_operation& op )const {
      update_tags( _db.get_comment( op.author, op.permlink ) );
   }

   void operator()( const vote_operation& op )const {
      update_tags( _db.get_comment( op.author, op.permlink ) );
      update_peer_stats( _db.get_account(op.voter),
                         _db.get_account(op.author),
                         _db.get_comment(op.author, op.permlink),
                         op.weight );
   }

   void operator()( const comment_payout_operation& op )const {
       update_tags( _db.get_comment( op.author, op.permlink ) );
   }

   template<typename Op>
   void operator()( Op&& )const{} /// ignore all other ops
};



void tags_plugin_impl::on_operation( const operation_object& op_obj ) {
   try { /// plugins shouldn't ever throw
      op_obj.op.visit( operation_visitor( database() ) );
   } catch ( const fc::exception& e ) {
      edump( (e.to_detail_string()) );
   } catch ( ... ) {
      elog( "unhandled exception" );
   }
}

} /// end detail namespace

tags_plugin::tags_plugin() :
   my( new detail::tags_plugin_impl(*this) )
{
   //ilog("Loading account history plugin" );
}

tags_plugin::~tags_plugin()
{
}

std::string tags_plugin::plugin_name()const
{
   return "tags";
}

void tags_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
   )
{
}

void tags_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   ilog("Intializing tags plugin" );
   database().post_apply_operation.connect( [&]( const operation_object& b){ my->on_operation(b); } );
   database().add_index< primary_index< tag_index  > >();
   database().add_index< primary_index< tag_stats_index > >();
   database().add_index< primary_index< peer_stats_index > >();

   app().register_api_factory<tag_api>("tag_api");
}


void tags_plugin::plugin_startup()
{
}

} } /// steemit::tags

STEEMIT_DEFINE_PLUGIN( tags, steemit::tags::tags_plugin )
