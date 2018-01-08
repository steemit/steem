#if !defined(SERIALIZED_HEADERS)
   #pragma error("serialize_operators.hpp can be included only from serialized_headers.hpp!")
#endif

#include <chainbase/chainbase.hpp>

#include <steem/chain/steem_objects.hpp>
#include <steem/chain/account_object.hpp>
#include <steem/chain/block_summary_object.hpp>
#include <steem/chain/comment_object.hpp>
#include <steem/chain/global_property_object.hpp>
#include <steem/chain/hardfork_property_object.hpp>
//#include <steem/chain/node_property_object.hpp>
#include <steem/chain/history_object.hpp>
#include <steem/chain/transaction_object.hpp>
#include <steem/chain/witness_objects.hpp>
#include <steem/chain/smt_objects/smt_token_object.hpp>
#include <steem/chain/smt_objects/account_balance_object.hpp>

#include <serialize3/h/storage/serializedumper.h>
#include <serialize3/h/storage/serializeloader.h>

class TStreamOut
{
   public:
   TStreamOut(ASerializeDumper& dumper) : Dumper(dumper) {}

   void write(const char* data, size_t len)
      { Dumper.WriteBuffer(reinterpret_cast<const unsigned char*>(data), len); }

   private:
   ASerializeDumper& Dumper;
};

class TStreamIn
{
   public:
   TStreamIn(ASerializeLoader& loader) : Loader(loader) {}

   void read(char* data, size_t len)
      { Loader.ReadBuffer(reinterpret_cast<unsigned char*>(data), len); }

   private:
   ASerializeLoader& Loader;
};

#define FC_DUMP_BODY() \
   { \
   TStreamOut s(dumper); \
   fc::raw::pack(s, o); \
   }

#define FC_LOAD_BODY() \
   { \
   TStreamIn s(loader); \
   fc::raw::unpack(s, o); \
   }


/// fc types
inline
void operator&(ASerializeDumper& dumper, const fc::ripemd160& o)
   FC_DUMP_BODY()

inline
void operator&(ASerializeLoader& loader, fc::ripemd160& o)
   FC_LOAD_BODY()

inline
void operator&(ASerializeDumper& dumper, const fc::uint128& o)
   FC_DUMP_BODY()

inline
void operator&(ASerializeLoader& loader, fc::uint128& o)
   FC_LOAD_BODY()

inline
void operator&(ASerializeDumper& dumper, const fc::time_point_sec& o)
   FC_DUMP_BODY()

inline
void operator&(ASerializeLoader& loader, fc::time_point_sec& o)
   FC_LOAD_BODY()

template <typename TType>
void operator&(ASerializeDumper& dumper, const fc::safe<TType>& o)
   FC_DUMP_BODY()

template <typename TType>
void operator&(ASerializeLoader& loader, fc::safe<TType>& o)
   FC_LOAD_BODY()

template<typename T, size_t N>
void operator&(ASerializeDumper& dumper, const fc::array<T,N>& o)
   FC_DUMP_BODY()

template<typename T, size_t N>
void operator&(ASerializeLoader& loader, fc::array<T,N>& o)
   FC_LOAD_BODY()

inline
void operator&(ASerializeDumper& dumper, const fc::sha256& o)
   FC_DUMP_BODY()

inline
void operator&(ASerializeLoader& loader, fc::sha256& o)
   FC_LOAD_BODY()
