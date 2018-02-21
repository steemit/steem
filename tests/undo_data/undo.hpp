#pragma once

#include <steem/chain/database.hpp>

namespace steem { namespace chain {

   struct u_types
   {
      using gen_type = uint32_t;
   };

   template< typename T >
   class abstract_data_generator
   {
      public:

         using p_self = std::shared_ptr< abstract_data_generator >;
         using items = std::shared_ptr< std::vector< T > >;

         virtual items get( u_types::gen_type size ) = 0;
   };

   template< typename T >
   class data_generator: public abstract_data_generator< T >
   {
      public:

         using items = typename abstract_data_generator< T >::items;

      private:

         static uint64_t cnt;

         template< typename CALL >
         items generate( u_types::gen_type size, CALL call );

         items generate( std::false_type, std::false_type, u_types::gen_type size );
         items generate( std::true_type, std::false_type, u_types::gen_type size );
         items generate( std::false_type, std::true_type, u_types::gen_type size );
         items generate( u_types::gen_type size );

      protected:
      public:

         data_generator();
         virtual ~data_generator();

         virtual items get( u_types::gen_type size ) override;
   };

   class generators_owner
   {
      public:

         using p_self = std::shared_ptr< generators_owner >;

      private:

         abstract_data_generator< uint32_t >::p_self uint32_gen;
         abstract_data_generator< std::string >::p_self string_gen;

      protected:
      public:

         generators_owner();
         virtual ~generators_owner();

         data_generator< uint32_t >::items get_uint32( u_types::gen_type size );
         data_generator< std::string >::items get_strings( u_types::gen_type size );
   };

   class abstract_wrapper
   {
      public:

         using p_self = std::shared_ptr< abstract_wrapper >;

      protected:
      
         const generators_owner::p_self& gen;

         virtual bool equal( const abstract_wrapper& obj ) const = 0;

      public:

         abstract_wrapper( const generators_owner::p_self& _gen );
         virtual ~abstract_wrapper();

         virtual void create( chain::database& db ) = 0;
         virtual void modify( chain::database& db ) = 0;
         virtual void remove( chain::database& db ) = 0;

         bool operator==( const abstract_wrapper& obj ) const;
   };

   class account_object_wrapper: public abstract_wrapper
   {
      private:

         std::string name = "";
         std::string json_metadata = "";

         uint32_t comment_count = 0;
         uint32_t lifetime_vote_count = 0;

         void fill();

      protected:

         virtual bool equal( const abstract_wrapper& obj ) const override;

      public:

         account_object_wrapper( const generators_owner::p_self& _gen );

         account_object_wrapper( const generators_owner::p_self& _gen,
                                 const std::string& _name,
                                 const std::string& _json_metadata,
                                 uint32_t _comment_count,
                                 uint32_t _lifetime_vote_count );

         virtual ~account_object_wrapper();

         virtual void create( chain::database& db ) override;
         virtual void modify( chain::database& db ) override;
         virtual void remove( chain::database& db ) override;

         bool operator==( const account_object_wrapper& obj ) const;
   };

   class undo_operation
   {
      public:

         using p_self = std::shared_ptr< undo_operation >;

      protected:

         abstract_wrapper::p_self item;
         chain::database& db;

      public:

         undo_operation( abstract_wrapper::p_self _item, chain::database& _db );
         virtual ~undo_operation();

         void create();
         void modify();
         void remove();
   };

   class event
   {
      private:

         std::string chain;
         uint32_t idx = 0;

      public:

         event( const std::string& _chain ) : chain( _chain ) {}

         void next() { ++idx; };
         bool completed() const { return idx >= chain.size(); }

         char get() const
         {
            assert( !completed() );
            return chain[ idx ];
         }
   };

   class abstract_scenario
   {
      protected:

         enum ops : char { nothing = '.', create = 'c', modify = 'm', remove = 'r' };

         using t_events = std::list< event >;
         using t_operations = std::list< undo_operation::p_self >;
         using t_old_values = std::list< abstract_wrapper::p_self >;

      private:

         database::session* session = nullptr;

         t_events events;
         t_operations operations;

         bool undo_session();
         bool check_input( const std::string& str );

         bool before_test();
         bool test_impl( bool& is_alive_chain );
         bool test();
         bool after_test();

      protected:

         t_old_values old_values;

         chain::database& db;

         virtual bool remember_old_values() = 0;
         virtual undo_operation::p_self create_operation() = 0;
         virtual bool check_result( const t_old_values& old ) = 0;

      public:

         abstract_scenario( chain::database& _db );
         virtual ~abstract_scenario();

         bool add_operations( const std::string& chain );
         bool run();
   };

   class account_object_scenario: public abstract_scenario
   {
      private:

         generators_owner::p_self gen;

      protected:

         virtual bool remember_old_values();
         virtual undo_operation::p_self create_operation();
         virtual bool check_result( const t_old_values& old );

      public:

         account_object_scenario( chain::database& _db );
         virtual ~account_object_scenario();
   };

} }
