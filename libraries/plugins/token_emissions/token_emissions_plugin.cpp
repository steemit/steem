#include <steem/chain/steem_fwd.hpp>

#include <steem/plugins/token_emissions/token_emissions_plugin.hpp>
#include <steem/plugins/token_emissions/token_emissions_objects.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/index.hpp>
#include <steem/chain/util/smt_token.hpp>
#include <steem/protocol/config.hpp>

#include <fc/io/json.hpp>

namespace steem { namespace plugins { namespace token_emissions {

namespace detail {

class token_emissions_impl
{
public:
   token_emissions_impl() : _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ) {}
   virtual ~token_emissions_impl() {}

   void on_post_apply_required_action( const required_action_notification& note );
   void on_post_apply_optional_action( const optional_action_notification& note );
   void on_post_apply_block( const block_notification& note );

   chain::database&              _db;
   boost::signals2::connection   post_apply_required_action_connection;
   boost::signals2::connection   post_apply_optional_action_connection;
   boost::signals2::connection   post_apply_block_connection;
};

void token_emissions_impl::on_post_apply_required_action( const required_action_notification& note )
{
   if ( _db.is_pending_tx() )
      return;

   if ( note.action.which() != required_automated_action::tag< smt_token_launch_action >::value )
      return;

   smt_token_launch_action token_launch = note.action.get< smt_token_launch_action >();

   auto next_emission = util::smt::next_emission_time( _db, token_launch.symbol );

   if ( next_emission )
   {
      _db.create< token_emissions_object >( [ token_launch, next_emission ]( token_emissions_object& o )
      {
         o.symbol        = token_launch.symbol;
         o.next_emission = *next_emission;
      } );
   }
}

void token_emissions_impl::on_post_apply_optional_action( const optional_action_notification& note )
{
   if ( _db.is_pending_tx() )
      return;

   if ( note.action.which() != optional_automated_action::tag< smt_token_emission_action >::value )
      return;

   smt_token_emission_action emission_action = note.action.get< smt_token_emission_action >();

   auto now  = _db.head_block_time();
   auto next = util::smt::next_emission_time( _db, emission_action.symbol, now );

   const auto& emission_obj = _db.get< token_emissions_object, by_symbol >( emission_action.symbol );
   if ( next )
   {
      _db.modify( emission_obj, [ now, next ]( token_emissions_object& o )
      {
         o.last_emission = now;
         o.next_emission = *next;
      } );
   }
   else
   {
      _db.remove( emission_obj );
   }
}

void token_emissions_impl::on_post_apply_block( const block_notification& note )
{
   // We're creating token emissions for the upcoming block.
   time_point_sec next_emission_time = note.block.timestamp + STEEM_BLOCK_INTERVAL;

   const auto& idx = _db.get_index< token_emissions_index, by_next_emission_symbol >();

   for ( auto itr = idx.begin(); itr != idx.end() && itr->next_emission <= next_emission_time; ++itr )
   {
      // By retrieving emissions between the last and the upcoming, we can "catch up" if necessary
      auto emissions = util::smt::emissions_in_range( _db, itr->symbol, itr->last_emission, next_emission_time );

      for ( auto& emission : emissions )
      {
         time_point_sec emission_time                       = emission.first;
         const smt_token_emissions_object* emissions_object = emission.second;

         FC_UNUSED( emission_time );
         FC_UNUSED( emissions_object );

         // Generate smt_token_emission here
      }
   }
}

} // detail

token_emissions_plugin::token_emissions_plugin() {}
token_emissions_plugin::~token_emissions_plugin() {}

void token_emissions_plugin::set_program_options( boost::program_options::options_description& cli, boost::program_options::options_description& cfg )
{
}

void token_emissions_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   STEEM_ADD_PLUGIN_INDEX(my->_db, token_emissions_index);

   my->post_apply_optional_action_connection = my->_db.add_post_apply_optional_action_handler(
      [&]( const optional_action_notification& note )
      {
         try
         {
            my->on_post_apply_optional_action( note );
         } FC_LOG_AND_RETHROW()
      }, *this, 0 );

   my->post_apply_block_connection = my->_db.add_post_apply_block_handler(
      [&]( const block_notification& note )
      {
         try
         {
            my->on_post_apply_block( note );
         } FC_LOG_AND_RETHROW()
      }, *this, 0 );

   my->post_apply_required_action_connection = my->_db.add_post_apply_required_action_handler(
      [&]( const required_action_notification& note )
      {
         try
         {
            my->on_post_apply_required_action( note );
         } FC_LOG_AND_RETHROW()
      }, *this, 0 );
}

void token_emissions_plugin::plugin_startup()
{

}

void token_emissions_plugin::plugin_shutdown()
{
   chain::util::disconnect_signal( my->post_apply_required_action_connection );
   chain::util::disconnect_signal( my->post_apply_optional_action_connection );
   chain::util::disconnect_signal( my->post_apply_block_connection );
}

} } } // steem::plugins::token_emissions

