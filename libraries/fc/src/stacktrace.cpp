// stacktrace.h (c) 2008, Timo Bingmann from http://idlebox.net/
// published under the WTFPL v2.0

// Downloaded from http://panthema.net/2008/0901-stacktrace-demangled/
// and modified for C++ and FC by Steemit, Inc.

#include <fc/filesystem.hpp>
#include <fc/macros.hpp>
#include <fc/stacktrace.hpp>

#if defined(__GNUC__) && !defined( __APPLE__ )

#include <cxxabi.h>
#include <execinfo.h>
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

FILE *popen(const char *command, const char *mode);
int pclose(FILE *stream);

namespace fc {

//
// This function is based on http://www.cplusplus.com/forum/beginner/117874/
// I use something platform-specific and self-contained, rather than using
// e.g. boost::process, because I want stacktrace to be small and self-contained
//
std::string run_command( const std::string& command )
{
    FILE* file = popen( command.c_str(), "r" ) ;

    if( file )
    {
        std::ostringstream ss;

        constexpr std::size_t MAX_LINE_SZ = 8192;
        char line[MAX_LINE_SZ];

        while( fgets( line, MAX_LINE_SZ, file ) )
           ss << line;

        pclose(file);
        return ss.str();
    }

    return "";
}

void print_stacktrace_linenums( void** addrlist, int addrlen )
{
   if( addrlen == 0 )
      return;

   char my_path[512];
   auto result = readlink("/proc/self/exe", my_path, 511);
   if( result < 0 )
   {
      std::cerr << "print_stacktrace_linenums() failed, could not read PID" << std::endl;
      return;
   }
   else
   {
      my_path[result] = '\0';
   }

   std::ostringstream ss_cmd;
   ss_cmd << "addr2line -p -a -f -C -i -e " << my_path;
   for( int i=0; i<addrlen; i++ )
      ss_cmd << " " << std::setfill('0') << std::setw(16) << std::hex << std::noshowbase << uint64_t(addrlist[i]);
   std::string cmd = ss_cmd.str();
   std::cerr << "executing command:" << std::endl;
   std::cerr << cmd << std::endl;
   std::string output = run_command(cmd);
   std::cerr << output << std::endl;
}

void print_stacktrace(std::ostream& out, unsigned int max_frames /* = 63 */, void* caller_overwrite_hack /* = nullptr */ )
{
    out << "stack trace:" << std::endl;

    // storage array for stack trace address data
    void* addrlist[max_frames+1];

    // retrieve current stack addresses
    int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

    if (addrlen == 0) {
	out << "  <empty, possibly corrupt>" << std::endl;
	return;
    }

    if( caller_overwrite_hack != nullptr )
        addrlist[2] = caller_overwrite_hack;

    // resolve addresses into strings containing "filename(function+address)",
    // this array must be free()-ed
    char** symbollist = backtrace_symbols(addrlist, addrlen);

    // allocate string which will be filled with the demangled function name
    size_t funcnamesize = 256;
    char* funcname = (char*)malloc(funcnamesize);

    // iterate over the returned symbol lines. skip the first, it is the
    // address of this function.
    for (int i = 1; i < addrlen; i++)
    {
	char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

	// find parentheses and +address offset surrounding the mangled name:
	// ./module(function+0x15c) [0x8048a6d]
	for (char *p = symbollist[i]; *p; ++p)
	{
	    if (*p == '(')
		begin_name = p;
	    else if (*p == '+')
		begin_offset = p;
	    else if (*p == ')' && begin_offset)
            {
		end_offset = p;
		break;
	    }
	}

        out << "  0x" << std::setfill('0') << std::setw(16) << std::hex << std::noshowbase << uint64_t(addrlist[i]);

	if (begin_name && begin_offset && end_offset
	    && begin_name < begin_offset)
	{
	    *begin_name++ = '\0';
	    *begin_offset++ = '\0';
	    *end_offset = '\0';

	    // mangled name is now in [begin_name, begin_offset) and caller
	    // offset in [begin_offset, end_offset). now apply
	    // __cxa_demangle():

	    int status;
	    char* ret = abi::__cxa_demangle(begin_name,
					    funcname, &funcnamesize, &status);
	    if (status == 0)
            {
		funcname = ret; // use possibly realloc()-ed string
                out << " " << symbollist[i] << " : " << funcname << "+" << begin_offset << std::endl;
	    }
	    else
            {
		// demangling failed. Output function name as a C function with
		// no arguments.
                out << " " << symbollist[i] << " : " << begin_name << "+" << begin_offset << std::endl;
	    }
	}
	else
	{
	    // couldn't parse the line? print the whole line.
            out << " " << symbollist[i] << std::endl;
	}
    }

    print_stacktrace_linenums(addrlist, addrlen);

    free(funcname);
    free(symbollist);
}

/* This structure mirrors the one found in /usr/include/asm/ucontext.h */
typedef struct _sig_ucontext
{
   unsigned long     uc_flags;
   struct ucontext*  uc_link;
   stack_t           uc_stack;
   struct sigcontext uc_mcontext;
   sigset_t          uc_sigmask;
} sig_ucontext_t;

// This function is based on https://stackoverflow.com/questions/77005/how-to-generate-a-stacktrace-when-my-gcc-c-app-crashes
void segfault_handler(int sig_num, siginfo_t * info, void * ucontext)
{
   void*              caller_address;
   sig_ucontext_t*    uc;

   uc = (sig_ucontext_t *)ucontext;

   /* Get the address at the time the signal was raised */
#if defined(__i386__) // gcc specific
   caller_address = (void *) uc->uc_mcontext.eip; // EIP: x86 specific
#elif defined(__x86_64__) // gcc specific
   caller_address = (void *) uc->uc_mcontext.rip; // RIP: x86_64 specific
#else
#error Unsupported architecture. // TODO: Add support for other arch.
#endif

   FC_UNUSED(caller_address);

   print_stacktrace( std::cerr, 128, nullptr );
   std::exit(EXIT_FAILURE);
}

void print_stacktrace_on_segfault()
{
   struct sigaction sigact;

   sigact.sa_sigaction = segfault_handler;
   sigact.sa_flags = SA_RESTART | SA_SIGINFO;

   if( sigaction(SIGSEGV, &sigact, (struct sigaction *)NULL) != 0 )
   {
      std::cerr << "Error setting signal handler" << std::endl;
      std::exit(EXIT_FAILURE);
   }
}

}

