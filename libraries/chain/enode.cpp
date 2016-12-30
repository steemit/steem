
#include <steemit/chain/enode.hpp>

namespace steemit { namespace chain {

enode::enode() {}
enode::~enode() {}

enode_type::enode_type() {}
enode_type::~enode_type() {}

enode_tree::enode_tree() {}
enode_tree::~enode_tree() {}

} }

namespace fc {

void to_variant( steemit::chain::enode_type* const& t, variant& var )
{
   if( t == nullptr )
      var.clear();
   else
      var = t->get_name();
}

void from_variant( const variant& var, steemit::chain::enode_type*& t )
{
   FC_ASSERT( false, "this method not implemented" );
}

}
