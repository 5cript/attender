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
        void id(std::string const& id);

    private:
        std::string id_;
    };
}
