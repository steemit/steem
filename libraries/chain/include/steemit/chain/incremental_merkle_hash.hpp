
#pragma once

#include <fc/crypto/sha256.hpp>
#include <fc/io/raw.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>

#include <cstdint>
#include <vector>

namespace steemit { namespace chain {

/**
 * Contains a data structure which allows incremental computation of Merkle root.
 */

class incremental_merkle_hash
{
   public:
      typedef fc::sha256 hash_type;

      incremental_merkle_hash() {}
      virtual ~incremental_merkle_hash() {}

      /**
       * The algorithm used to compute inner node hashes.
       */
      static hash_type hash_pair( const hash_type& left, const hash_type& right )
      {
         return hash_type::hash( std::make_pair( left, right ) );
      }

      /**
       * The definition of the root hash of an empty tree.
       */
      static hash_type empty_tree_hash();

      /**
       * Add a node hash.
       * Worst-case performance is O(log(n)) hash_pair() operations.
       * Average-case performance for building a tree is O(1) hash_pair() operations.
       */
      void add_node_hash( const hash_type& node );

      /**
       * Compute the root hash.  Requires O(log(n)) hash_apri
       */
      hash_type compute_root_hash()const;

      /**
       * Convenience method to hash and add a node of any FC serializable type.
       */
      template< typename T >
      void add_node( const T& node )
      {
         add_node_hash( hash_type::hash( node ) );
      }

      /**
       * The number of leaf nodes that have been added.
       */
      uint64_t leaf_count = 0;

      /**
       * The hash of the root of each L-branch.
       *
       * An L-branch is a subtree with the following two conditions:
       * - The L-branch root's parent is an ancestor node of the next leaf node.
       * - The L-branch root is not an ancestor node of the next leaf node.
       *
       * Since the tree's filled left-to-right, L-branch roots are the left-child siblings
       * of the right-child ancestors of the next leaf node.
       */
      std::vector< hash_type > lbranches;
};

/**
 * Incremental Merkle hash which remembers the last computed hash value.
 * Avoids expensive recomputation of hashes.  Has the same
 * binary and JSON serialization as incremental_merkle_hash.
 */

class annotated_incremental_merkle_hash
{
   typedef incremental_merkle_hash::hash_type hash_type;

   annotated_incremental_merkle_hash() {}
   virtual ~annotated_incremental_merkle_hash() {}

   hash_type get_root_hash()
   {
      if( !root_hash.valid() )
         root_hash = ihash.compute_root_hash();
      return *root_hash;
   }

   void add_node_hash( const hash_type& node )
   {
      root_hash.reset();
      ihash.add_node_hash( node );
   }

   template< typename T >
   void add_node( const T& node )
   {
      add_node_hash( hash_type::hash( node ) );
   }

   incremental_merkle_hash ihash;
   fc::optional< hash_type > root_hash;
};

} }

FC_REFLECT( steemit::chain::incremental_merkle_hash, (leaf_count)(lbranches) )

namespace fc {

template<typename T>
void to_variant( const steemit::chain::annotated_incremental_merkle_hash& h, variant& vo )
{
   to_variant( h.ihash, vo );
}

template<typename T>
void from_variant( const variant& vo, steemit::chain::annotated_incremental_merkle_hash& h )
{
   from_variant( vo, h.ihash );
}

namespace raw {

template< typename Stream >
inline void pack( Stream& s, const steemit::chain::annotated_incremental_merkle_hash& h )
{
   fc::raw::pack( s, h.ihash );
}

template< typename Stream >
inline void unpack( Stream& s, steemit::chain::annotated_incremental_merkle_hash& h )
{
   h.root_hash.reset();
   unpack( s, h.ihash );
}

} }
