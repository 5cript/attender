#pragma once

namespace attender
{
    class session_data
    {
    public:
        virtual std::string serialize() = 0;
        virtual void deserialize(std::string const& data) = 0;

        virtual ~session_data() = default;
    };
}
