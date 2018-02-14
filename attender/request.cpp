#include "request.hpp"

#include "tcp_connection.hpp"
#include "tcp_read_sink.hpp"
#include "tcp_server_interface.hpp"

#include "response.hpp"

#include <string>
#include <limits>

namespace attender
{
//#####################################################################################################################
    request_handler::request_handler(tcp_connection_interface* connection) noexcept
        : parser_{}
        , header_{}
        , connection_{connection}
        , sink_{nullptr}
        , on_parse_{}
        , params_{}
        , on_finished_read_{}
        , max_read_{0}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    request_handler::~request_handler()
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
        return header_;
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_handler::header_read_handler(boost::system::error_code ec)
    {
        if (ec)
        {
            on_parse_(ec, {});
            return;
        }

        try
        {
            parser_.feed(connection_);
        }
        catch (std::bad_alloc const& exc)
        {
            on_parse_(boost::system::errc::make_error_code(boost::system::errc::not_enough_memory), exc);
            return;
        }
        catch (std::exception const& exc)
        {
            on_parse_(boost::system::errc::make_error_code(boost::system::errc::protocol_error), exc);
            return;
        }

        // header finished
        if (parser_.finished())
        {
            try
            {
                header_ = parser_.get_header();

                auto expect = header_.get_field("Expect");
                if (expect && expect.get() == "100-continue")
                {
                    connection_->write("HTTP/1.1 100 Continue\r\n\r\n", [this](boost::system::error_code ec){
                        on_parse_(ec, {});
                    });
                }
                else
                    on_parse_({}, {});
            }
            catch (std::exception const& exc)
            {
                connection_->get_response_handler().send_status(400);
                // on_parse_(boost::system::errc::make_error_code(boost::system::errc::invalid_argument));
            }
            // on_parse_ = {}; // frees shared_ptrs; TODO: FIXME?
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_handler::set_parameters(std::unordered_map <std::string, std::string> const& params)
    {
        params_ = params;
    }
//---------------------------------------------------------------------------------------------------------------------
    request_parser::buffer_size_type request_handler::get_content_length() const
    {
        auto body_length = header_.get_field("Content-Length");

        if (!body_length)
        {
            throw std::runtime_error("no content length field provided for reading body");
        }

        // throws if Content-Length is not a number
        return std::atoll (body_length.get().c_str());
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_handler::body_read_handler(boost::system::error_code ec)
    {
        if (ec)
        {
            on_finished_read_.error(ec);
            if (ec != boost::asio::error::operation_aborted)
                connection_->get_response_handler().end();
            return;
        }

        // expected = ContentLength - Amount Read Overall
        auto expected = get_content_length() - sink_->get_total_bytes_written();

        // remaining limit = Min(Amount Read Overall, Maximum Read Allowed)
        int64_t remaining_limit = 0;
        if (max_read_ != 0)
            remaining_limit = static_cast <int64_t> (max_read_) - static_cast <int64_t> (sink_->get_total_bytes_written());

        if (remaining_limit < 0) remaining_limit = 0; // should never happen.

        // limit reached?
        if (max_read_ != 0 && remaining_limit == 0ll)
        {
            on_finished_read_.fullfill();
            return;
        }

        auto write_amount = expected;
        if (max_read_ != 0)
            write_amount = std::min(expected, static_cast <request_parser::buffer_size_type> (remaining_limit));

        // write into the sink
        sink_->write(connection_->get_read_buffer(), write_amount);

        // remaining = ContentLength - Amount Read Overall  (after read)
        auto remaining = std::max(static_cast <int64_t> (get_content_length()) - static_cast <int64_t>(sink_->get_total_bytes_written()), static_cast <int64_t> (0));

        if (remaining == 0ll)
            on_finished_read_.fullfill();
        else
            connection_->read();
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_handler::initialize_read(request_parser::buffer_size_type& max)
    {
        //if (max == 0)
        //    max = std::numeric_limits <std::decay_t<decltype(max)>>::max();

        // rearrange the callback for body reading.
        connection_->set_read_callback([this](boost::system::error_code ec) {
            body_read_handler(ec);
        });

        max_read_ = max;
    }
//---------------------------------------------------------------------------------------------------------------------
    request_parser::buffer_size_type request_handler::get_read_amount() const
    {
        return sink_->get_total_bytes_written();
    }
//---------------------------------------------------------------------------------------------------------------------
    callback_wrapper& request_handler::read_body(std::ostream& stream, request_parser::buffer_size_type max)
    {
        // set handler and set reader maximum
        initialize_read(max);

        // assign a sink, which prevails the asynchronous structure.
        sink_.reset(new tcp_stream_sink(&stream));

        return body_read_start(max);
    }
//---------------------------------------------------------------------------------------------------------------------
    callback_wrapper& request_handler::read_body(std::string& str, size_type max)
    {
        // set handler and set reader maximum
        initialize_read(max);

        // assign a sink, which prevails the asynchronous structure.
        sink_.reset(new tcp_string_sink(&str));

        return body_read_start(max);
    }
//---------------------------------------------------------------------------------------------------------------------
    callback_wrapper& request_handler::body_read_start(size_type max)
    {
        try
        {
            // Content-Length requirement test (411 otherwise)
            get_content_length();
        }
        catch(...)
        {
            on_finished_read_.error(boost::asio::error::invalid_argument);
            connection_->get_response_handler().send_status(411);
            return on_finished_read_;
        }

        // write what we already have read by parsing the header
        if (!parser_.is_buffer_empty())
        {
            auto from_header_buffer = std::min(max, static_cast <size_type> (parser_.get_buffer().length()));

            auto&& body_begin = parser_.read_front(from_header_buffer); // start of body
            sink_->write(body_begin.c_str(), from_header_buffer);

            // if header buffer exhausted & more content & max not reached.
            // = read more if more data is to be expected.
            if (parser_.is_buffer_empty() && (get_content_length() - from_header_buffer) > 0 && from_header_buffer < max)
                connection_->read();
            else
                on_finished_read_.fullfill();
        }
        else
        {
            // do not start a read operation, if the whole content has been read.
            auto remaining = std::max(static_cast <int64_t> (get_content_length()) - static_cast <int64_t>(sink_->get_total_bytes_written()), static_cast <int64_t> (0));
            if (remaining == 0ll)
                on_finished_read_.fullfill();
            else
                connection_->read();
        }

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
//---------------------------------------------------------------------------------------------------------------------
    boost::optional <std::string> request_handler::get_cookie_value(std::string const& name) const
    {
        return header_.get_cookie(name);
    }
//#####################################################################################################################
}
