#include <steem/plugins/sps/sps_plugin.hpp>
#include <steem/plugins/sps/sps_objects.hpp>
#include <steem/plugins/sps/sps_operations.hpp>

#include <steem/chain/notifications.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/index.hpp>
#include <steem/chain/account_object.hpp>

#include <steem/chain/generic_custom_operation_interpreter.hpp>

namespace steem { namespace plugins { namespace sps {

using namespace steem::chain;
using namespace steem::protocol;

using steem::chain::block_notification;
using steem::chain::generic_custom_operation_interpreter;

namespace detail {

class sps_plugin_impl
{
   public:

      using t_proposals = std::vector< std::reference_wrapper< const proposal_object > >;

   private:

      chain::database& _db;
      sps_plugin& _self;

      boost::signals2::connection _on_proposal_processing;
      std::shared_ptr< generic_custom_operation_interpreter< sps_plugin_operation > > _custom_operation_interpreter;

      bool is_maintenance_period( const time_point_sec& head_time ) const;

      void find_active_proposals( const time_point_sec& head_time, t_proposals& proposals );

      uint64_t calculate_votes( const proposal_id_type& id );
      void calculate_votes( const t_proposals& proposals );

      void sort_by_votes( t_proposals& proposals );

      asset get_treasury_fund();

      asset get_daily_inflation();

      asset calculate_maintenance_budget( const time_point_sec& head_time );

      void transfer_daily_inflation_to_treasury( const asset& daily_inflation );

      void transfer_payments( asset& maintenance_budget_limit, const t_proposals& proposals );

      void update_settings( const time_point_sec& head_time );

      void make_payments( const block_notification& note );

   public:

      sps_plugin_impl( sps_plugin& _plugin ) :
         _db( appbase::app().get_plugin< steem::plugins::chain::chain_plugin >().db() ),
         _self( _plugin ) {}

      ~sps_plugin_impl() {}

      void initialize();

      void on_proposal_processing( const block_notification& note );

