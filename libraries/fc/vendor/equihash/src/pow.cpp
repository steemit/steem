/*Code by Dmitry Khovratovich, 2016
CC0 license

Modifications by Steemit, Inc. 2016
*/

#include <equihash/pow.hpp>
#include <equihash/blake2.h>
#include <algorithm>

#include <../../../include/fc/macros.hpp>

#ifdef EQUIHASH_POW_VERBOSE
#include <iomanip>
#include <iostream>

#define EQUIHASH_LOG(s) \
   std::cerr << s << std::endl;
#else
#define EQUIHASH_LOG(s)
#endif

static uint64_t rdtsc(void) {
#ifdef _MSC_VER
    return __rdtsc();
#else
#if defined(__amd64__) || defined(__x86_64__)
    uint64_t rax, rdx;
    __asm__ __volatile__("rdtsc" : "=a"(rax), "=d"(rdx) : : );
    return (rdx << 32) | rax;
#elif defined(__i386__) || defined(__i386) || defined(__X86__)
    uint64_t rax;
    __asm__ __volatile__("rdtsc" : "=A"(rax) : : );
    return rax;
#else
#error "Not implemented!"
#endif
#endif
}


using namespace _POW;
using namespace std;

void Equihash::InitializeMemory()
{
    uint32_t  tuple_n = ((uint32_t)1) << (n / (k + 1));
    Tuple default_tuple(k); // k blocks to store (one left for index)
    std::vector<Tuple> def_tuples(LIST_LENGTH, default_tuple);
    tupleList = std::vector<std::vector<Tuple>>(tuple_n, def_tuples);
    filledList= std::vector<unsigned>(tuple_n, 0);
    solutions.resize(0);
    forks.resize(0);
}

void Equihash::FillMemory(uint32_t length) //works for k<=7
{
    uint32_t input[SEED_LENGTH + 2];
    for (unsigned i = 0; i < SEED_LENGTH; ++i)
        input[i] = seed[i];
    input[SEED_LENGTH] = nonce;
    input[SEED_LENGTH + 1] = 0;
    uint32_t buf[MAX_N / 4];
    for (unsigned i = 0; i < length; ++i, ++input[SEED_LENGTH + 1]) {
        blake2b((uint8_t*)buf, &input, NULL, sizeof(buf), sizeof(input), 0);
        uint32_t index = buf[0] >> (32 - n / (k + 1));
        unsigned count = filledList[index];
        if (count < LIST_LENGTH) {
            for (unsigned j = 1; j < (k + 1); ++j) {
                //select j-th block of n/(k+1) bits
                tupleList[index][count].blocks[j - 1] = buf[j] >> (32 - n / (k + 1));
            }
            tupleList[index][count].reference = i;
            filledList[index]++;
        }
    }
}

std::vector<Input> Equihash::ResolveTreeByLevel(Fork fork, unsigned level) {
    if (level == 0)
        return std::vector<Input>{fork.ref1, fork.ref2};
    auto v1 = ResolveTreeByLevel(forks[level - 1][fork.ref1], level - 1);
    auto v2 = ResolveTreeByLevel(forks[level - 1][fork.ref2], level - 1);
    v1.insert(v1.end(), v2.begin(), v2.end());
    return v1;
}

std::vector<Input> Equihash::ResolveTree(Fork fork) {
    return ResolveTreeByLevel(fork, forks.size());
}


void Equihash::ResolveCollisions(bool store) {
    const unsigned tableLength = tupleList.size();  //number of rows in the hashtable
    const unsigned maxNewCollisions = tupleList.size()*FORK_MULTIPLIER;  //max number of collisions to be found
    const unsigned newBlocks = tupleList[0][0].blocks.size() - 1;// number of blocks in the future collisions
    std::vector<Fork> newForks(maxNewCollisions); //list of forks created at this step
    auto tableRow = vector<Tuple>(LIST_LENGTH, Tuple(newBlocks)); //Row in the hash table
    vector<vector<Tuple>> collisionList(tableLength,tableRow);
    std::vector<unsigned> newFilledList(tableLength,0);  //number of entries in rows
    uint32_t newColls = 0; //collision counter
    for (unsigned i = 0; i < tableLength; ++i) {
        for (unsigned j = 0; j < filledList[i]; ++j)        {
            for (unsigned m = j + 1; m < filledList[i]; ++m) {   //Collision
                //New index
                uint32_t newIndex = tupleList[i][j].blocks[0] ^ tupleList[i][m].blocks[0];
                Fork newFork = Fork(tupleList[i][j].reference, tupleList[i][m].reference);
                //Check if we get a solution
                if (store) {  //last step
                    if (newIndex == 0) {//Solution
                        std::vector<Input> solution_inputs = ResolveTree(newFork);
                        solutions.push_back(Proof(n, k, seed, nonce, solution_inputs));
                    }
                }
                else {         //Resolve
                    if (newFilledList[newIndex] < LIST_LENGTH && newColls < maxNewCollisions) {
                        for (unsigned l = 0; l < newBlocks; ++l) {
                            collisionList[newIndex][newFilledList[newIndex]].blocks[l]
                                = tupleList[i][j].blocks[l+1] ^ tupleList[i][m].blocks[l+1];
                        }
                        newForks[newColls] = newFork;
                        collisionList[newIndex][newFilledList[newIndex]].reference = newColls;
                        newFilledList[newIndex]++;
                        newColls++;
                    }//end of adding collision
                }
            }
        }//end of collision for i
    }
    forks.push_back(newForks);
    std::swap(tupleList, collisionList);
    std::swap(filledList, newFilledList);
}

