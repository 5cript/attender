#pragma once

#include <string>

namespace attender
{
    class session
    {
    public:
        session(std::string id = "");
		virtual ~session() = default;
        std::string id() const;

    private:
        std::string id_;
    };
}
