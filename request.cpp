#include "request.hpp"

#include "tcp_connection.hpp"
#include "tcp_read_sink.hpp"

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

        auto* ptr = connection_.get();
        parser_.feed(connection_.get());

        if (parser_.finished())
            on_parse_({});
    }
//---------------------------------------------------------------------------------------------------------------------
    readable_request_handler request_handler::initiate_read(read_callback callback)
    {
        return readable_request_handler(this, std::move(callback)); // DO NOT BREAK HERE!!!!
    }
//---------------------------------------------------------------------------------------------------------------------
    uint64_t request_handler::get_content_length() const
    {
        // TODO: use memoization.

        auto header = parser_.get_header();
        auto body_length = header.fields.find("Content-Length");

        if (body_length == header.fields.end())
            throw std::runtime_error("no content length field provided for reading body");

        // throws if Content-Length is not a anumber
        return boost::lexical_cast <uint64_t> (body_length->second);
    }
//#####################################################################################################################
    readable_request_handler::readable_request_handler(request_handler* parent, read_callback callback)
        : res_(parent)
        , callback_(std::move(callback))
    {
        res_->connection_->set_read_callback([this](boost::system::error_code ec) {
            body_read_handler(ec);
        });
    }
//---------------------------------------------------------------------------------------------------------------------
    void readable_request_handler::body_read_handler(boost::system::error_code ec)
    {
        if (ec)
        {
            std::cerr << "-";
        }

        res_->sink_->write(res_->connection_->get_read_buffer());

        auto remaining = res_->get_content_length() - res_->sink_->get_bytes_written();

        if (remaining == 0)
            callback_({});
        else
            res_->connection_->read();
    }
//---------------------------------------------------------------------------------------------------------------------
    void readable_request_handler::read_body(std::ostream& stream)
    {
        // assign a sink, which prevails the asynchronous structure.
        res_->sink_.reset(new tcp_stream_sink(&stream));

        // write what we already have read by parsing the header
        auto body_begin = res_->parser_.get_buffer(); // start of body
        res_->sink_->write(body_begin.c_str(), body_begin.size());

        std::cout << res_->get_content_length() << "\n";
        std::cout << body_begin.size() << "\n";

        // read more if more data is to be expected.
        if (res_->get_content_length() - body_begin.size() > 0)
            res_->connection_->read();
        else
            callback_({});
    }
//#####################################################################################################################
}
