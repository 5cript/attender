#pragma once

#include <string>

namespace attender
{
    class session
    {
    public:
        session(std::string id = "");
        std::string id() const;

    private:
        std::string id_;
    };
}
