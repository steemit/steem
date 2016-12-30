#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <boost/container/flat_map.hpp>

#include <fc/exception/exception.hpp>
#include <fc/io/json.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/signals.hpp>

#define CREATE_ENODE( tree, Data, Name ) \
   enode_impl< Data > Name ## _impl;     \
   Data& Name = Name ## _impl.data;      \
   (tree).push_enode( Name ## _impl );

#define CREATE_ANON_ENODE( tree, Data )  \
   enode_impl< Data > enode_ ## __LINE__ ## _impl;  \
   (tree).push_enode( enode_ ## __LINE__ ## _impl );

namespace steemit { namespace chain {

class enode_tree;
class enode_type;

class enode
{
   public:
      typedef int64_t id_type;

      enode();
      virtual ~enode();

      template< typename Data >
      Data* maybe_as();

      template< typename Data >
      const Data* maybe_as()const;

      template< typename Data >
      Data& as()
      {
         Data* p = maybe_as< Data >();
         FC_ASSERT( p != nullptr );
         return *p;
      }

      template< typename Data >
      const Data& as()const
      {
         const Data* p = maybe_as< Data >();
         FC_ASSERT( p != nullptr );
         return *p;
      }

      template< typename Data >
      bool is()const
      {
         return maybe_as< Data >() != nullptr;
      }

      enode_tree*     tree  = nullptr;
      id_type         enid  = 0;
      id_type         enpid = 0;

      enode_type*     type  = nullptr;
};

template< typename Data >
class enode_impl : public enode
{
   public:
      enode_impl() {}
      virtual ~enode_impl();

      Data data;
};

class enode_type
{
   public:
      enode_type();
      virtual ~enode_type();

      virtual void                to_variant( const enode& node, fc::variant& var )const = 0;
      virtual std::string         get_name()const = 0;

      template< typename EnodeType >
      bool is_type();
};

template< typename Data >
class enode_type_impl : public enode_type
{
   private:
      enode_type_impl( std::string name ) : _name(name) {}
      virtual ~enode_type_impl() {}

   public:
      static enode_type_impl<Data>* instance()
      {
         // The compiler tries to force us to declare inst as "constexpr" if we put it in the class
         // body, but we can outsmart it by putting it in this method instead.
         static enode_type_impl<Data>* inst = nullptr;
         if( inst == nullptr )
            inst = new enode_type_impl<Data>( fc::get_typename< Data >::name() );
         return inst;
      }

      virtual void to_variant( const enode& node, fc::variant& var )const override;

      virtual std::string get_name()const override
      {
         return _name;
      }

      std::string _name;
};

template< typename Data >
bool enode_type::is_type()
{
   return (dynamic_cast< enode_type_impl<Data>* >(this) != nullptr);
}

template< typename Data >
Data* enode::maybe_as()
{
   enode_impl<Data>* p = dynamic_cast< enode_impl<Data>* >(this);
   if( p == nullptr )
      return nullptr;
   return &(p->data);
}

template< typename Data >
const Data* enode::maybe_as()const
{
   const enode_impl<Data>* p = dynamic_cast< const enode_impl<Data>* >(this);
   if( p == nullptr )
      return nullptr;
   return &(p->data);
}

class enode_tree
{
   public:
      enode_tree();
      virtual ~enode_tree();

      void clear()
      {
         _nodes.clear();
         _path.clear();
      }

      template< typename Data >
      void push_enode( enode_impl< Data >& node )
      {
         node.tree = this;
         node.type = enode_type_impl< Data >::instance();
         node.enid = _nodes.size();
         if( _path.size() == 0 )
            node.enpid = -1;
         else
            node.enpid = _path.back();
         _nodes.push_back( &node );
         _path.push_back( node.enid );
         if( on_push_enode != nullptr )
            (*on_push_enode)(node);
      }

      void pop_enode( enode& node )
      {
         FC_ASSERT( _nodes.size() > 0 );
         FC_ASSERT( _path.size() > 0 );
         FC_ASSERT( _nodes[_path.back()] == &node );

         if( on_pop_enode != nullptr )
            (*on_pop_enode)(node);
         _nodes.back() = nullptr;
         _path.pop_back();
         node.tree = nullptr;
      }

      template< typename Data >
      enode_impl<Data>* find_enode_impl()
      {
         auto it = _path.end();
         while( it != _path.begin() )
         {
            --it;
            enode_impl< Data >* p = dynamic_cast< enode_impl<Data>* >( _nodes[*it] );
            if( p != nullptr )
               return p;
         }
         return nullptr;
      }

      template< typename Data >
      enode* find_enode()
      {
         return find_enode_impl<Data>();
      }

      template< typename Data >
      enode& get_enode()
      {
         Data* result = find_enode<Data>();
         FC_ASSERT( result != nullptr );
         return *result;
      }

      template< typename Data >
      Data* find_enode_as()
      {
         enode_impl< Data >* p = find_enode_impl< Data >();
         if( p == nullptr )
            return nullptr;
         return &(p->data);
      }

      template< typename Data >
      Data& get_enode_as()
      {
         enode_impl< Data >* p = find_enode_impl<Data>();
         FC_ASSERT( p != nullptr );
         return p->data;
      }

      std::vector< enode* >         _nodes;
      std::vector< enode::id_type > _path;

      fc::signal<void( const enode& )>* on_push_enode = nullptr;
      fc::signal<void( const enode& )>* on_pop_enode = nullptr;
};

class enode_type_registry
{
   public:
      enode_type_registry() {}
      virtual ~enode_type_registry() {}

      template< typename Data >
      void register_type()
      {
         enode_type_impl< Data >* t = enode_type_impl< Data >::instance();
         _registry.emplace( t->get_name(), t );
      }

      boost::container::flat_map< std::string, enode_type* >      _registry;
};

} }

FC_REFLECT( steemit::chain::enode, (enid)(enpid)(type) )

namespace fc {

void to_variant( steemit::chain::enode_type* const& t, variant& var );
void from_variant( const variant& var, steemit::chain::enode_type*& t );

template< typename Data >
void to_variant( const steemit::chain::enode_impl<Data>& node, variant& var )
{
   fc::variant tmp;
   fc::to_variant( node.data, tmp );
   mutable_variant_object mvo( tmp.get_object() );
   mvo("enid" , node.enid)
      ("enpid", node.enpid)
      ("type" , node.type)
      ;
   var = mvo;
}

template< typename Data >
void from_variant( const variant& var, steemit::chain::enode_impl<Data>& node )
{
   steemit::chain::enode& node_base = node;
   fc::from_variant( var, node_base );
   fc::from_variant( var, node.data );
}

}

namespace steemit { namespace chain {

template< typename Data >
enode_impl<Data>::~enode_impl()
{
   if( tree != nullptr )
      tree->pop_enode(*this);
}

template< typename Data >
void enode_type_impl<Data>::to_variant( const enode& node, fc::variant& var )const
{
   const enode_impl< Data >* node2 = dynamic_cast< const enode_impl< Data >* >( &node );
   FC_ASSERT( node2 != nullptr );
   fc::to_variant( *node2, var );
}

} }

