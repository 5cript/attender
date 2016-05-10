#include "request.hpp"

#include "tcp_connection.hpp"
#include "tcp_read_sink.hpp"
#include "tcp_server_interface.hpp"

#include <string>
#include <iostream>
#include <boost/lexical_cast.hpp>

namespace attender
{
//#####################################################################################################################
    request_handler::request_handler(std::shared_ptr <tcp_connection> connection)
        : parser_{}
        , connection_(std::move(connection))
        , sink_(nullptr)
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_handler::initiate_header_read(parse_callback on_parse)
    {
        on_parse_ = on_parse;
        connection_->set_read_callback([this](boost::system::error_code ec) {
            header_read_handler(ec);
        });
        connection_->read();
    }
//---------------------------------------------------------------------------------------------------------------------
    request_header request_handler::get_header() const
    {
        return parser_.get_header();
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_handler::header_read_handler(boost::system::error_code ec)
    {
        if (ec)
            on_parse_(ec);

        parser_.feed(connection_.get());

        if (parser_.finished())
            on_parse_({});
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_handler::set_parameters(std::unordered_map <std::string, std::string> const& params)
    {
        params_ = params;
    }
//---------------------------------------------------------------------------------------------------------------------
    uint64_t request_handler::get_content_length() const
    {
        // TODO: use memoization.

        auto header = parser_.get_header();
        auto body_length = header.fields.find("Content-Length");

        if (body_length == header.fields.end())
            throw std::runtime_error("no content length field provided for reading body");

        // throws if Content-Length is not a number
        return boost::lexical_cast <uint64_t> (body_length->second);
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_handler::body_read_handler(boost::system::error_code ec)
    {
        if (ec)
        {
            on_finished_read_.error(ec);
            return;
        }

        sink_->write(connection_->get_read_buffer());

        auto remaining = get_content_length() - sink_->get_bytes_written();

        if (remaining == 0)
            on_finished_read_.fullfill();
        else
            connection_->read();
    }
//---------------------------------------------------------------------------------------------------------------------
    callback_wrapper& request_handler::read_body(std::ostream& stream)
    {
        // rearrange the callback for body reading.
        connection_->set_read_callback([this](boost::system::error_code ec) {
            body_read_handler(ec);
        });

        // assign a sink, which prevails the asynchronous structure.
        sink_.reset(new tcp_stream_sink(&stream));

        // write what we already have read by parsing the header
        auto body_begin = parser_.get_buffer(); // start of body
        sink_->write(body_begin.c_str(), body_begin.size());

        std::cout << get_content_length() << "\n";
        std::cout << body_begin.size() << "\n";

        // read more if more data is to be expected.
        if (get_content_length() - body_begin.size() > 0)
            connection_->read();
        else
            on_finished_read_.fullfill();

        return on_finished_read_;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string request_handler::hostname() const
    {
        if (connection_->get_parent()->get_settings().trust_proxy)
        {
            auto xhost = parser_.get_field("X-Forwarded-Host");
            if (xhost)
                return xhost.get();
        }

        auto host = parser_.get_field("Host");
        if (host)
            return host.get();

        return "";
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string request_handler::ip() const
    {
        return connection_->get_socket()->remote_endpoint().address().to_string();
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string request_handler::method() const
    {
        return parser_.get_header().verb;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string request_handler::url() const
    {
        return parser_.get_header().path;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string request_handler::param(std::string const& key) const
    {
        auto parm = params_.find(key);
        if (parm = std::end(params_))
            throw std::runtime_error("key is valid for this request");

        return parm->second;
    }
//#####################################################################################################################
}
