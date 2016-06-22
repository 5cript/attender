#pragma once

#include "tcp_fwd.hpp"
#include "request_header.hpp"

#include <boost/optional.hpp>

#include <string>
#include <iosfwd>
#include <stdexcept>

namespace attender
{
    namespace internal
    {
        enum class parser_progress
        {
            verb,
            url,
            protocol_and_version,
            protocol_and_version_phase_2,
            fields,
            body
        };
    }

    struct header_limitations_error : std::runtime_error
    {
        template <typename T>
        header_limitations_error(T&& msg)
            : std::runtime_error(std::forward <T&&> (msg))
        {}
    };

    class request_parser
    {
    public:
        request_parser();

        // returns true if parser finished parsing the header
        bool feed(tcp_connection* connection);

        request_header get_header() const;
        bool finished() const;
        std::string get_buffer() const;
        boost::optional <std::string> get_field(std::string const& key) const;

    private:
        bool get_line_from_buffer(std::string& line);
        bool get_word_from_buffer(std::string& word);
        bool expect_space();
        void add_field(std::string const& line);
        bool parse_words();

    private:
        request_header_intermediate header_;
        std::string header_buffer_;
        internal::parser_progress progress_;
        int field_parse_counter_;
    };
}
