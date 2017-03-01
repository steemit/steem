
#include <steemit/chain/incremental_merkle_hash.hpp>

namespace steemit { namespace chain {

incremental_merkle_hash::hash_type incremental_merkle_hash::empty_tree_hash()
{
   return incremental_merkle_hash::hash_type::hash("");
}

void incremental_merkle_hash::add_node_hash( const incremental_merkle_hash::hash_type& node )
{
   //
   // Every 1 in the binary representation of leaf_count is an L-branch.
   //
   // An even-numbered leaf will become a new L-branch consisting of a single node.
   //
   // For an odd-numbered leaf, we can follow some number of right-child links in
   // its ancestry until we reach the lowest left-child link (if we traverse all the
   // way to the root, we simply reparent the tree as the left child of a new root).
   // Call this the R-path.  The number of links in the R-path is the number of 1
   // bits at the end of leaf_count.
   //
   // The root of the right-child links is a new L-branch obtained by combining the
   // L-branches along the R-path with the new node.
   //

   uint64_t lbits = leaf_count ^ (leaf_count+1);
   // leaf_count=...0      -> lbits=1
   // leaf_count=...01     -> lbits=11
   // leaf_count=...011    -> lbits=111
   // leaf_count=...0111   -> lbits=1111
   // leaf_count=...01^K   -> lbits=1^(K+1)

   lbranches.push_back( node );
   size_t n = lbranches.size();

   while( lbits > 1 )
   {
      lbranches[n-2] = hash_pair( lbranches[n-2], lbranches[n-1] );
      lbranches.pop_back();
      --n;
      lbits >>= 1;
   }
   ++leaf_count;
}

incremental_merkle_hash::hash_type incremental_merkle_hash::compute_root_hash()const
{
   size_t n = lbranches.size();
   if( n == 0 )
      return empty_tree_hash();

   hash_type h = lbranches[--n];
   while( n > 0 )
   {
      --n;
      h = hash_pair( lbranches[n], h );
   }
   return h;
}

} } // steemit::chain
