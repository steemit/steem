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

   void update_tag( const tag_object& current, const comment_object& comment )const
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
      });
      add_stats( current, stats );
   }

   void create_tag( const string& tag, const comment_object& comment )const {
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
          obj.author            = author;
          obj.net_votes         = comment.net_votes;
      });
      add_stats( tag_obj, get_stats( tag ) );
   }


   /** finds tags that have been added or removed or updated */
   void update_tags( const comment_object& c )const {
      try {
      comment_metadata meta;

      if( c.json_metadata.size() ){
         meta = fc::json::from_string( c.json_metadata ).as<comment_metadata>();
      }

      set<string> lower_tags;
      for( const auto& tag : meta.tags )
         lower_tags.insert(fc::to_lower( tag ) );
      if( !_db.has_hardfork( STEEMIT_HARDFORK_0_5 ) )
         lower_tags.insert( c.category );

      meta.tags = lower_tags; /// TODO: std::move???
      if( meta.tags.size() > 1 ) wdump( (meta.tags) );

      const auto& comment_idx = _db.get_index_type<tag_index>().indices().get<by_comment>();
      auto citr = comment_idx.lower_bound( c.id );

      map<string, const tag_object*> existing_tags;
      while( citr != comment_idx.end() && citr->comment == c.id ) {
         const tag_object* tag = &*citr;
         ++citr;
         if( meta.tags.find( tag->tag ) == meta.tags.end() ) {
            remove_tag( *tag );
         } else {
            existing_tags[tag->tag] = tag;
         }
      }

      for( const auto& tag : meta.tags ) {
         auto existing = existing_tags.find(tag);
         if( existing == existing_tags.end() ) {
            create_tag( tag, c );
         } else {
            update_tag( *existing->second, c );
         }
      }

      if( c.parent_author.size() )
      {
         update_tags( _db.get_comment( c.parent_author, c.parent_permlink ) );
      }
     } FC_CAPTURE_AND_RETHROW( (c) )
   }

   void operator()( const comment_operation& op )const {
      update_tags( _db.get_comment( op.author, op.permlink ) );
   }

   void operator()( const vote_operation& op )const {
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
   database().on_applied_operation.connect( [&]( const operation_object& b){ my->on_operation(b); } );
   database().add_index< primary_index< tag_index  > >();
   database().add_index< primary_index< tag_stats_index > >();

   app().register_api_factory<tag_api>("tag_api");
}


void tags_plugin::plugin_startup()
{
}

} } /// steemit::tags

STEEMIT_DEFINE_PLUGIN( tags, steemit::tags::tags_plugin )
