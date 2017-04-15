#include <boost/format.hpp>
#include <CL/cl.hpp>

#define CHK(code) \
    if ((code) != CL_SUCCESS)\
    {\
     std::cerr << (boost::format\
                   ("ERROR:\n\t\
                     Code: %d\n\t\
                     Filename: %s\n\t\
                     Line: %d\n")\
                   % (code) % __FILE__ % __LINE__);\
     exit(code);\
    }
