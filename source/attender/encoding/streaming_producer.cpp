#include <attender/encoding/streaming_producer.hpp>

#include <iostream>

namespace attender
{
//#####################################################################################################################
    streaming_producer::streaming_producer
    (
        std::string encoding,
        std::function <void()> on_ready,
        std::function <void(boost::system::error_code)> on_error
    )
        : buffer_{}
        , encoding_{std::move(encoding)}
        , on_ready_{std::move(on_ready)}
        , on_error_{std::move(on_error)}
        , completed_{false}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    void streaming_producer::flush()
    {
        produced_data();
    }
//---------------------------------------------------------------------------------------------------------------------
    void streaming_producer::finish()
    {
        completed_.store(true);
        produced_data();
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string streaming_producer::encoding() const
    {
        return encoding_;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::size_t streaming_producer::available() const
    {
        std::lock_guard <std::recursive_mutex> guard{buffer_saver_};
        return buffer_.size();
    }
//---------------------------------------------------------------------------------------------------------------------
    char const* streaming_producer::data() const
    {
        return buffer_.data();
    }
//---------------------------------------------------------------------------------------------------------------------
    bool streaming_producer::complete() const
    {
        return completed_.load();
    }
//---------------------------------------------------------------------------------------------------------------------
    void streaming_producer::has_consumed(std::size_t size)
    {
        std::lock_guard <std::recursive_mutex> guard{buffer_saver_};
        if (buffer_.size() < size)
            buffer_.clear();
        else
            buffer_.erase(buffer_.begin(), buffer_.begin() + size);

        producer::has_consumed(size);
    }
//---------------------------------------------------------------------------------------------------------------------
    void streaming_producer::buffer_locked_do(std::function <void()> const& fn) const
    {
        std::lock_guard <std::recursive_mutex> guard{buffer_saver_};
        fn();
    }
//---------------------------------------------------------------------------------------------------------------------
    void streaming_producer::on_error(boost::system::error_code ec)
    {
        on_error_(ec);
    }
//---------------------------------------------------------------------------------------------------------------------
    void streaming_producer::start_production()
    {
        on_ready_();
        if (available() > 0)
            produced_data();
    }
//#####################################################################################################################
}
