
#include <steem/protocol/operations.hpp>

#include <fc/io/json.hpp>

#include <boost/algorithm/string.hpp>

using namespace steem::protocol;
using boost::container::flat_map;
using std::string;
using std::vector;

struct witness_properties
{
   // Chain properties
   fc::optional< asset >               account_creation_fee;
   fc::optional< uint32_t >            maximum_block_size;
   fc::optional< uint16_t >            sbd_interest_rate;
   fc::optional< int32_t >             account_subsidy_budget;
   fc::optional< uint32_t >            account_subsidy_decay;

   // Per-witness fields
   fc::optional< public_key_type >     key;
   fc::optional< public_key_type >     new_signing_key;
   fc::optional< price >               sbd_exchange_rate;
   fc::optional< std::string >         url;
};

FC_REFLECT( witness_properties,
   (account_creation_fee)
   (maximum_block_size)
   (sbd_interest_rate)
   (account_subsidy_budget)
   (account_subsidy_decay)

   (key)
   (new_signing_key)
   (sbd_exchange_rate)
   (url)
   );

class serialize_member_visitor
{
   public:
      serialize_member_visitor( const witness_properties& in, flat_map< string, vector<char> >& out )
         : _in(in), _out(out) {}

      template<typename Member, class Class, Member (Class::*member)>
      void operator()( const char* name )const
      {
         if( !(_in.*member) )
            return;

         vector<char> v = fc::raw::pack_to_vector( *(_in.*member) );
         _out.emplace( name, std::move( v ) );
      }

   private:
      const witness_properties& _in;
      flat_map< string, vector<char> >& _out;
};

int main( int argc, char** argv, char** envp )
{
   // Serialize the witness_set_properties_operation
   // Take a sequence of witness_properties, one per line
   while( std::cin )
   {
      std::string line;
      std::getline( std::cin, line );
      boost::trim(line);
      if( line == "" )
         continue;
      try
      {
         fc::variant v = fc::json::from_string( line, fc::json::strict_parser );
         witness_properties wprops;
         witness_set_properties_operation op;
         fc::from_variant( v, wprops );
         serialize_member_visitor vtor( wprops, op.props );

         // For each field
         fc::reflector< witness_properties >::visit( vtor );

         std::cout << fc::json::to_string( op.props ) << std::endl;
      }
      catch( const fc::exception& e )
      {
         elog( "Error: ${e}", ("e", e.to_detail_string()) );
         return 1;
      }
   }

   return 0;
}
