#include <steemit/chain/steem_objects.hpp>
#include <steemit/chain/database.hpp>
#include <steemit/chain/hardfork.hpp>

#include <fc/uint128.hpp>

namespace steemit { namespace chain {

set< string > account_member_index::get_account_members( const account_object& a ) const
{
   set< string > result;
   for( auto auth : a.owner.account_auths )
      result.insert( auth.first );
   for( auto auth : a.active.account_auths )
      result.insert( auth.first );
   return result;
}

set< public_key_type > account_member_index::get_key_members( const account_object& a ) const
{
   set< public_key_type > result;
   for( auto auth : a.owner.key_auths )
      result.insert( auth.first );
   for( auto auth : a.active.key_auths )
      result.insert( auth.first );
   result.insert( a.memo_key );
   return result;
}

void account_member_index::object_inserted( const object& obj )
{
    assert( dynamic_cast< const account_object* >( &obj ) ); // for debug only
    const account_object& a = static_cast< const account_object& >( obj );

    auto account_members = get_account_members( a );
    for( auto item : account_members )
       account_to_account_memberships[item].insert( a.name );

    auto key_members = get_key_members( a );
    for( auto item : key_members )
       account_to_key_memberships[item].insert( a.name );

}

void account_member_index::object_removed( const object& obj )
{
    assert( dynamic_cast<const account_object*>( &obj ) ); // for debug only
    const account_object& a = static_cast< const account_object& >( obj );

    auto key_members = get_key_members( a );
    for( auto item : key_members )
       account_to_key_memberships[item].erase( a.name );


    auto account_members = get_account_members( a );
    for( auto item : account_members )
       account_to_account_memberships[item].erase( a.name );
}

void account_member_index::about_to_modify( const object& before )
{
   before_key_members.clear();
   before_account_members.clear();
   assert( dynamic_cast< const account_object* >( &before ) ); // for debug only
   const account_object& a = static_cast< const account_object& >( before );
   before_key_members     = get_key_members( a );
   before_account_members = get_account_members( a );
}

void account_member_index::object_modified( const object& after )
{
    assert( dynamic_cast< const account_object* >( &after ) ); // for debug only
    const account_object& a = static_cast< const account_object& >( after );

    {
       set< string > after_account_members = get_account_members( a );
       vector< string > removed;
       removed.reserve( before_account_members.size() );
       std::set_difference( before_account_members.begin(), before_account_members.end(),
                            after_account_members.begin(), after_account_members.end(),
                            std::inserter(removed, removed.end() ) );

       for( auto itr = removed.begin(); itr != removed.end(); ++itr )
          account_to_account_memberships[*itr].erase( a.name );

       vector<string> added;
       added.reserve( after_account_members.size() );
       std::set_difference( after_account_members.begin(), after_account_members.end(),
                            before_account_members.begin(), before_account_members.end(),
                            std::inserter(added, added.end() ) );

       for( auto itr = added.begin(); itr != added.end(); ++itr )
          account_to_account_memberships[*itr].insert( a.name );
    }


    {
       set<public_key_type> after_key_members = get_key_members( a );

       vector<public_key_type> removed;
       removed.reserve( before_key_members.size() );
       std::set_difference( before_key_members.begin(), before_key_members.end(),
                            after_key_members.begin(), after_key_members.end(),
                            std::inserter(removed, removed.end() ) );

       for( auto itr = removed.begin(); itr != removed.end(); ++itr )
          account_to_key_memberships[*itr].erase( a.name );

       vector<public_key_type> added;
       added.reserve(after_key_members.size());
       std::set_difference( after_key_members.begin(), after_key_members.end(),
                            before_key_members.begin(), before_key_members.end(),
                            std::inserter(added, added.end() ) );

       for( auto itr = added.begin(); itr != added.end(); ++itr )
          account_to_key_memberships[*itr].insert( a.name );
    }
}

} } // steemit::chain
