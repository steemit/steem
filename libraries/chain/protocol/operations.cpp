#include <steemit/chain/protocol/operations.hpp>

namespace fc {
   using namespace steemit::chain;

   string name_from_type( string type_name ) {
      auto start = type_name.find_last_of( ':' )+1;
      auto end   = type_name.find_last_of( '_' );
      return type_name.substr( start, end-start );
   }
   struct from_operation
   {
      variant& var;
      from_operation( variant& dv ):var(dv){}

      typedef void result_type;
      template<typename T> void operator()( const T& v )const
      {
         auto name = name_from_type( fc::get_typename<T>::name() );
         var = variant( std::make_pair(name,v) );
      }
   };
   struct get_operation_name
   {
      string& name;
      get_operation_name( string& dv ):name(dv){}

      typedef void result_type;
      template<typename T> void operator()( const T& v )const
      {
         name = name_from_type( fc::get_typename<T>::name() );
      }
   };

   template< typename Ops = static_variant<> >
   void abstract_operation_to_variant( const Ops& var,  fc::variant& vo )
   {
      var.visit( from_operation( vo ) );
   }

   template< typename Ops = static_variant<> >
   void abstract_operation_from_variant( const fc::variant& var,  Ops& vo )
   {
      static std::map<string,uint32_t> to_tag = [](){
         std::map<string,uint32_t> name_map;
         for( int i = 0; i < Ops::count(); ++i ) {
            Ops tmp;
            tmp.set_which(i);
            string n;
            tmp.visit( get_operation_name(n) );
            name_map[n] = i;
         }
         return name_map;
      }();

      auto ar = var.get_array();
      if( ar.size() < 2 ) return;
      if( ar[0].is_uint64() )
         vo.set_which( ar[0].as_uint64() );
      else
      {
         auto itr = to_tag.find(ar[0].as_string());
         FC_ASSERT( itr != to_tag.end(), "Invalid operation name: ${n}", ("n", ar[0]) );
         vo.set_which( to_tag[ar[0].as_string()] );
      }
       vo.visit( fc::to_static_variant( ar[1] ) );
   }

   void to_variant( const steemit::chain::operation& var,  fc::variant& vo )
    {
       abstract_operation_to_variant< steemit::chain::operation >( var, vo );
    }

    void from_variant( const fc::variant& var,  steemit::chain::operation& vo )
    {
       abstract_operation_from_variant< steemit::chain::operation >( var, vo );
    }
}

namespace steemit { namespace chain {


/**
 * @brief Used to validate operations in a polymorphic manner
 */
struct operation_validator
{
   typedef void result_type;
   template<typename T>
   void operator()( const T& v )const { v.validate(); }
};

struct operation_get_required_auth
{
   typedef void result_type;

   flat_set<string>&    active;
   flat_set<string>&    owner;
   flat_set<string>&    posting;
   vector<authority>&   other;

   operation_get_required_auth( flat_set<string>& a,
     flat_set<string>& own,
     flat_set<string>& post,
     vector<authority>&  oth ):active(a),owner(own),posting(post),other(oth){}

   template<typename T>
   void operator()( const T& v )const
   {
      v.get_required_active_authorities( active );
      v.get_required_owner_authorities( owner );
      v.get_required_posting_authorities( posting );
      v.get_required_authorities( other );
   }
};

template< typename Ops = static_variant<> >
void abstract_operation_validate( const Ops& op )
{
   op.visit( operation_validator() );
}

void operation_validate( const operation& op )
{
   abstract_operation_validate< operation >( op );
}

template< typename Ops = static_variant<> >
void abstract_operation_get_required_authorities( const Ops& op,
                                                  flat_set<string>& active,
                                                  flat_set<string>& owner,
                                                  flat_set<string>& posting,
                                                  vector<authority>&  other )
{
   op.visit( operation_get_required_auth( active, owner, posting, other ) );
}

void operation_get_required_authorities( const operation& op,
                                         flat_set< string >& active,
                                         flat_set< string >& owner,
                                         flat_set< string >& posting,
                                         vector< authority >& other )
{
abstract_operation_get_required_authorities< operation >( op, active, owner, posting, other );
}

struct is_market_op_visitor {
   typedef bool result_type;

   template<typename T>
   bool operator()( T&& v )const { return false; }
   bool operator()( const limit_order_create_operation& )const { return true; }
   bool operator()( const limit_order_cancel_operation& )const { return true; }
   bool operator()( const transfer_operation& )const { return true; }
   bool operator()( const transfer_to_vesting_operation& )const { return true; }
};

bool is_market_operation( const operation& op ) {
   return op.visit( is_market_op_visitor() );
}

} } // namespace steemit::chain
