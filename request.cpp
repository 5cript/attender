#include "request.hpp"

#include "tcp_connection.hpp"
#include "tcp_read_sink.hpp"
#include "tcp_server_interface.hpp"

#include "response.hpp"

#include <string>
#include <iostream>
#include <limits>
#include <boost/lexical_cast.hpp>

namespace attender
{
//#####################################################################################################################
    request_handler::request_handler(tcp_connection_interface* connection)
        : parser_{}
        , header_{}
        , connection_{connection}
        , sink_{nullptr}
        , on_parse_{}
        , params_{}
        , on_finished_read_{}
        , max_read_{std::numeric_limits <decltype(request_handler::max_read_)>::max()}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    request_handler::~request_handler()
    {
        std::cout << "request destroyed\n";
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
        return header_;
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_handler::header_read_handler(boost::system::error_code ec)
    {
        if (ec)
        {
            on_parse_(ec);
            return;
        }

        parser_.feed(connection_);

        // header finished
        if (parser_.finished())
        {
            header_ = parser_.get_header();
            on_parse_({});
            on_parse_ = {}; // frees shared_ptrs; TODO: FIXME?
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_handler::set_parameters(std::unordered_map <std::string, std::string> const& params)
    {
        params_ = params;
    }
//---------------------------------------------------------------------------------------------------------------------
    uint64_t request_handler::get_content_length() const
    {
        auto body_length = header_.get_field("Content-Length");

        if (!body_length)
            throw std::runtime_error("no content length field provided for reading body");

        // throws if Content-Length is not a number
        return boost::lexical_cast <uint64_t> (body_length.get());
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_handler::body_read_handler(boost::system::error_code ec)
    {
        if (ec)
        {
            on_finished_read_.error(ec);
            connection_->get_response_handler().end();
            return;
        }

        // expected = ContentLength - Amount Read Overall
        auto expected = get_content_length() - sink_->get_total_bytes_written();

        // remaining limit = Min(Amount Read Overall, Maximum Read Allowed)
        uint64_t remaining_limit = std::min(static_cast <int64_t> (max_read_) - static_cast <int64_t> (sink_->get_total_bytes_written()), 0ll);

        // limit reached?
        if (remaining_limit == 0ll)
        {
            on_finished_read_.fullfill();
            return;
        }

        // write into the sink
        sink_->write(connection_->get_read_buffer(), std::min(expected, remaining_limit));

        // remaining = ContentLength - Amount Read Overall  (after read)
        auto remaining = std::max(static_cast <int64_t> (get_content_length()) - static_cast <int64_t>(sink_->get_total_bytes_written()), 0ll);

        if (remaining == 0ll)
            on_finished_read_.fullfill();
        else
            connection_->read();
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_handler::initialize_read(uint64_t& max)
    {
        if (max == 0)
            max = std::numeric_limits <std::decay_t<decltype(max)>>::max();

        // rearrange the callback for body reading.
        connection_->set_read_callback([this](boost::system::error_code ec) {
            body_read_handler(ec);
        });

        max_read_ = max;
    }
//---------------------------------------------------------------------------------------------------------------------
    uint64_t request_handler::get_read_amount() const
    {
        return sink_->get_total_bytes_written();
    }
//---------------------------------------------------------------------------------------------------------------------
    callback_wrapper& request_handler::read_body(std::ostream& stream, uint64_t max)
    {
        // set handler and set reader maximum
        initialize_read(max);

        // assign a sink, which prevails the asynchronous structure.
        sink_.reset(new tcp_stream_sink(&stream));

        return body_read_start(max);
    }
//---------------------------------------------------------------------------------------------------------------------
    callback_wrapper& request_handler::read_body(std::string& str, uint64_t max)
    {
        // set handler and set reader maximum
        initialize_read(max);

        // assign a sink, which prevails the asynchronous structure.
        sink_.reset(new tcp_string_sink(&str));

        return body_read_start(max);
    }
//---------------------------------------------------------------------------------------------------------------------
    callback_wrapper& request_handler::body_read_start(uint64_t max)
    {
        // write what we already have read by parsing the header
        if (!parser_.is_buffer_empty())
        {
            auto from_header_buffer = std::min(max, parser_.get_buffer().length());

            auto&& body_begin = parser_.read_front(from_header_buffer); // start of body
            sink_->write(body_begin.c_str(), from_header_buffer);

            // if header buffer exhausted & more content & max not reached.
            // = read more if more data is to be expected.
            if (parser_.is_buffer_empty() && get_content_length() - from_header_buffer > 0 && from_header_buffer != max)
                connection_->read();
            else
                on_finished_read_.fullfill();
        }
        else
        {
            // do not start a read operation, if the whole content has been read.
            auto remaining = std::max(static_cast <int64_t> (get_content_length()) - static_cast <int64_t>(sink_->get_total_bytes_written()), 0ll);
            if (remaining == 0ll)
                on_finished_read_.fullfill();
            else
                connection_->read();
        }

        //std::cout << get_content_length() << "\n";
        //std::cout << body_begin.size() << "\n";

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
        return header_.get_method();
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string request_handler::url() const
    {
        return header_.get_url();
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string request_handler::param(std::string const& key) const
    {
        auto parm = params_.find(key);
        if (parm == std::end(params_))
            throw std::runtime_error("key is not valid for this request");

        return parm->second;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string request_handler::path() const
    {
        return header_.get_path();
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string request_handler::protocol() const
    {
        return "http";
    }
//---------------------------------------------------------------------------------------------------------------------
    boost::optional <std::string> request_handler::query(std::string const& key) const
    {
        return header_.get_query(key);
    }
//---------------------------------------------------------------------------------------------------------------------
    bool request_handler::secure() const
    {
        return false;
    }
//---------------------------------------------------------------------------------------------------------------------
    boost::optional <std::string> request_handler::get_header_field(std::string const& key) const
    {
        return header_.get_field(key);
    }
//#####################################################################################################################
}
