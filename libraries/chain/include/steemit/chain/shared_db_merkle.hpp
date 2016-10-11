#include <steemit/protocol/types.hpp>

namespace steemit { namespace chain {

inline static const map< uint32_t, checksum_type >& get_shared_db_merkle()
{
   static const map< uint32_t, checksum_type > shared_db_merkle
   {
      { 3705111, checksum_type( "0a8f0fd5450c3706ec8b8cbad795cd0b3679bf35" ) },
      { 3705120, checksum_type( "2027edb72b671f7011c8cc4c7a8b59c39b305093" ) },
      { 3713940, checksum_type( "bf8a1d516927c506ebdbb7b38bef2e992435435f" ) },
      { 3714132, checksum_type( "e8b77773d268b72c8d650337b8cce360bbe64779" ) },
      { 3714567, checksum_type( "45af59a8c2d7d4a606151ef5dae03d2dfe13fbdd" ) },
      { 3714588, checksum_type( "e64275443bdc82f104ac936486d367af8f6d1584" ) },
      { 4138790, checksum_type( "f65a3a788a2ef52406d8ba5705d7288be228403f" ) },
      { 5435426, checksum_type( "0b32538b2d22bd3146d54b6e3cb5ae8b9780e8a5" ) }
   };

   return shared_db_merkle;
}

} } //steemit::chain
