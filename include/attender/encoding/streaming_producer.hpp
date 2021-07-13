#pragma once

#include "producer.hpp"

#include <atomic>
#include <string>
#include <algorithm>
#include <functional>
#include <string_view>
#include <utility>
#include <charconv>

namespace attender
{
    /**
     *  An example producer that uses chunked encoding for data that is streamed into the connection and
     *  then at some point completes.
     */
    class streaming_producer : public producer
    {
    public:
        streaming_producer
        (
            std::string encoding,
            std::function <void()> on_ready,
            std::function <void(boost::system::error_code)> on_error
        );

        std::string encoding() const override;
        std::size_t available() const override;
        char const* data() const override;
        bool complete() const override;
        void has_consumed(std::size_t size) override;
        void on_error(boost::system::error_code) override;
        void start_production() override;

        /**
         *  Indicate that data was produced and shall be written.
         */
        void flush();

        // requires container that has size(), begin() and end().
        template <typename T>
        friend streaming_producer& operator<<(streaming_producer& stream, T const& data)
        {
            // guard
            std::lock_guard <std::recursive_mutex> guard{stream.buffer_saver_};

            // calculate size and offset
            stream.buffer_.resize(stream.buffer_.size() + data.size());
            std::copy(std::begin(data), std::end(data), std::end(stream.buffer_) - data.size());

            stream.produced_data();
            return stream;
        }

        friend streaming_producer& operator<<(streaming_producer& stream, char const* nullterminated)
        {
            return operator<<(stream, std::string_view{nullterminated});
        }

        template <typename T>
        friend std::enable_if_t <std::is_integral_v <T>, streaming_producer&> operator<<(streaming_producer& stream, T integral)
        {
            return operator<<(stream, std::to_string(integral));
        }

        void buffer_locked_do(std::function <void()> const&) const override;

        /**
         *  Set stream as complete.
         */
        void finish();

    private:
        std::vector <char> buffer_;
        std::string encoding_;
        std::function <void()> on_ready_;
        std::function <void(boost::system::error_code)> on_error_;
        std::atomic_bool completed_;
        mutable std::recursive_mutex buffer_saver_;
    };
}
