#include <steem/protocol/operations.hpp>

#include <steem/protocol/operation_util_impl.hpp>

namespace fc
{
   std::string name_from_type( const std::string& type_name )
   {
      // Extract operation name from full type name
      // Example: "steem::protocol::transfer_operation" -> "transfer"
      const std::string prefix = "steem::protocol::";
      const std::string suffix = "_operation";
      
      if( type_name.size() >= prefix.size() + suffix.size() &&
          type_name.substr( 0, prefix.size() ) == prefix &&
          type_name.substr( type_name.size() - suffix.size(), suffix.size() ) == suffix )
      {
         return type_name.substr( prefix.size(), type_name.size() - prefix.size() - suffix.size() );
      }
      
      // Fallback: use trim_typename_namespace
      auto start = type_name.find_last_of( ':' );
      start = ( start == std::string::npos ) ? 0 : start + 1;
      auto end = type_name.find_last_of( '_' );
      if( end != std::string::npos && end > start )
      {
         return type_name.substr( start, end - start );
      }
      return type_name.substr( start );
   }
}

namespace steem { namespace protocol {

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

struct is_vop_visitor
{
   typedef bool result_type;

   template< typename T >
   bool operator()( const T& v )const { return v.is_virtual(); }
};

bool is_virtual_operation( const operation& op )
{
   return op.visit( is_vop_visitor() );
}

} } // steem::protocol

STEEM_DEFINE_OPERATION_TYPE( steem::protocol::operation )