Proof Equihash::FindProof(){
    this->nonce = 1;
    while (nonce < MAX_NONCE) {
        nonce++;
        uint64_t start_cycles = rdtsc();
        InitializeMemory(); //allocate
        FillMemory(4UL << (n / (k + 1)-1));   //fill with hashes
        /*fp = fopen("proof.log", "a+");
        fprintf(fp, "\n===MEMORY FILLED:\n");
        PrintTuples(fp);
        fclose(fp);*/
        for (unsigned i = 1; i <= k; ++i) {
            bool to_store = (i == k);
            ResolveCollisions(to_store); //XOR collisions, concatenate indices and shift
           /* fp = fopen("proof.log", "a+");
            fprintf(fp, "\n===RESOLVED AFTER STEP %d:\n", i);
            PrintTuples(fp);
            fclose(fp);*/
        }
        uint64_t stop_cycles = rdtsc();

        double  mcycles_d = (double)(stop_cycles - start_cycles) / (1UL << 20);
        uint32_t kbytes = (tupleList.size()*LIST_LENGTH*k*sizeof(uint32_t)) / (1UL << 10);

        FC_UNUSED(mcycles_d, kbytes);

        //Duplicate check
        for (unsigned i = 0; i < solutions.size(); ++i) {
            auto vec = solutions[i].inputs;
            std::sort(vec.begin(), vec.end());
            bool dup = false;
            for (unsigned k = 0; k < vec.size() - 1; ++k) {
                if (vec[k] == vec[k + 1])
                    dup = true;
            }
            if (!dup)
            {
                return solutions[i].CanonizeIndexes();
            }
        }
    }
    return Proof(n, k, seed, nonce, std::vector<uint32_t>());
}

/**
 * Added by Steemit, Inc. for single iteration
 */
Proof Equihash::FindProof( Nonce _nonce )
{
    this->nonce = _nonce;
    InitializeMemory(); //allocate
    FillMemory(4UL << (n / (k + 1)-1));   //fill with hashes

    for (unsigned i = 1; i <= k; ++i) {
        bool to_store = (i == k);
        ResolveCollisions(to_store); //XOR collisions, concatenate indices and shift
    }

    //Duplicate check
    for (unsigned i = 0; i < solutions.size(); ++i) {
       auto vec = solutions[i].inputs;
       std::sort(vec.begin(), vec.end());
       bool dup = false;
       for (unsigned k = 0; k < vec.size() - 1; ++k) {
             if (vec[k] == vec[k + 1])
                dup = true;
       }
       if (!dup)
       {
          return solutions[i].CanonizeIndexes();
       }
    }

    return Proof(n, k, seed, nonce, std::vector<uint32_t>());
}

Proof Proof::CanonizeIndexes()const
{
   // We consider the index values in the inputs array to be the leaf nodes of a binary
   // tree, and the inner nodes to be labelled with the XOR of the corresponding vector
   // elements.
   //
   // Define a binary tree to be canonically sorted if, for each inner node, the least
   // leaf descendant of the left child is less than the least leaf descendant of the
   // right child.
   //
   // This method puts the inputs into canonical order without altering the inner node
   // labels.  Thus canonization preserves the validity of the proof and the
   // footprint of Wagner's algorithm.
   //
   // We use a bottom-up traversal, dividing the input into successively larger power-of-2
   // blocks and swapping the two half-blocks if non-canonical.
   //
   // Say a block is least-first if the least element is the first element.
   //
   // If each half-block is least-first, the conditional swap ensures the full block will also
   // be least-first. The half-blocks in the initial iteration are obviously least-first
   // (they only have a single element!).  So by induction, at each later iteration the half-blocks
   // of that iteration are least-first (since they were the full blocks of the previous iteration,
   // which were made least-first by the previous iteration's conditional swap).
   //
   // As a consequence, no search is necessary to find the least element in each half-block,
   // it is always the first element in the half-block.

   std::vector< uint32_t > new_inputs = inputs;

   size_t input_size = inputs.size();
   size_t half_size = 1;
   size_t block_size = 2;
   while( block_size <= input_size )
   {
      for( size_t i=0; i+block_size<=input_size; i+=block_size )
      {
         auto ita = new_inputs.begin()+i, itb = ita+half_size;
         if( (*ita) >= (*itb) )
         {
            std::swap_ranges( ita, itb, itb );
         }
      }
      half_size = block_size;
      block_size += block_size;
   }
   return Proof(n, k, seed, nonce, new_inputs);
}

