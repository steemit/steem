
#include <steem/utilities/benchmark_dumper.hpp>

namespace steem { namespace utilities {

#define PROC_STATUS_LINE_LENGTH 1028

typedef std::function<void(const char* key)> TScanErrorCallback;

uint64_t read_u64_value_from(FILE* input, const char* key, unsigned key_length, TScanErrorCallback error_callback)
{
   char line_buffer[PROC_STATUS_LINE_LENGTH];
   while( fgets(line_buffer, PROC_STATUS_LINE_LENGTH, input) != nullptr )
   {
      const char* found_pos = strstr(line_buffer, key);
      if( found_pos != nullptr )
      {
         uint64_t result = 0;

         /*
          Clang thinks &result is an unsignged long long *
          GCC thinks &result is a long unsigned int *
          */
#if defined( __clang__ )
         if( sscanf(found_pos+key_length, "%llu", &result) != 1 )
#else
         if( sscanf(found_pos+key_length, "%lu", &result) != 1 )
#endif
         {
            error_callback(key);
         }

         return result;
      }
   }

   error_callback(key);
   return 0;
}

bool benchmark_dumper::read_mem(pid_t pid, uint64_t* current_virtual, uint64_t* peak_virtual)
{
   const char* procPath = "/proc/self/status";
   FILE* input = fopen(procPath, "r");
   if(input == NULL)
   {
      elog( "cannot read: ${file} file.", ("file", procPath) );
      return false;
   }

   TScanErrorCallback error_callback = [procPath](const char* key)
   {
      elog( "cannot read value of ${key} key in ${file}", ("key", key) ("file", procPath) );
   };

   *peak_virtual = read_u64_value_from( input, "VmPeak:", 7, error_callback );
   *current_virtual = read_u64_value_from( input, "VmSize:", 7, error_callback );

   fclose(input);
   return true;
}

} }
