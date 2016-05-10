#pragma once

#include <string>
#include <unordered_map>

namespace attender
{
    struct request_header
    {
        std::string verb;
        std::string url;
        std::string protocol;
        std::string version;

        std::string get_path() const;

        std::unordered_map <std::string, std::string> fields;

    private:
        std::string path_; // memoization
    };
}
