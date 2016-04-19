#include "request.hpp"

#include "tcp_connection.hpp"

#include <string>
#include <boost/iostreams/stream.hpp>
#include <iostream>

namespace attender
{
//#####################################################################################################################
    request_handler::request_handler(std::shared_ptr <tcp_connection> connection)
        : connection_(std::move(connection))
    {
        connection_->set_read_callback([this](boost::system::error_code ec) {
            header_read_handler(ec);
        });
        connection_->read();
    }
//---------------------------------------------------------------------------------------------------------------------
    void request_handler::header_read_handler(boost::system::error_code ec)
    {
        if (ec)
        {
            std::cerr << ec.message() << "\n";
        }
        std::cerr << "read something!\n";

        tcp_stream_device device {connection_.get()};
        boost::iostreams::stream <tcp_stream_device> stream(device);

        do
        {
            std::string line;
            auto hit_eof = std::getline(stream, line, '\n').eof();
            header_buffer_ += line;
            if (line.back() == '\r') {
                header_buffer_ += "\r\n";
                line.pop_back();
            }

            if (!hit_eof && line.empty())
                break;
            else if (hit_eof) {
                connection_->read();
                break;
            }
        } while(true);

        std::cout << header_buffer_ << "\n";

        connection_->write("HTTP/1.1 200 OK");
    }
//#####################################################################################################################
}
