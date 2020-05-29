#include "mount_options.hpp"

#include <stdexcept>

namespace attender
{
//#####################################################################################################################
    const char* mount_option_to_string(mount_options option)
    {
        switch (option)
        {
            case(GET): return "GET";
            case(PUT): return "PUT";
            case(POST): return "POST";
            case(DELETE): return "DELETE";
            case(HEAD): return "HEAD";
            case(OPTIONS): return "OPTIONS";
        }
        throw std::runtime_error("unimplemented option string. This is an attender bug.");
    }
//---------------------------------------------------------------------------------------------------------------------
//#####################################################################################################################
}