bool Proof::CheckIndexesCanon()const
{
   // This method is logically identical to CanonizeIndexes() but will return false
   // instead of swapping elements.

   size_t input_size = inputs.size();
   size_t half_size = 1;
   size_t block_size = 2;
   while( block_size <= input_size )
   {
      for( size_t i=0; i+block_size<=input_size; i+=block_size )
      {
         auto ita = inputs.begin()+i, itb = ita+half_size;
         if( (*ita) >= (*itb) )
            return false;
      }
      half_size = block_size;
      block_size += block_size;
   }
   return true;
}

bool Proof::Test()
{
    uint32_t input[SEED_LENGTH + 2];
    for (unsigned i = 0; i < SEED_LENGTH; ++i)
        input[i] = seed[i];
    input[SEED_LENGTH] = nonce;
    input[SEED_LENGTH + 1] = 0;
    uint32_t buf[MAX_N / 4];
    std::vector<uint32_t> blocks(k+1,0);
    for (unsigned i = 0; i < inputs.size(); ++i) {
        input[SEED_LENGTH + 1] = inputs[i];
        blake2b((uint8_t*)buf, &input, NULL, sizeof(buf), sizeof(input), 0);
        for (unsigned j = 0; j < (k + 1); ++j) {
            //select j-th block of n/(k+1) bits
            blocks[j] ^= buf[j] >> (32 - n / (k + 1));
        }
    }
    bool b = inputs.size() != 0;
    for (unsigned j = 0; j < (k + 1); ++j) {
        b &= (blocks[j] == 0);
    }

    return b;
}

bool Proof::FullTest()const
{
   // Length must be 2**k
   if( inputs.size() != size_t(1 << k) )
   {
      EQUIHASH_LOG( "PoW failed length test" );
      return false;
   }

   // Ensure all values are distinct
   std::vector<Input> sorted_inputs = inputs;
   std::sort( sorted_inputs.begin(), sorted_inputs.end() );
   for( size_t i=1; i<inputs.size(); i++ )
   {
      if( sorted_inputs[i-1] >= sorted_inputs[i] )
      {
         EQUIHASH_LOG( "PoW failed distinct test" );
         return false;
      }
   }

   // Ensure all values are canonically indexed
   /*
   if( !CheckIndexesCanon() )
      return false;
   */

   // Initialize blocks array
   uint32_t input[SEED_LENGTH + 2];
   for( size_t i=0; i<SEED_LENGTH; i++ )
      input[i] = seed[i];
   input[SEED_LENGTH] = nonce;
   input[SEED_LENGTH + 1] = 0;
   uint32_t buf[MAX_N / 4];

   std::vector< std::vector< uint32_t > > blocks;

   const uint32_t max_input = uint32_t(1) << (n / (k + 1) + 1);

   for( size_t i=0; i<inputs.size(); i++ )
   {
      input[SEED_LENGTH + 1] = inputs[i];
      if( inputs[i] >= max_input )
      {
         EQUIHASH_LOG( "PoW failed max_input test" );
         return false;
      }

      blake2b((uint8_t*)buf, &input, NULL, sizeof(buf), sizeof(input), 0);
      blocks.emplace_back();
      std::vector<uint32_t>& x = blocks.back();
      x.resize(k+1);
      for( size_t j=0; j<(k+1); j++ )
      {
         //select j-th block of n/(k+1) bits
         x[j] = buf[j] >> (32 - n / (k + 1));
      }
   }

   while( true )
   {
#ifdef EQUIHASH_POW_VERBOSE
      std::cerr << "\n\nBegin loop iteration\n";
      for( const std::vector< uint32_t >& x : blocks )
      {
         for( const uint32_t& e : x )
            std::cerr << std::hex << std::setw(5) << e << " ";
         std::cerr << std::endl;
      }
#endif

      size_t count = blocks.size();
      if( count == 0 )
      {
         EQUIHASH_LOG( "PoW failed with count == 0" );
         return false;
      }
      if( count == 1 )
      {
         if( blocks[0].size() != 1 )
         {
            EQUIHASH_LOG( "PoW failed due to vector size" );
            return false;
         }
         if( blocks[0][0] != 0 )
         {
            EQUIHASH_LOG( "PoW failed because final bits are not zero" );
            return false;
         }
         return true;
      }
      if( (count&1) != 0 )
      {
         EQUIHASH_LOG( "PoW failed with odd count" );
         return false;
      }
      for( size_t i=0,new_i=0; i<count; i+=2,new_i++ )
      {
         if( blocks[i][0] != blocks[i+1][0] )
         {
            EQUIHASH_LOG( "PoW failed because leading element of vector pair does not match" );
            return false;
         }
         for( size_t j=1; j<blocks[i].size(); j++ )
            blocks[new_i][j-1] = blocks[i][j] ^ blocks[i+1][j];
         blocks[new_i].resize(blocks[new_i].size()-1);
      }
      blocks.resize(blocks.size() >> 1);
   }
}