#elif defined( __APPLE__ )

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>

namespace fc {

void segfault_handler(int sig_num)
{
   std::cerr << "caught segfault\n";
   print_stacktrace( std::cerr, 128, nullptr );
   std::exit(EXIT_FAILURE);
}

void print_stacktrace( std::ostream& out, unsigned int max_frames /* = 63 */, void* caller_overwrite_hack /* = nullptr */ )
{
   std::cerr << "print stacktrace\n";
   assert( max_frames <= 128 );
   void* callstack[ 128 ];
   int frames = backtrace( callstack, max_frames );
   char** strs = backtrace_symbols( callstack, frames );

   for( size_t i = 0; i < frames; ++i )
   {
      out << strs[i] << "\n";
   }

   free( strs );
}

void print_stacktrace_on_segfault()
{
   struct sigaction sigact;

   sigact.sa_handler = &segfault_handler;
   sigact.sa_flags = SA_RESTART | SA_SIGINFO;

   std::cerr << "registering signal handler\n";

   if( sigaction(SIGSEGV, &sigact, NULL) != 0 )
   {
      std::cerr << "Error setting signal handler" << std::endl;
      std::exit(EXIT_FAILURE);
   }
}

}

#else

namespace fc {

void print_stacktrace(std::ostream& out, unsigned int max_frames /* = 63 */ )
{
    out << "stack trace not supported on this compiler" << std::endl;
}

void print_stacktrace_on_segfault() {}

}

#endif
