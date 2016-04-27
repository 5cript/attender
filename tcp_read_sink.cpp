#include "tcp_read_sink.hpp"

#include <iostream>

namespace attender
{
//#####################################################################################################################
    uint64_t tcp_read_sink::get_bytes_written() const
    {
        return written_bytes_;
    }
//#####################################################################################################################
    tcp_stream_sink::tcp_stream_sink(std::ostream* sink)
        : sink_(sink)
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_stream_sink::write(const char* data, std::size_t size)
    {
        written_bytes_ += size;
        sink_->write(data, size);
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_stream_sink::write(std::vector <char> const& buffer)
    {
        write(buffer.data(), buffer.size());
    }
//#####################################################################################################################
    tcp_string_sink::tcp_string_sink(std::string* sink)
        : sink_(sink)
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_string_sink::write(const char* data, std::size_t size)
    {
        written_bytes_ += size;
        *sink_ += std::string{data, size};
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_string_sink::write(std::vector <char> const& buffer)
    {
        write(buffer.data(), buffer.size());
    }
//#####################################################################################################################
}
