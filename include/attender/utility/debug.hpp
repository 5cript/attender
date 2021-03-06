#pragma once

#include <boost/preprocessor/stringize.hpp>
#include <boost/system/error_code.hpp>

#define DEV_BUILD

namespace attender
{
#ifndef _WIN32
#   define ATTENDER_CODE_PLACE BOOST_PP_STRINGIZE(__FILE__), BOOST_PP_STRINGIZE(__LINE__), __PRETTY_FUNCTION__
#else
#	define ATTENDER_CODE_PLACE BOOST_PP_STRINGIZE(__FILE__), BOOST_PP_STRINGIZE(__LINE__), __func__ 
#endif

    void dump(boost::system::error_code const& ec, char const* file, char const* line, char const* function);

#ifdef DEV_BUILD
#   define DUMP(...) dump(__VA_ARGS__);
#else
#   define DUMP(...)
#endif // DEV_BUILD
}
