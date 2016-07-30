#pragma once

#include "tcp_fwd.hpp"

#include <cstdint>
#include <iosfwd>
#include <string>
#include <iterator>
#include <vector>
#include <limits>

namespace attender
{
    class tcp_read_sink
    {
    public:
        tcp_read_sink();
        virtual ~tcp_read_sink() = default;
        virtual void write(const char* data, std::size_t size) = 0;
        virtual void write(std::vector <char> const& buffer, std::size_t amount = std::numeric_limits <std::size_t>::max()) = 0;

        uint64_t get_bytes_written() const;

    protected:
        uint64_t written_bytes_;
    };

    class tcp_stream_sink : public tcp_read_sink
    {
    public:
        tcp_stream_sink(std::ostream* sink);
        void write(const char* data, std::size_t size) override;
        void write(std::vector <char> const& buffer, std::size_t amount = std::numeric_limits <std::size_t>::max()) override;
    private:
        std::ostream* sink_;
    };

    class tcp_string_sink : public tcp_read_sink
    {
    public:
        tcp_string_sink(std::string* sink);
        void write(const char* data, std::size_t size) override;
        void write(std::vector <char> const& buffer, std::size_t amount = std::numeric_limits <std::size_t>::max()) override;
    private:
        std::string* sink_;
    };
}
