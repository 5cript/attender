#include "debug.hpp"

#include <boost/system/system_error.hpp>

#include <exception>
#include <iostream>
#include <ctime>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace attender
{
//#####################################################################################################################
    std::string getTimeStamp(bool spaceless = false)
    {
        std::stringstream sstr;
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);

        std::string pattern = "%d-%m-%Y %H-%M-%S";
        if (spaceless)
            pattern = "%d-%m-%Y_%H-%M-%S";

        sstr << std::put_time(&tm, pattern.c_str());
        return sstr.str();
    }
//#####################################################################################################################
    void dump(boost::system::error_code const& ec, char const* file, char const* line, char const* function)
    {
        std::cerr << "(" << getTimeStamp() << ") {" << file << ":" << line << " in \"" << function << "\"}: " << boost::system::system_error {ec}.what();
        std::terminate();
    }
//---------------------------------------------------------------------------------------------------------------------
//#####################################################################################################################
}
