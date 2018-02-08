#include <golos/plugins/debug_node/plugin.hpp>

#include <golos/chain/witness_objects.hpp>

#include <fc/io/buffered_iostream.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>

#include <fc/time.hpp>

#include <fc/thread/future.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>

#include <graphene/utilities/key_conversion.hpp>


#include <sstream>
#include <string>

// #define CHECK_ARG_SIZE(s) \
//    FC_ASSERT( args.args->size() == s, "Expected #s argument(s), was ${n}", ("n", args.args->size()) );

namespace golos { namespace plugins { namespace debug_node {

using namespace golos::chain;

struct plugin::plugin_impl {
public:
    plugin_impl() : db_(appbase::app().get_plugin<plugins::chain::plugin>().db()) {
    }

    // APIs
    debug_generate_blocks_r                debug_generate_blocks               (debug_generate_blocks_a & args);
    debug_push_blocks_r                    debug_push_blocks                   (debug_push_blocks_a & args);
    debug_generate_blocks_until_r          debug_generate_blocks_until         (debug_generate_blocks_until_a & args);
    debug_pop_block_r                      debug_pop_block                     (debug_pop_block_a & args);
    debug_get_witness_schedule_r           debug_get_witness_schedule          (debug_get_witness_schedule_a & args);
    debug_set_hardfork_r                   debug_set_hardfork                  (debug_set_hardfork_a & args);
    debug_has_hardfork_r                   debug_has_hardfork                  (debug_has_hardfork_a & args);
    //
    golos::chain::database &database() {
      return db_;
    }

    template< typename Lambda >
    void debug_update( Lambda&& callback, uint32_t skip = golos::chain::database::skip_nothing )
    {
        // this was a method on database in Graphene
        golos::chain::database& db = database();
        golos::chain::block_id_type head_id = db.head_block_id();
        auto it = _debug_updates.find( head_id );
        if( it == _debug_updates.end() ) {
            it = _debug_updates.emplace( head_id, std::vector< std::function< void( golos::chain::database& ) > >() ).first;
        }
        it->second.emplace_back( callback );

        fc::optional<golos::chain::signed_block> head_block = db.fetch_block_by_id( head_id );
        FC_ASSERT( head_block.valid() );

        // What the last block does has been changed by adding to node_property_object, so we have to re-apply it
        db.pop_block();
        db.push_block( *head_block, skip );
    }

    void apply_debug_updates();
    void on_applied_block( const protocol::signed_block & b );


    // void save_debug_updates( fc::mutable_variant_object& target );
    // void load_debug_updates( const fc::variant_object& target );

    bool logging = true;

