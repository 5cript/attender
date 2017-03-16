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
    size_type tcp_read_sink::get_total_bytes_written() const
    {
        return written_bytes_;
    }
//#####################################################################################################################
    tcp_stream_sink::tcp_stream_sink(std::ostream* sink)
        : sink_{sink}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    size_type tcp_stream_sink::write(const char* data, size_type size)
    {
        written_bytes_ += size;
        sink_->write(data, size);
        return size;
    }
//---------------------------------------------------------------------------------------------------------------------
    size_type tcp_stream_sink::write(std::vector <char> const& buffer, size_type amount)
    {
        return write(buffer.data(), std::min(buffer.size(), amount));
    }
//#####################################################################################################################
    tcp_string_sink::tcp_string_sink(std::string* sink)
        : sink_{sink}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    size_type tcp_string_sink::write(const char* data, size_type size)
    {
        written_bytes_ += size;
        sink_->append(data, size);
        return size;
    }
//---------------------------------------------------------------------------------------------------------------------
    size_type tcp_string_sink::write(std::vector <char> const& buffer, size_type amount)
    {
        return write(buffer.data(), std::min(buffer.size(), amount));
    }
//#####################################################################################################################
}