      void plugin_shutdown();
};

void sps_plugin_impl::initialize()
{
   _on_proposal_processing = _db.add_on_proposal_processing_handler(
         [&]( const block_notification& note ){ on_proposal_processing( note ); }, _self, 0 );

   // Each plugin needs its own evaluator registry.
   _custom_operation_interpreter = std::make_shared< generic_custom_operation_interpreter< sps_plugin_operation > >( _db, _self.name() );

   // Add each operation evaluator to the registry
   _custom_operation_interpreter->register_evaluator< create_proposal_evaluator >( &_self );
   _custom_operation_interpreter->register_evaluator< update_proposal_votes_evaluator >( &_self );

   // Add the registry to the database so the database can delegate custom ops to the plugin
   _db.register_custom_operation_interpreter( _custom_operation_interpreter );

   add_plugin_index< proposal_index      >( _db );
   add_plugin_index< proposal_vote_index >( _db );
}

bool sps_plugin_impl::is_maintenance_period( const time_point_sec& head_time ) const
{
   return _db.get_dynamic_global_properties().next_maintenance_time >= head_time;
}

void sps_plugin_impl::find_active_proposals( const time_point_sec& head_time, t_proposals& proposals )
{
   const auto& pidx = _db.get_index< proposal_index >().indices().get< by_date >();
   auto it = pidx.begin();

   std::for_each( pidx.begin(), pidx.lower_bound( head_time ), [&]( auto& proposal )
                                             {
                                                if( head_time >= proposal.start_date && head_time <= proposal.end_date )
                                                   proposals.emplace_back( *it );                                                
                                             } );
}

uint64_t sps_plugin_impl::calculate_votes( const proposal_id_type& id )
{
   uint64_t ret = 0;

   const auto& pvidx = _db.get_index< proposal_vote_index >().indices().get< by_proposal_voter >();
   auto found = pvidx.find( id );

   while( found != pvidx.end() && found->proposal_id == id )
   {
      const auto& _voter = _db.get_account( found->voter );
      ret += _voter.vesting_shares.amount.value;

      ++found;
   }

   return ret;
}

void sps_plugin_impl::calculate_votes( const t_proposals& proposals )
{
   for( auto& item : proposals )
   {
      const proposal_object& _item = item;
      auto total_votes = calculate_votes( _item.id );

      _db.modify( _item, [&]( auto& proposal )
                        {
                           proposal.total_votes = total_votes;
                        } );
   }
}

void sps_plugin_impl::sort_by_votes( t_proposals& proposals )
{ 
   std::sort( proposals.begin(), proposals.end(), []( const proposal_object& a, const proposal_object& b )
                                                      {
                                                         return a.total_votes > b.total_votes;
                                                      } );
}

asset sps_plugin_impl::get_treasury_fund()
{
   auto& treasury_account = _db.get_account( STEEM_TREASURY_ACCOUNT );

   return treasury_account.sbd_balance;
}

asset sps_plugin_impl::get_daily_inflation()
{
   //temporary
   return asset( 0, SBD_SYMBOL );
}

asset sps_plugin_impl::calculate_maintenance_budget( const time_point_sec& head_time )
{
   //Get number of microseconds for 1 day( daily_ms )
   static auto daily_us = fc::days(1).count();

   //Get funds from 'treasury' account ( treasury_fund ) 
   asset treasury_fund = get_treasury_fund();

   //Get daily proposal inflation ( daily_proposal_inflation )
   asset daily_inflation = get_daily_inflation();

   FC_ASSERT( treasury_fund.symbol == daily_inflation.symbol, "symbols must be the same" );

   //Calculate budget for given maintenance period
   auto passed_time_us = ( head_time - _db.get_dynamic_global_properties().last_budget_time ).count();

   //Calculate daily_budget_limit
   auto daily_budget_limit = treasury_fund.amount.value / 100 + daily_inflation.amount.value;
   daily_budget_limit = daily_budget_limit * ( passed_time_us / daily_us );

   //Transfer daily_proposal_inflation to `treasury account`
   transfer_daily_inflation_to_treasury( daily_inflation );

   return asset( daily_budget_limit, treasury_fund.symbol );
}

void sps_plugin_impl::transfer_daily_inflation_to_treasury( const asset& daily_inflation )
{
   const auto& treasury_account = _db.get_account( STEEM_TREASURY_ACCOUNT );
   _db.adjust_balance( treasury_account, daily_inflation );
}

void sps_plugin_impl::transfer_payments( asset& maintenance_budget_limit, const t_proposals& proposals )
{
   for( auto& item : proposals )
   {
      const proposal_object& _item = item;
      const auto& treasury_account = _db.get_account( STEEM_TREASURY_ACCOUNT );
      const auto& receiver_account = _db.get_account( _item.receiver );

      if( _item.daily_pay >= maintenance_budget_limit )
      {
         _db.adjust_balance( treasury_account, -maintenance_budget_limit );
         _db.adjust_balance( receiver_account, maintenance_budget_limit );
         break;
      }
      else
      {
         _db.adjust_balance( treasury_account, -_item.daily_pay );
         _db.adjust_balance( receiver_account, _item.daily_pay );
         maintenance_budget_limit -= _item.daily_pay;
      }
   }
}

void sps_plugin_impl::update_settings( const time_point_sec& head_time )
{
   _db.modify( _db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& _dgpo )
                                          {
                                             _dgpo.next_maintenance_time = head_time + fc::seconds( STEEM_PROPOSAL_MAINTENANCE_PERIOD );
                                             _dgpo.last_budget_time = head_time;
                                          } );
}

void sps_plugin_impl::make_payments( const block_notification& note ) 
{
   auto head_time = note.block.timestamp;

   //Check maintenance period
   if( !is_maintenance_period( head_time ) )
      return;

   t_proposals proposals;

   //Find all active proposals, where actual_time >= start_date and actual_time <= end_date
   find_active_proposals( head_time, proposals );

   //Calculate total_votes for every active proposal
   calculate_votes( proposals );

   //Sort all active proposals by total_votes
   sort_by_votes( proposals );

   //Calculate budget for given maintenance period
   asset maintenance_budget_limit = calculate_maintenance_budget( head_time );

   //Execute transfer for every active proposal
   transfer_payments( maintenance_budget_limit, proposals );

   //Set `new maintenance time` and `last budget time`
   update_settings( head_time );
}

void sps_plugin_impl::on_proposal_processing( const block_notification& note )
{
   make_payments( note );
}

void sps_plugin_impl::plugin_shutdown()
{
   chain::util::disconnect_signal( _on_proposal_processing );
}

}; // detail

sps_plugin::sps_plugin() {}

sps_plugin::~sps_plugin() {}

void sps_plugin::set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
   )
{
}

void sps_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   try
   {
      ilog("Intializing sps plugin" );

      my = std::make_unique< detail::sps_plugin_impl >( *this );
      my->initialize();
   }
   FC_CAPTURE_AND_RETHROW()
}

void sps_plugin::plugin_startup() {}

void sps_plugin::plugin_shutdown()
{
   my->plugin_shutdown();
}

} } } // steem::plugins::sps
