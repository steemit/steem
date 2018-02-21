
#include "undo.hpp"
#include <steem/chain/steem_objects.hpp>

namespace steem { namespace chain {

template< typename T >
uint64_t data_generator< T >::cnt = 0;

template< typename T >
data_generator< T >::data_generator()
{

}

template< typename T >
data_generator< T >::~data_generator()
{

}

template< typename T >
typename data_generator< T >::items data_generator< T >::generate( std::false_type, std::false_type, u_types::gen_type size )
{
   static items empty;
   return empty;
}

template< typename T >
template< typename CALL >
typename data_generator< T >::items data_generator< T >::generate( u_types::gen_type size, CALL call )
{
   items ret( new std::vector< T >() );

   ret->resize( size );

   std::for_each( ret->begin(), ret->end(), call );

   return ret;
}

template< typename T >
typename data_generator< T >::items data_generator< T >::generate( std::true_type, std::false_type, u_types::gen_type size )
{
   assert( std::is_integral< T >::value );

   return generate( size, [&]( T& val ){ val = cnt++; } );
}

template< typename T >
typename data_generator< T >::items data_generator< T >::generate( std::false_type, std::true_type, u_types::gen_type size )
{
   static_assert( std::is_same< T, std::string >::value, " T != std::string " );

   return generate( size, [&]( T& val ){ val = "s"; val += std::to_string( cnt++ ); } );
}

template< typename T >
typename data_generator< T >::items data_generator< T >::generate( u_types::gen_type size )
{
   using val = std::integral_constant< bool, std::is_integral< T >::value >;
   using val2 = std::integral_constant< bool, std::is_same< T, std::string >::value >;

   return generate( val(), val2(), size );
}

template< typename T >
typename data_generator< T >::items data_generator< T >::get( u_types::gen_type size )
{
   return generate( size );
}

generators_owner::generators_owner()
{
   uint32_gen = abstract_data_generator< uint32_t >::p_self( new data_generator< uint32_t >() );
   string_gen = abstract_data_generator< std::string >::p_self( new data_generator< std::string >() );
}

generators_owner::~generators_owner()
{

}

data_generator< uint32_t >::items generators_owner::get_uint32( u_types::gen_type size )
{
   assert( uint32_gen );
   return uint32_gen->get( size );
}

data_generator< std::string >::items generators_owner::get_strings( u_types::gen_type size )
{
   assert( string_gen );
   return string_gen->get( size );
}

abstract_wrapper::abstract_wrapper( const generators_owner::p_self& _gen )
               : gen( _gen )
{

}

abstract_wrapper::~abstract_wrapper()
{

}

bool abstract_wrapper::operator==( const abstract_wrapper& obj ) const
{
   return equal( obj );
}

account_object_wrapper::account_object_wrapper( const generators_owner::p_self& _gen )
                     : abstract_wrapper( _gen )
{
   fill();
}

account_object_wrapper::account_object_wrapper( const generators_owner::p_self& _gen, const std::string& _name, const std::string& _json_metadata, uint32_t _comment_count, uint32_t _lifetime_vote_count )
                     : abstract_wrapper( _gen ), 
                     name( _name ), json_metadata( _json_metadata ), comment_count( _comment_count ), lifetime_vote_count( _lifetime_vote_count )
{
}

account_object_wrapper::~account_object_wrapper()
{

}

void account_object_wrapper::fill()
{
   data_generator< uint32_t >::items values = gen->get_uint32( 2 );
   data_generator< std::string >::items names = gen->get_strings( 2 );

   name = names->at( 0 );
   json_metadata = names->at( 1 );

   comment_count = values->at( 0 );
   lifetime_vote_count = values->at( 1 );
}

bool account_object_wrapper::equal( const abstract_wrapper& obj ) const
{
   const account_object_wrapper* other = dynamic_cast< const account_object_wrapper* >( &obj );
   return other && ( *this ) == ( *other );
}

void account_object_wrapper::create( chain::database& db )
{
   db.create< account_object >( [&]( account_object& obj )
   {
      obj.name = name;
      obj.json_metadata = json_metadata.c_str();
      obj.comment_count = comment_count;
      obj.lifetime_vote_count = lifetime_vote_count;
   } );
}

void account_object_wrapper::modify( chain::database& db )
{
   account_object_wrapper new_obj( gen );
   const auto& old_obj = db.get_account( name );

   db.modify( old_obj, [&]( account_object& obj )
   {
      obj.name = new_obj.name;
      obj.json_metadata = new_obj.json_metadata.c_str();
      obj.comment_count = new_obj.comment_count;
      obj.lifetime_vote_count = new_obj.lifetime_vote_count;
   } );
}

void account_object_wrapper::remove( chain::database& db )
{
   const auto& found = db.get_account( name );
   db.remove( found );
}

bool account_object_wrapper::operator==( const account_object_wrapper& obj ) const
{
   return name == obj.name &&
          json_metadata == obj.json_metadata &&
          comment_count == obj.comment_count &&
          lifetime_vote_count == obj.lifetime_vote_count;
}

undo_operation::undo_operation( abstract_wrapper::p_self _item, chain::database& _db )
                  : item( _item ), db( _db )
{
}

undo_operation::~undo_operation()
{
}

void undo_operation::create()
{
   assert( item );
   item->create( db );
}

void undo_operation::modify()
{
   assert( item );
   item->modify( db );
}

void undo_operation::remove()
{
   assert( item );
   item->remove( db );
}

abstract_scenario::abstract_scenario( chain::database& _db )
            : db( _db )
{
}

abstract_scenario::~abstract_scenario()
{

}

bool abstract_scenario::undo_session()
{
   try
   {
      if( session )
         session->undo();

      db.undo();

      if( session )
         delete session;
   }
   FC_LOG_AND_RETHROW()

   return true;
}

bool abstract_scenario::check_input( const std::string& str )
{
   for( auto& item : str )
      switch( item )
      {
         case nothing: case create: case modify: case remove: break;

         default:
            assert( "Unknown value in undo operations." );
            return false;
      }

   return true;
}

bool abstract_scenario::before_test()
{
   try
   {
      if( !undo_session() )
         return false;

      session = new database::session( db.start_undo_session( true ) );

      for( uint32_t i = 0; i < events.size(); ++i )
         operations.push_back( create_operation() );

      remember_old_values();
   }
   FC_LOG_AND_RETHROW()

   return true;
}

bool abstract_scenario::test_impl( bool& is_alive_chain )
{
   try
   {
      is_alive_chain = false;

      assert( events.size() == operations.size() );

      auto op_it = operations.begin();

      for( auto& item : events )
      {
         if( item.completed() )
         {
            ++op_it;
            continue;
         }

         is_alive_chain = true;

         switch( item.get() )
         {
            case nothing: break;

            case create:
               ( *op_it )->create();
               break;

            case modify:
               ( *op_it )->modify();
               break;

            case remove:
               ( *op_it )->remove();
               break;

            default:
               assert( 0 && "Unknown value in undo operations." );
               return false;
         }

         item.next();

         ++op_it;
      }
   }
   FC_LOG_AND_RETHROW()
   return true;
}

bool abstract_scenario::test()
{
   try
   {
      bool is_alive_chain = true;

      while( is_alive_chain )
      {
         if( !test_impl( is_alive_chain ) )
            return false;
      }
   }
   FC_LOG_AND_RETHROW()

   return true;
}

bool abstract_scenario::after_test()
{
   try
   {
      if( !undo_session() )
         return false;

      if( !check_result( old_values ) )
         return false;
   }
   FC_LOG_AND_RETHROW()

   return true;
}

bool abstract_scenario::add_operations( const std::string& chain )
{
   if( !check_input( chain ) )
      return false;

   events.emplace_back( std::move( chain ) );

   return true;
}

bool abstract_scenario::run()
{
   try
   {
      if( !before_test() )
         return false;

      if( !test() )
         return false;

      if( !after_test() )
         return false;
   }
   FC_LOG_AND_RETHROW()

   return true;
}

account_object_scenario::account_object_scenario( chain::database& _db )
                     : abstract_scenario( _db ), gen( new generators_owner() )
{
}

account_object_scenario::~account_object_scenario()
{

}

bool account_object_scenario::remember_old_values()
{
   old_values.clear();

   const auto& idx = db.get_index< account_index >().indices().get< by_id >();

   auto it = idx.begin();
   auto it_end = idx.end();

   while( it != it_end )
   {
      old_values.push_back( abstract_wrapper::p_self
                     (
                        new account_object_wrapper(
                           gen,
                           std::string( it->name ),
                           it->json_metadata.c_str(),
                           it->comment_count,
                           it->lifetime_vote_count )
                     ) );
      ++it;
   }

   return true;
}

undo_operation::p_self account_object_scenario::create_operation()
{
   abstract_wrapper::p_self obj( new account_object_wrapper( gen ) );

   return undo_operation::p_self( new undo_operation( obj, db ) );
}

bool account_object_scenario::check_result( const t_old_values& old )
{
   const auto& idx = db.get_index< account_index >().indices().get< by_id >();

   uint32_t idx_size = idx.size();
   uint32_t old_size = old.size();
   if( idx_size != old_size )
      return false;

   auto it = idx.begin();
   auto it_end = idx.end();

   auto it_old = old.begin();

   while( it != it_end )
   {
      abstract_wrapper::p_self _old = *it_old;
      account_object_wrapper actual( gen, std::string( it->name ), it->json_metadata.c_str(), it->comment_count, it->lifetime_vote_count );

      if( !( ( *_old ) == actual ) )
         return false;

      ++it;
      ++it_old;
   }
   return true;
}

}}//steem::chain