    boost::signals2::connection applied_block_connection;
    std::map< protocol::block_id_type, std::vector< std::function< void( golos::chain::database& ) > > > _debug_updates;
private:
    golos::chain::database & db_;
};

plugin::plugin() {

}

plugin::~plugin() {

}

void plugin::set_program_options (
   boost::program_options::options_description &cli,
   boost::program_options::options_description &cfg
) {
   cfg.add_options()
      ("debug-node-edit-script,e",
         boost::program_options::value< std::vector< std::string > >()->composing(),
            "Database edits to apply on startup (may specify multiple times)")
      ("edit-script", boost::program_options::value< std::vector< std::string > >()->composing(),
          "Database edits to apply on startup (may specify multiple times). Deprecated in favor of debug-node-edit-script.")
   ;
}

void plugin::plugin_initialize( const boost::program_options::variables_map& options ) {
    my.reset(new plugin_impl);

    if( options.count( "debug-node-edit-script" ) > 0 ) {
        _edit_scripts = options.at( "debug-node-edit-script" ).as< std::vector< std::string > >();
    }

    if( options.count("edit-script") > 0 ) {
        wlog( "edit-scripts is deprecated in favor of debug-node-edit-script" );
        auto scripts = options.at( "edit-script" ).as< std::vector< std::string > >();
        _edit_scripts.insert( _edit_scripts.end(), scripts.begin(), scripts.end() );
    }

    // connect needed signals
    my->applied_block_connection = my->database().applied_block.connect( [this](const golos::chain::signed_block& b){
        on_applied_block(b);
    });

    JSON_RPC_REGISTER_API ( name() );
}

void plugin::plugin_startup() {
   /*for( const std::string& fn : _edit_scripts )
   {
      std::shared_ptr< fc::ifstream > stream = std::make_shared< fc::ifstream >( fc::path(fn) );
      fc::buffered_istream bstream(stream);
      fc::variant v = fc::json::from_stream( bstream, fc::json::strict_parser );
      load_debug_updates( v.get_object() );
   }*/
}

void plugin::plugin_shutdown() {
   disconnect_signal( my->applied_block_connection );
   /*if( _json_object_stream )
   {
      _json_object_stream->close();
      _json_object_stream.reset();
   }*/
   return;
}

void plugin::plugin_impl::apply_debug_updates() {
    // this was a method on database in Graphene
    auto & db = database();
    golos::chain::block_id_type head_id = db.head_block_id();
    auto it = _debug_updates.find( head_id );
    if( it == _debug_updates.end() ) {
        return;
    }
    //for( const fc::variant_object& update : it->second )
    //   debug_apply_update( db, update, logging );
    for( auto& update : it->second ) {
        update( db );
    }
}

void plugin::plugin_impl::on_applied_block( const protocol::signed_block & b ) {
    try
    {
        if( !_debug_updates.empty() ) {
            apply_debug_updates();
        }
    }
    FC_LOG_AND_RETHROW()
}

void plugin::on_applied_block( const protocol::signed_block & b ) {
    my->on_applied_block(b);
}

debug_generate_blocks_r plugin::plugin_impl::debug_generate_blocks(debug_generate_blocks_a & args) {
    debug_generate_blocks_r ret;
    if( args.count == 0 ) {
        ret.blocks = 0;
        return ret;
    }

    fc::optional<fc::ecc::private_key> debug_private_key;
    golos::chain::public_key_type debug_public_key;

    if( args.debug_key != "" ) {
        debug_private_key = golos::utilities::wif_to_key( args.debug_key );
        FC_ASSERT( debug_private_key.valid() );
        debug_public_key = debug_private_key->get_public_key();
    }
    else {
        if( logging ) {
            elog( "Skipping generation because I don't know the private key");
        }
        ret.blocks = 0;
        return ret;
    }

    golos::chain::database& db = database();
    uint32_t slot = args.miss_blocks+1, produced = 0;
    while( produced < args.count ) {
        uint32_t new_slot = args.miss_blocks+1;
        std::string scheduled_witness_name = db.get_scheduled_witness( slot );
        fc::time_point_sec scheduled_time = db.get_slot_time( slot );
        const golos::chain::witness_object& scheduled_witness = db.get_witness( scheduled_witness_name );
        golos::chain::public_key_type scheduled_key = scheduled_witness.signing_key;
        if( logging ) {
            wlog( "slot: ${sl}   time: ${t}   scheduled key is: ${sk}   dbg key is: ${dk}",
            ("sk", scheduled_key)("dk", debug_public_key)("sl", slot)("t", scheduled_time) );
        }
        if( scheduled_key != debug_public_key ) {
            if( args.edit_if_needed ) {
                if( logging ) {
                    wlog( "Modified key for witness ${w}", ("w", scheduled_witness_name) );
                }
                debug_update( [=]( golos::chain::database& db )
                {
                   db.modify( db.get_witness( scheduled_witness_name ), [&]( golos::chain::witness_object& w )
                   {
                      w.signing_key = debug_public_key;
                   });
                }, args.skip );
            }
            else {
                break;
            }
        }

        db.generate_block( scheduled_time, scheduled_witness_name, *debug_private_key, args.skip );
        ++produced;
        slot = new_slot;
    }

    ret.blocks = produced;
    return ret;
}


debug_generate_blocks_until_r plugin::plugin_impl::debug_generate_blocks_until(debug_generate_blocks_until_a & args) {
    const std::string& debug_key = args.debug_key;
    const fc::time_point_sec& head_block_time = args.head_block_time;
    bool generate_sparsely = args.generate_sparsely;
    uint32_t skip = golos::chain::database::skip_nothing;
    
    golos::chain::database& db = database();

    if( db.head_block_time() >= head_block_time ) {
        return {0};
    }

    uint32_t new_blocks = 0;

    if( generate_sparsely ) {
        debug_generate_blocks_a tmp_args;

        tmp_args.debug_key = debug_key;
        tmp_args.count = 1;
        tmp_args.skip = skip;

        new_blocks += debug_generate_blocks( tmp_args ).blocks;
        auto slots_to_miss = db.get_slot_at_time( head_block_time );
        if( slots_to_miss > 1 ) {
            slots_to_miss--;
            tmp_args.miss_blocks = slots_to_miss;
            new_blocks += debug_generate_blocks( tmp_args ).blocks;
        }
    }
    else {
        while( db.head_block_time() < head_block_time ) {
            debug_generate_blocks_a tmp_args;

            tmp_args.debug_key = debug_key;
            tmp_args.count = 1;

            new_blocks += debug_generate_blocks( tmp_args ).blocks;
        }
    }
    return {new_blocks};
}

debug_push_blocks_r plugin::plugin_impl::debug_push_blocks(debug_push_blocks_a & args) {
    auto& src_filename = args.src_filename;
    auto count = args.count;
    auto skip_validate_invariants = args.skip_validate_invariants;

    if( count == 0 ) {
        return { 0 };
    }

    fc::path src_path = fc::path( src_filename );
    fc::path index_path = fc::path( src_filename + ".index" );

    if( fc::exists(src_path) && fc::exists(index_path) &&
        !fc::is_directory(src_path) && !fc::is_directory(index_path)
    ) {
        ilog( "Loading ${n} from block_log ${fn}", ("n", count)("fn", src_filename) );
        idump( (src_filename)(count)(skip_validate_invariants) );
        golos::chain::block_log log;
        log.open( src_path );
        uint32_t first_block = database().head_block_num()+1;
        uint32_t skip_flags = golos::chain::database::skip_nothing;

        if( skip_validate_invariants ) {
            skip_flags = skip_flags | golos::chain::database::skip_validate_invariants;
        }

        for( uint32_t i=0; i<count; i++ ) {
            //fc::optional< chain::signed_block > block = log.read_block( log.get_block_pos( first_block + i ) );
            uint64_t block_pos = log.get_block_pos( first_block + i );
            if( block_pos == golos::chain::block_log::npos ) {
                wlog( "Block database ${fn} only contained ${i} of ${n} requested blocks", ("i", i)("n", count)("fn", src_filename) );
                return { i };
            }

            decltype( log.read_block(0) ) result;

            try {
                result = log.read_block( block_pos );
            }
            catch( const fc::exception& e ) {
                elog( "Could not read block ${i} of ${n}", ("i", i)("n", count) );
                continue;
            }

            try{
                database().push_block( result.first, skip_flags );
            }
            catch( const fc::exception& e ) {
                elog( "Got exception pushing block ${bn} : ${bid} (${i} of ${n})", ("bn", result.first.block_num())("bid", result.first.id())("i", i)("n", count) );
                elog( "Exception backtrace: ${bt}", ("bt", e.to_detail_string()) );
            }
        }
        ilog( "Completed loading block_database successfully" );
        return { count };
    }
    return { 0 };
}

debug_pop_block_r plugin::plugin_impl::debug_pop_block(debug_pop_block_a & args) {
    auto & db = database();
    return { db.fetch_block_by_number( db.head_block_num() ) };
}

debug_get_witness_schedule_r plugin::plugin_impl::debug_get_witness_schedule(debug_get_witness_schedule_a & args) {
    auto & db = database();
    return db.get( golos::chain::witness_schedule_id_type() );
}

// debug_get_hardfork_property_object_r plugin::plugin_impl::debug_get_hardfork_property_object(debug_get_hardfork_property_object_a & args) {
//     auto & db = database();
//     return db.get( chain::hardfork_property_id_type() );
// }

debug_set_hardfork_r plugin::plugin_impl::debug_set_hardfork(debug_set_hardfork_a & args) {
    if( args.hardfork_id > STEEMIT_NUM_HARDFORKS ) {
        return {};
    }

    database().set_hardfork( args.hardfork_id, false );
    // golos::chain::database::set_hardfork( args.hardfork_id, false );

    return {};
}

debug_has_hardfork_r plugin::plugin_impl::debug_has_hardfork(debug_has_hardfork_a & args) {
    auto & db = database();
    return { db.get( golos::chain::hardfork_property_id_type() ).last_hardfork >= args.hardfork_id };
}


// If you look at debug_generate_blocks_a struct, you will see that there are fields, which have default value
// So may be for some of the field default parameters will be used
// To prevent casting errors there are some arg count checks and manualy filling structure fields

// If you like to test this API, you can just use curl. Smth like that:
// ```
// curl --data '{"jsonrpc": "2.0", "method": "call", "params": ["debug_node","debug_generate_blocks",["5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3",5, 0]] }' 'content-type: text/plain;' http://localhost:8090 | json_reformat
// ```
// You can pass from 1 to 5 params as you wish.
// Just don't forget that it work for testnet only and you have to write this in ur config.ini:
// 
// witness = "cyberfounder"
// private-key = 5JVFFWRLwz6JoP9kguuRFfytToGU6cLgBVTL9t6NB3D3BQLbUBS
// enable-stale-production = true
// required-participation = 0
// 
// 

DEFINE_API ( plugin, debug_generate_blocks ) {
    debug_generate_blocks_a tmp;

    FC_ASSERT(args.args.valid(), "Invalid parameters" ) ;

    auto args_count = args.args->size() ;

    FC_ASSERT( args_count > 0 && args_count < 5, "Wrong parameters number, given ${n}", ("n", args_count) );
    auto args_vector = *(args.args);
    tmp.debug_key = args_vector[0].as_string();
    if (args_count > 1) {
        tmp.count = args_vector[1].as_int64();        
    }
    if (args_count > 2) {
        tmp.skip = args_vector[2].as_int64();
    }
    if (args_count > 3) {
        tmp.miss_blocks = args_vector[3].as_int64();
    }
    if (args_count > 4) {
        tmp.edit_if_needed = args_vector[4].as_bool();
    }

    // This causes a casting error  |
    //                              V
    // auto tmp = args.args->at(0).as<debug_generate_blocks_a>();
    // 
    auto &db = my->database();
    return db.with_read_lock([&]() {
        return my->debug_generate_blocks(tmp);
    });
}

DEFINE_API ( plugin, debug_push_blocks ) {
    debug_push_blocks_a tmp;

    FC_ASSERT(args.args.valid(), "Invalid parameters" ) ;

    auto args_count = args.args->size() ;

    FC_ASSERT( args_count > 0 && args_count < 5, "Wrong parameters number, given ${n}", ("n", args_count) );
    auto args_vector = *(args.args);

    tmp.src_filename = args_vector[0].as_string();
    tmp.count = args_vector[1].as_int64();        
    if (args_count > 2) {
        tmp.skip_validate_invariants = args_vector[3].as_bool();
    }
    // auto tmp = args.args->at(0).as<debug_push_blocks_a>();
    auto &db = my->database();
    return db.with_read_lock([&]() {
        return my->debug_push_blocks(tmp);
    });
}

DEFINE_API ( plugin, debug_generate_blocks_until ) {
    debug_generate_blocks_until_a tmp;

    FC_ASSERT(args.args.valid(), "Invalid parameters" ) ;

    auto args_count = args.args->size() ;

    FC_ASSERT( args_count > 0 && args_count < 5, "Wrong parameters number, given ${n}", ("n", args_count) );
    auto args_vector = *(args.args);

    tmp.debug_key = args_vector[0].as_string();
    tmp.head_block_time = fc::time_point_sec::from_iso_string( args_vector[1].as_string() );      

    if (args_count > 2) {
        tmp.generate_sparsely = args_vector[3].as_bool();
    }
    // auto tmp = args.args->at(0).as<debug_generate_blocks_until_a>();
    auto &db = my->database();
    return db.with_read_lock([&]() {
        return my->debug_generate_blocks_until(tmp);
    });
}

DEFINE_API ( plugin, debug_pop_block ) {
    auto tmp = args.args->at(0).as<debug_pop_block_a>();
    auto &db = my->database();
    return db.with_read_lock([&]() {
        return my->debug_pop_block(tmp);
    });
}

DEFINE_API ( plugin, debug_get_witness_schedule ) {
    auto tmp = args.args->at(0).as<debug_get_witness_schedule_a>();
    auto &db = my->database();
    return db.with_read_lock([&]() {
        return my->debug_get_witness_schedule(tmp);
    });
}

// DEFINE_API ( plugin, debug_get_hardfork_property_object ) {
//     auto tmp = args.args->at(0).as<debug_get_hardfork_property_object_a>();
//     auto &db = my->database();
//     return db.with_read_lock([&]() {
//         return my->debug_get_hardfork_property_object(tmp);
//     });
// }

DEFINE_API ( plugin, debug_set_hardfork ) {
    auto tmp = args.args->at(0).as<debug_set_hardfork_a>();
    auto &db = my->database();
    return db.with_read_lock([&]() {
        return my->debug_set_hardfork(tmp);
    });
}

DEFINE_API ( plugin, debug_has_hardfork ) {
    auto tmp = args.args->at(0).as<debug_has_hardfork_a>();
    auto &db = my->database();
    return db.with_read_lock([&]() {
        return my->debug_has_hardfork(tmp);
    });
}



/*void plugin::save_debug_updates( fc::mutable_variant_object& target )
{
   for( const std::pair< chain::block_id_type, std::vector< fc::variant_object > >& update : _debug_updates )
   {
      fc::variant v;
      fc::to_variant( update.second, v );
      target.set( update.first.str(), v );
   }
}*/

/*void plugin::load_debug_updates( const fc::variant_object& target )
{
   for( auto it=target.begin(); it != target.end(); ++it)
   {
      std::vector< fc::variant_object > o;
      fc::from_variant(it->value(), o);
      _debug_updates[ chain::block_id_type( it->key() ) ] = o;
   }
}*/

/*
void debug_apply_update( chain::database& db, const fc::variant_object& vo, bool logging )
{
   static const uint8_t
      db_action_nil = 0,
      db_action_create = 1,
      db_action_write = 2,
      db_action_update = 3,
      db_action_delete = 4,
      db_action_set_hardfork = 5;
   if( logging ) wlog( "debug_apply_update:  ${o}", ("o", vo) );

   // "_action" : "create"   object must not exist, unspecified fields take defaults
   // "_action" : "write"    object may exist, is replaced entirely, unspecified fields take defaults
   // "_action" : "update"   object must exist, unspecified fields don't change
   // "_action" : "delete"   object must exist, will be deleted

   // if _action is unspecified:
   // - delete if object contains only ID field
   // - otherwise, write

   graphene::db2::generic_id oid;
   uint8_t action = db_action_nil;
   auto it_id = vo.find("id");
   FC_ASSERT( it_id != vo.end() );

   from_variant( it_id->value(), oid );
   action = ( vo.size() == 1 ) ? db_action_delete : db_action_write;

   from_variant( vo["id"], oid );
   if( vo.size() == 1 )
      action = db_action_delete;

   fc::mutable_variant_object mvo( vo );
   mvo( "id", oid._id );
   auto it_action = vo.find("_action" );
   if( it_action != vo.end() )
   {
      const std::string& str_action = it_action->value().get_string();
      if( str_action == "create" )
         action = db_action_create;
      else if( str_action == "write" )
         action = db_action_write;
      else if( str_action == "update" )
         action = db_action_update;
      else if( str_action == "delete" )
         action = db_action_delete;
      else if( str_action == "set_hardfork" )
         action = db_action_set_hardfork;
   }

   switch( action )
   {
      case db_action_create:

         idx.create( [&]( object& obj )
         {
            idx.object_from_variant( vo, obj );
         } );

         FC_ASSERT( false );
         break;
      case db_action_write:
         db.modify( db.get_object( oid ), [&]( graphene::db::object& obj )
         {
            idx.object_default( obj );
            idx.object_from_variant( vo, obj );
         } );
         FC_ASSERT( false );
         break;
      case db_action_update:
         db.modify_variant( oid, mvo );
         break;
      case db_action_delete:
         db.remove_object( oid );
         break;
      case db_action_set_hardfork:
         {
            uint32_t hardfork_id;
            from_variant( vo[ "hardfork_id" ], hardfork_id );
            db.set_hardfork( hardfork_id, false );
         }
         break;
      default:
         FC_ASSERT( false );
   }
}
*/
} } } // golos::plugins::debug_node