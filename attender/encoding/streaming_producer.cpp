#include "streaming_producer.hpp"

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
        , avail_{0}
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
        return avail_.load();
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
        avail_.store(avail_.load() - size);
        std::lock_guard <std::recursive_mutex> guard{buffer_saver_};
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
