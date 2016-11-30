#include <steemit/chain/shared_authority.hpp>

namespace steemit { namespace chain {

shared_authority::operator authority()const
{
   authority result;

   result.account_auths.reserve( account_auths.size() );
   for( const auto& item : account_auths )
      result.account_auths.insert( item );

   result.key_auths.reserve( key_auths.size() );
   for( const auto& item : key_auths )
      result.key_auths.insert( item );

   result.weight_threshold = weight_threshold;

   return result;
}

shared_authority& shared_authority::operator=( const authority& a )
{
   clear();

   for( const auto& item : a.account_auths )
      account_auths.insert( item );

   for( const auto& item : a.key_auths )
      key_auths.insert( item );

   weight_threshold = a.weight_threshold;

   return *this;
}

void shared_authority::add_authority( const public_key_type& k, weight_type w )
{
   key_auths[k] = w;
}

void shared_authority::add_authority( const account_name_type& k, weight_type w )
{
   account_auths[k] = w;
}

vector<public_key_type> shared_authority::get_keys()const
{
   vector<public_key_type> result;
   result.reserve( key_auths.size() );
   for( const auto& k : key_auths )
      result.push_back(k.first);
   return result;
}

bool shared_authority::is_impossible()const
{
   uint64_t auth_weights = 0;
   for( const auto& item : account_auths ) auth_weights += item.second;
   for( const auto& item : key_auths ) auth_weights += item.second;
   return auth_weights < weight_threshold;
}

uint32_t shared_authority::num_auths()const { return account_auths.size() + key_auths.size(); }

void shared_authority::clear() { account_auths.clear(); key_auths.clear(); }

void shared_authority::validate()const
{
   for( const auto& item : account_auths )
   {
      FC_ASSERT( protocol::is_valid_account_name( item.first ) );
   }
}

bool operator == ( const shared_authority& a, const shared_authority& b )
{
   return ( a.weight_threshold == b.weight_threshold ) &&
            ( a.account_auths  == b.account_auths )    &&
            ( a.key_auths      == b.key_auths );
}

bool operator == ( const authority& a, const shared_authority& b )
{
   return a == authority( b );
}

bool operator == ( const shared_authority& a, const authority& b )
{
   return authority( a ) == b;
}

} } // steemit::chain
