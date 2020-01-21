#pragma once

#include <boost/system/error_code.hpp>

#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

namespace attender
{
    /**
     *  This represents an object that produces sendable streamable data.
     */
    class producer
    {
    private:
        std::function <void()> on_produce_{};
        std::function <void(boost::system::error_code)> on_finish_{};
        mutable std::mutex on_produce_protect_{};
        std::atomic_bool consuming_{false};

    protected:
        /**
         *  Call this if the amount of available data increased.
         */
        void produced_data() const;

    public:
        virtual ~producer();

        void set_finish_callback(std::function <void(boost::system::error_code)> const& on_finish);

        /**
         *  Returns the amount of hexadecimal characters when base10Number is converted to string very fast.
         */
        static std::size_t hexlen(std::size_t base10Number);

        /**
         *  Returns false when the connection is not consuming.
         */
        bool has_consumer_attached() const;

        /**
         *  Waits for setup on the connection
         *
         *  @return returns false on timeout
         */
        bool wait_for_consumer(std::chrono::milliseconds timeout = std::chrono::milliseconds{1000}) const;

        /**
         *  Returns the encoding the producer provides.
         */
        virtual std::string encoding() const = 0;

        /**
         *  The amount of available data in the buffer.
         */
        virtual std::size_t available() const = 0;

        /**
         *  A pointer to the begin of the buffer.
         */
        virtual char const* data() const = 0;

        /**
         *  Everything within the supplied function is guarded by a mutex that also guards other buffer accesses.
         */
        virtual void buffer_locked_do(std::function <void()> const&) const = 0;

        /**
         *  Returns whether the data production has ended.
         */
        virtual bool complete() const = 0;

        /**
         *  The base class calls this function to signal that some data has been consumed.
         *  Always call the base class function at the end of your implementation.
         */
        virtual void has_consumed(std::size_t size);

        /**
         *  This function is called when the write function returns an error.
         *  Use this to stop any production.
         */
        virtual void on_error(boost::system::error_code) = 0;

        /**
         *  The server calls this function when a consumer is attached.
         */
        virtual void start_production() = 0;

        /**
         *  The server calls this function for teardown purposes.
         *  This usually means that the connection is dead.
         *
         *  Call base class function if you override
         */
        virtual void end_production(boost::system::error_code);

        /**
         *  Set a callback for when new data is available.
         */
        void set_on_produce_cb(std::function <void()> cb);
    };
}
