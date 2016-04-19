#pragma once

#include <string>
#include <unordered_map>

namespace attender
{
    struct request_header
    {
        std::string verb;
        std::string path;
        std::string protocol;
        std::string version;

        std::unordered_map <std::string, std::string> fields;
    };
}
