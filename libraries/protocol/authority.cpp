#include <steemit/protocol/authority.hpp>

namespace steemit { namespace protocol {

// authority methods
void authority::add_authority( const public_key_type& k, weight_type w )
{
   key_auths[k] = w;
}

void authority::add_authority( const account_name_type& k, weight_type w )
{
   account_auths[k] = w;
}

vector< public_key_type > authority::get_keys()const
{
   vector< public_key_type > result;
   result.reserve( key_auths.size() );
   for( const auto& k : key_auths )
      result.push_back(k.first);
   return result;
}

bool authority::is_impossible()const
{
   uint64_t auth_weights = 0;
   for( const auto& item : account_auths ) auth_weights += item.second;
   for( const auto& item : key_auths ) auth_weights += item.second;
   return auth_weights < weight_threshold;
}

uint32_t authority::num_auths()const { return account_auths.size() + key_auths.size(); }

void authority::clear() { account_auths.clear(); key_auths.clear(); }

void authority::validate()const
{
   for( const auto& item : account_auths )
   {
      FC_ASSERT( is_valid_account_name( item.first ) );
   }
}


bool is_valid_account_name( const string& name )
{
#if STEEMIT_MIN_ACCOUNT_NAME_LENGTH < 3
#error This is_valid_account_name implementation implicitly enforces minimum name length of 3.
#endif

   const size_t len = name.size();
   if( len < STEEMIT_MIN_ACCOUNT_NAME_LENGTH )
      return false;

   if( len > STEEMIT_MAX_ACCOUNT_NAME_LENGTH )
      return false;

   size_t begin = 0;
   while( true )
   {
      size_t end = name.find_first_of( '.', begin );
      if( end == std::string::npos )
         end = len;
      if( end - begin < 3 )
         return false;
      switch( name[begin] )
      {
         case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
         case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
         case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
         case 'y': case 'z':
            break;
         default:
            return false;
      }
      switch( name[end-1] )
      {
         case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
         case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
         case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
         case 'y': case 'z':
         case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
         case '8': case '9':
            break;
         default:
            return false;
      }
      for( size_t i=begin+1; i<end-1; i++ )
      {
         switch( name[i] )
         {
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
            case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
            case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
            case 'y': case 'z':
            case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
            case '8': case '9':
            case '-':
               break;
            default:
               return false;
         }
      }
      if( end == len )
         break;
      begin = end+1;
   }
   return true;
}

bool operator == ( const authority& a, const authority& b )
{
   return ( a.weight_threshold == b.weight_threshold ) &&
            ( a.account_auths  == b.account_auths )    &&
            ( a.key_auths      == b.key_auths );
}

} } // steemit::protocol
