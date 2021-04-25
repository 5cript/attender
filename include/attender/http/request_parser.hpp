#pragma once

#include <attender/http/http_fwd.hpp>
#include <attender/http/http_connection_interface.hpp>
#include <attender/http/request_header.hpp>

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
        using buffer_size_type = std::string::size_type;

    public:
        request_parser();

        /**
         *  @return Returns true if the parser finished parsing the header.
         */
        bool feed(http_connection_interface* connection);

        /**
         *  @return Returns the parsed header.
         */
        request_header get_header() const;

        /**
         *  @return Returns true, if the header end was consumed by the parser.
         */
        bool finished() const;

        /**
         *  Returns the header buffer. The parser will consume from the front and upon
         *  finishing, this buffer will contain the rest that potentially contains the body.
         */
        std::string& get_buffer();

        /**
         *  Returns the first 'length' bytes from the header buffer and remote this from the buffer.
         *
         *  @param length The amount of bytes to read and remove.
         *  @return Returns the read string. Warning: this might be smaller than 'length'.
         */
        std::string read_front(size_type length);

        /**
         *  @return get_buffer().empty();
         */
        bool is_buffer_empty() const;

        /**
         *  Rewind the header parser to header entries. Required for 100-continue.
         */
        void rewind();

        /**
         *  @param key A header entry key, such as "Content-Length"
         *
         *  @return Returns the value associated with the key.
         */
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
