#include <boost/multi_index_container.hpp>

namespace mira
{

template< typename Value, typename IndexSpecifierList, typename Allocator >
class boost_multi_index_adapter : public class boost::multi_index_container< Value, IndexSpecifierList, Allocator >
{
   public:
      int64_t revision() { return 0; }
      int64_t set_revision() { return 0; }
      void print_stats() const {}
      void close() {}
      void wipe( const boost::filesystem::path& p ) {}
      void flush() {}
      bool open( const boost::filesystem::path& p ) {}
};

} // mira
