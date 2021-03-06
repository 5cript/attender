#include <attender/http/response.hpp>
#include <attender/http/http_connection.hpp>
#include <attender/http/http_server.hpp>
#include <attender/http/mime.hpp>

#include <boost/filesystem.hpp>

#include <fstream>
#include <memory>
#include <charconv>
#include <iostream>
#include <iomanip>

using namespace std::string_literals;

namespace attender
{
//#####################################################################################################################
    /**
     *  Trick to hide template in c++ file. So that http_connection.hpp is not required in the header.
     */
    template <typename T>
    static void write(response_handler* res, std::shared_ptr <T> data, std::function <void()> cleanup = [](){})
    {
        res->send_header([res, data, cleanup](boost::system::error_code ec, std::size_t){
            // end the connection, on error
            if (ec)
            {
                cleanup();
                res->end();
                return;
            }

            res->get_connection()->write(*data, [res, cleanup]([[maybe_unused]] boost::system::error_code ec, std::size_t){
                // end no matter what.
                cleanup();
                res->end();
            });
        });
    }
//---------------------------------------------------------------------------------------------------------------------
    template <typename T /*fptr: void(response_handler* res)*/>
    static void write_header_for_chunked
    (
        response_handler* res,
        T&& on_header_sent
    )
    {
        res->send_header([res, on_header_sent{std::forward <T&&>(on_header_sent)}](boost::system::error_code ec, std::size_t) {
            // end the connection, on error
            if (ec)
            {
                res->end();
                return;
            }

            on_header_sent(res);
        });
    }
//---------------------------------------------------------------------------------------------------------------------
    struct stream_keeper
    {
        stream_keeper(std::istream& stream) : stream{&stream}
        {
        }

        operator std::istream&() {return *stream;}

