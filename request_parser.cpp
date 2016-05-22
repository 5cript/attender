#include "net_core.hpp"
#include "request_parser.hpp"
#include "tcp_connection.hpp"

#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/iostreams/stream.hpp>

namespace attender
{
//#####################################################################################################################
    request_parser::request_parser()
        : header_{}
        , header_buffer_{}
        , progress_{internal::parser_progress::verb}
        , field_parse_counter_{0}
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    request_header request_parser::get_header() const
    {
        return {header_};
    }
//---------------------------------------------------------------------------------------------------------------------
    boost::optional <std::string> request_parser::get_field(std::string const& key) const
    {
        auto value = header_.fields.find(key);
        if (value != std::end(header_.fields))
            return value->second;

        return boost::none;
    }
//---------------------------------------------------------------------------------------------------------------------
    bool request_parser::finished() const
    {
        return progress_ == internal::parser_progress::body;
    }
//---------------------------------------------------------------------------------------------------------------------
    bool request_parser::feed(tcp_connection* connection)
    {
        if (finished())
            return true;

        tcp_stream_device device {connection};
        boost::iostreams::stream <tcp_stream_device> stream(device);

        std::string stream_string {std::istreambuf_iterator <char> (stream), {}};
        header_buffer_ += stream_string;

        // check max
        if (header_buffer_.size() > config::header_buffer_max)
            throw std::runtime_error("exceeded maximum of header buffer size");

        if (progress_ != internal::parser_progress::fields)
        {
            if (!parse_words())
            {
                connection->read();
                return false;
            }
        }

        do
        {
            std::string line;
            if (get_line_from_buffer(line))
            {
                if (line.empty())
                {
                    // header ended.
                    progress_ = internal::parser_progress::body;
                    return true;
                }

                // check max
                if (field_parse_counter_ + 1u > config::header_field_max)
                    throw std::runtime_error("exceeded maximum amount of header fields");

                add_field(line);
            }
            else
            {
                connection->read();
                break;
            }

        } while (true);

        return false;
    }
//---------------------------------------------------------------------------------------------------------------------
    bool request_parser::parse_words()
    {
        if (progress_ == internal::parser_progress::verb)
        {
            if (!get_word_from_buffer(header_.method))
                return false;
            progress_ = internal::parser_progress::url;
        }

        if (progress_ == internal::parser_progress::url)
        {
            if (!get_word_from_buffer(header_.url))
                return false;
            progress_ = internal::parser_progress::protocol_and_version;
        }

        if (progress_ == internal::parser_progress::protocol_and_version)
        {
            std::string protocol_and_version;
            if (!get_line_from_buffer(protocol_and_version))
                return false;

            auto slash_pos = protocol_and_version.find('/');
            if (slash_pos == std::string::npos)
                throw std::runtime_error("header does not contain correct protocol/version specification");

            header_.protocol = protocol_and_version.substr(0, slash_pos);
            header_.version = protocol_and_version.substr(slash_pos + 1, protocol_and_version.size() - slash_pos - 1);

            progress_ = internal::parser_progress::fields;
        }

        return true;
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_parser::add_field(std::string const& line)
    {
        ++field_parse_counter_;

        auto colpos = line.find(':');
        if (colpos == std::string::npos)
            throw std::runtime_error("header field does not contain colon");

        header_.fields[boost::algorithm::trim_right_copy(line.substr(0, colpos))] =
            boost::algorithm::trim_left_copy(line.substr(colpos + 1, line.size() - colpos - 1));
    }
//---------------------------------------------------------------------------------------------------------------------
    bool request_parser::get_line_from_buffer(std::string& line)
    {
        line.reserve(64);
        for (auto i = std::cbegin(header_buffer_), end = std::cend(header_buffer_); i != end; ++i)
        {
            if (*i != '\r')
                line.push_back(*i);
            else
            {
                auto seekpos = i + 1;
                if (seekpos != end && *seekpos == '\n')
                {
                    header_buffer_ = header_buffer_.substr(seekpos - std::cbegin(header_buffer_) + 1,
                                                           std::cend(header_buffer_) - std::cbegin(header_buffer_) - 2);

                    return true;
                }
                else
                    line.push_back('\r');
            }
        }

        return false;
    }
//---------------------------------------------------------------------------------------------------------------------
    bool request_parser::get_word_from_buffer(std::string& word)
    {
        auto space = header_buffer_.find(' ');
        if (space == std::string::npos)
            return false;

        word = header_buffer_.substr(0, space);
        header_buffer_ = header_buffer_.substr(space + 1, header_buffer_.size() - space - 1);
        return true;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string request_parser::get_buffer() const
    {
        return header_buffer_;
    }
//#####################################################################################################################
}
