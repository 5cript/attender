#pragma once

#include <attender/net_core.hpp>
#include <boost/system/error_code.hpp>

#include <cstddef>
#include <string_view>
#include <mutex>
#include <functional>
#include <queue>

namespace attender::websocket
{

class connection;

class session_base
{
public:
    session_base(connection* owner);

    virtual void on_close() = 0;
    virtual void on_text(std::string_view data) = 0;
    virtual void on_binary(char const* begin, std::size_t amount) = 0;
    virtual void on_error(boost::system::error_code ec, char const* where) = 0;
    virtual void on_write_complete(std::size_t bytes_transferred);

    void close_connection();
    
    /**
     * Tries to write immediately and can fail if writes are in progress.
     */
    bool write_text(std::string_view text, std::function<void(session_base*, std::size_t)> const& on_complete = {});

    /**
     * Tries to write immediately and can fail if writes are in progress.
     */
    bool write_binary(char const* data, std::size_t amount, std::function<void(session_base*, std::size_t)> const& on_complete = {});

    /**
     * Puts a write on a sequentialized queue. The passed function might be called from 
     * an undefined asynchronous context or immediately if no writes operations are on the queue.
     */
    void enqueue_write(std::function<void(session_base*, std::size_t)> writeOp);

    virtual ~session_base() = default;

private:
    bool write_common(char const* begin, std::size_t amount, std::function<void(session_base*, std::size_t)> const& on_complete);

private:
    connection* owner_;
    std::mutex queue_mut_;
    std::queue<std::function<void(session_base* session, std::size_t)>> enqueued_writes_;
};

}