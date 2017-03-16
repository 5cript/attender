#pragma once

#include "tcp_fwd.hpp"

#include <cstdint>
#include <iosfwd>
#include <string>
#include <iterator>
#include <vector>

namespace attender
{
    class tcp_read_sink
    {
    public:
        /**
         *  @param max_bytes The parameter configures the maximum amount of bytes that this sink accepts.
         *                   If this number is exceeded,
         */
        tcp_read_sink();
        virtual ~tcp_read_sink() = default;

        /**
         *  @param data The data to write to the sink
         *  @param size The amount of bytes to write to the sink.
         *
         *  @return Returns the actual amount of bytes written to the sink.
         */
        virtual size_type write(const char* data, size_type size) = 0;

        /**
         *  @param buffer The data to write to the sink.
         *  @param amount The amount of bytes to write to the sink.
         *
         *  @return Returns the actual amount of bytes written to the sink.
         */
        virtual size_type write(std::vector <char> const& buffer, size_type amount) = 0;

        /**
         *  @return Returns the total amount of bytes that were written using this particular sink instance.
         */
        size_type get_total_bytes_written() const;

    protected:
        size_type written_bytes_;
        // uint64_t max_bytes_;
    };

    class tcp_stream_sink : public tcp_read_sink
    {
    public:
        explicit tcp_stream_sink(std::ostream* sink);
        size_type write(const char* data, size_type size) override;
        size_type write(std::vector <char> const& buffer, size_type amount) override;
    private:
        std::ostream* sink_;
    };

    class tcp_string_sink : public tcp_read_sink
    {
    public:
        explicit tcp_string_sink(std::string* sink);
        size_type write(const char* data, size_type size) override;
        size_type write(std::vector <char> const& buffer, size_type amount) override;
    private:
        std::string* sink_;
    };
}
