
#include <steemit/chain/protocol/authority.hpp>

namespace steemit { namespace chain {

void add_authority_accounts(
   flat_set<string>& result,
   const authority& a
   )
{
   for( auto& item : a.account_auths )
      result.insert( item.first );
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

void authority::validate()const
{
   for( auto item : account_auths )
      FC_ASSERT( is_valid_account_name( item.first ) );
}

} } // steemit::chain
