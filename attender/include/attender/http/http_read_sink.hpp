#pragma once

#include <attender/http/http_fwd.hpp>

#include <cstdint>
#include <iosfwd>
#include <string>
#include <iterator>
#include <vector>

namespace attender
{
    class http_read_sink
    {
    public:
        /**
         *  @param max_bytes The parameter configures the maximum amount of bytes that this sink accepts.
         *                   If this number is exceeded,
         */
        http_read_sink();
        virtual ~http_read_sink() = default;

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

    class http_stream_sink : public http_read_sink
    {
    public:
        explicit http_stream_sink(std::ostream* sink);
        size_type write(const char* data, size_type size) override;
        size_type write(std::vector <char> const& buffer, size_type amount) override;
    private:
        std::ostream* sink_;
    };

    class http_string_sink : public http_read_sink
    {
    public:
        explicit http_string_sink(std::string* sink);
        size_type write(const char* data, size_type size) override;
        size_type write(std::vector <char> const& buffer, size_type amount) override;
    private:
        std::string* sink_;
    };
}
