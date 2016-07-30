#include "tcp_read_sink.hpp"

#include <iostream>

namespace attender
{
//#####################################################################################################################
    tcp_read_sink::tcp_read_sink()
        : written_bytes_{0}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
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
    void tcp_stream_sink::write(std::vector <char> const& buffer, std::size_t amount)
    {
        write(buffer.data(), std::min(buffer.size(), amount));
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
        sink_->append(data, size);
    }
//---------------------------------------------------------------------------------------------------------------------
    void tcp_string_sink::write(std::vector <char> const& buffer, std::size_t amount)
    {
        write(buffer.data(), std::min(buffer.size(), amount));
    }
//#####################################################################################################################
}
