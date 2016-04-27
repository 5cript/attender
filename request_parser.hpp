#pragma once

#include "tcp_fwd.hpp"
#include "request_header.hpp"

#include <string>
#include <iosfwd>

namespace attender
{
    namespace internal
    {
        enum class parser_progress
        {
            verb,
            url,
            protocol_and_version,
            fields,
            body
        };
    }

    class request_parser
    {
    public:
        request_parser();

        // returns true if parser finished parsing the header
        bool feed(tcp_connection* connection);

        request_header get_header() const;
        bool finished() const;
        std::string get_buffer() const;

    private:
        bool get_line_from_buffer(std::string& line);
        bool get_word_from_buffer(std::string& word);
        void add_field(std::string const& line);
        bool parse_words();

    private:
        request_header header_;
        std::string header_buffer_;
        internal::parser_progress progress_;
        int field_parse_counter_;
    };
}