        std::istream* stream;
    };
//#####################################################################################################################
    response_handler::response_handler(http_connection_interface* connection) noexcept
        : connection_{connection}
        , header_{}
        , header_sent_{false}
        , observer_{}
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    response_handler::~response_handler()
    {
        if (observer_) observer_->has_died();
        //std::cout << "got killed\n";
    }
//---------------------------------------------------------------------------------------------------------------------
    http_connection_interface* response_handler::get_connection()
    {
        return connection_;
    }
//---------------------------------------------------------------------------------------------------------------------
    response_handler& response_handler::append(std::string const& field, std::string const& value)
    {
        header_.append_field(field, value);
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    response_handler& response_handler::set(std::string const& field, std::string const& value)
    {
        header_.set_field(field, value);
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    response_handler& response_handler::location(std::string const& where)
    {
        header_.set_field("Location", where);
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    response_handler& response_handler::redirect(std::string const& where, int code)
    {
        header_.set_code(code);
        return location(where);
    }
//---------------------------------------------------------------------------------------------------------------------
    response_handler& response_handler::links(std::map <std::string /* rel */, std::string /* url */> const& links)
    {
        std::string value;
        for (auto const& i : links)
        {
            value += "<";
            value += i.second;
            value += ">; rel=\"";
            value += i.first;
            value += "\", ";
        }
        value.pop_back();
        value.pop_back();

        set("Links", value);
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::send(std::string const& body)
    {
        try_set("Content-Length", std::to_string(body.length()));
        try_set("Content-Type", "text/plain");

        // fix code
        if (header_.get_code() == 204 && !body.empty())
            status(200);
        else if (header_.get_code() == 200 && body.empty())
            status(204);

        write(this, std::make_shared <std::string> (body));
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::send(std::vector <char> const& body)
    {
        try_set("Content-Length", std::to_string(body.size()));
        try_set("Content-Type", "application/octet-stream");

        // fix code
        if (header_.get_code() == 204 && !body.empty())
            status(200);
        else if (header_.get_code() == 200 && body.empty())
            status(204);

        write(this, std::make_shared <std::vector <char>> (body));
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::send(std::istream& body, std::function <void()> const& on_finish)
    {
        body.seekg(0, std::ios_base::end);
        auto size = body.tellg();
        body.seekg(0);

        try_set("Content-Length", std::to_string(size));
        try_set("Content-Type", "application/octet-stream");

        if (header_.get_code() == 204 && size > 0)
            status(200);
        else if (header_.get_code() == 200 && static_cast <size_type> (size) == 0u)
            status(204);

        write(this, std::make_shared <stream_keeper> (body), on_finish);
    }
//---------------------------------------------------------------------------------------------------------------------
    std::shared_ptr <conclusion_observer> response_handler::observe_conclusion()
    {
        if (observer_)
            return observer_;
        observer_ = std::make_shared <conclusion_observer>();
        return observer_;
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::send_chunked
    (
        producer& prod,
        std::function <void(boost::system::error_code)> const& on_finish
    )
    {
        status(200);
        try_set("Content-Encoding", prod.encoding());
        set("Transfer-Encoding", "chunked");

        write_header_for_chunked(this, [concl=std::shared_ptr <conclusion_observer>{observe_conclusion()}, this, &prod, on_finish](auto)
        {
            // header is sent, now send chunked data.
            std::function <void(std::string const& err, bool)> on_produce;
            on_produce = [this, concl, &prod, on_produce](std::string const& err, bool /*control_call*/)
            {
                if (!concl->is_alive())
                {
                    prod.end_production({boost::system::errc::connection_reset, boost::system::system_category()});
                    this->end(); // neccessary, or connection leaks in manager.
                    return;
                }

                if (!err.empty())
                {
                    this->get_connection()->write("0\r\n\r\n"s, [this, &prod](boost::system::error_code, std::size_t) {
                        prod.end_production({boost::system::errc::connection_reset, boost::system::system_category()});
                        this->end();
                    });
                    return;
                }

                if (prod.complete())
                {
                    this->get_connection()->write("0\r\n\r\n"s, [this, &prod](boost::system::error_code, std::size_t) {
                        prod.end_production({boost::system::errc::connection_reset, boost::system::system_category()});
                        this->end();
                    });
                    return;
                }

                std::size_t avail = prod.available();
                if (avail == 0)
                {
                    prod.has_consumed(0);
                    return;
                }

                std::size_t size_len = producer::hexlen(avail);

                std::vector <char> avail_bytes(avail + size_len + 4ull);
                auto [p, to_chars_error] = std::to_chars(avail_bytes.data(), avail_bytes.data() + size_len, avail, 16);

                *p = '\r';
                *(p+1) = '\n';

                auto iter = std::begin(avail_bytes) + size_len + 2;
                prod.buffer_locked_do([&prod, &avail, &iter]{
                    std::copy(prod.data(), prod.data() + avail, iter);
                });
                iter += avail;
                *iter = '\r';
                ++iter;
                *iter = '\n';

                get_connection()->write(
                    /*std::move(avail_bytes)*/ avail_bytes,
                    [&prod, avail, this](auto ec, auto)
                    {
                        if (!ec)
                        {
                            /*
                            if (amount < avail)
                            {
                                prod.on_error(ec);
                                prod.end_production(ec);
                                return;
                            }
                            */
                            prod.has_consumed(avail);
                        }
                        else
                        {
                            prod.on_error(ec);
                            prod.end_production(ec);
                            this->end();
                        }
                    }
                );
            };

            prod.set_on_produce_cb(on_produce);
            prod.set_finish_callback(on_finish);
            prod.start_production();
        });
    }
//---------------------------------------------------------------------------------------------------------------------
    bool response_handler::send_file(std::string const& fileName)
    {
        auto reader = std::make_shared <std::ifstream> (fileName, std::ios_base::binary);
        if (!reader->good())
            return false;

        type(boost::filesystem::path{fileName}.extension().string(), true);
        send(*reader.get(), [reader]{}); // binds the shared ptr to the function, extending the life time, until the write operation terminates.
        return true;
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::send_status(int code)
    {
        status(code);
        if (code != 204)
            send(header_.get_message());
        else
            end();
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::end()
    {
        if (observer_) observer_->conclude();

        send_header([this](boost::system::error_code, std::size_t){
            connection_->get_parent()->get_connections()->remove(connection_);
            // further use of this is invalid from here.
        });
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::try_set(std::string const& field, std::string const& value)
    {
        if (!header_.has_field(field))
            set(field, value);
    }
//---------------------------------------------------------------------------------------------------------------------
    response_handler& response_handler::status(int code)
    {
        header_.set_code(code);
        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::send_header(write_callback continuation)
    {
        if (observer_) observer_->conclude();

        if (header_sent_.load() == false)
        {
            header_sent_.store(true);
            connection_->write(header_.to_string(), continuation);
        }
        else
            continuation({}, 0);
    }
//---------------------------------------------------------------------------------------------------------------------
    bool response_handler::has_concluded() const
    {
        if (observer_ && observer_->has_concluded())
            return true;
        return header_sent_.load();
    }
//---------------------------------------------------------------------------------------------------------------------
    response_handler& response_handler::type(std::string const& mime, bool no_except)
    {
        auto set_type = [this, no_except](std::string const& what)
        {
            if (what.empty() && !no_except)
                throw std::invalid_argument("could not find appropriate mime type from extension");
            else if (!what.empty())
                header_.set_field("Content-Type", what);
        };

        if (mime.find('/') != std::string::npos)
            set_type(mime);
        else if (!mime.empty() && mime.front() == '.')
            set_type(extension_to_mime(mime));
        else
            set_type(search_mime(mime));

        return *this;
    }
//---------------------------------------------------------------------------------------------------------------------
    void response_handler::set_cookie(cookie ck)
    {
        header_.set_cookie(ck);
    }
//#####################################################################################################################
}
