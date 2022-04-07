#include <attender/websocket/server/session_base.hpp>

#include <attender/websocket/server/connection.hpp>

namespace attender::websocket
{
//#####################################################################################################################
    session_base::session_base(connection* owner)
        : owner_{owner}
        , queue_mut_{}
        , enqueued_writes_{}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    void session_base::on_write_complete(std::size_t bytes_transferred)
    {        
        std::scoped_lock lock{queue_mut_};
        if (!enqueued_writes_.empty())
        {
            auto front = std::move(enqueued_writes_.front());
            enqueued_writes_.pop();
            front(this, bytes_transferred);
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    void session_base::enqueue_write(std::function<void(session_base*, std::size_t)> writeOp)
    {
        if (!owner_->write_in_progress_.load())
        {
            writeOp(this, 0);
        }
        else
        {
            std::scoped_lock lock{queue_mut_};
            enqueued_writes_.push(writeOp);
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    bool session_base::write_text(std::string_view text, std::function<void(session_base*, std::size_t)> const& on_complete)
    {
        if (owner_->write_in_progress_.load())
        {
            return false;
        }
        owner_->ws_.text(true);
        return write_common(text.data(), text.size(), on_complete);
    }
//---------------------------------------------------------------------------------------------------------------------
    bool session_base::write_binary(char const* data, std::size_t amount, std::function<void(session_base*, std::size_t)> const& on_complete)
    {
        if (owner_->write_in_progress_.load())
        {
            return false;
        }
        owner_->ws_.binary(true);
        return write_common(data, amount, on_complete);
    }
//---------------------------------------------------------------------------------------------------------------------
    void session_base::close_connection()
    {
        owner_->close();
    }
//---------------------------------------------------------------------------------------------------------------------
    bool session_base::write_common(char const* begin, std::size_t amount, std::function<void(session_base*, std::size_t)> const& on_complete)
    {
        owner_->write_in_progress_.store(true);
        if (on_complete)
            enqueue_write(on_complete);
        auto bufferCpy = boost::asio::buffer_copy(owner_->write_buffer_.prepare(amount), boost::asio::buffer(std::string_view{begin, amount}));
        owner_->write_buffer_.commit(bufferCpy);
        owner_->ws_.async_write(
            owner_->write_buffer_.data(),
            boost::asio::bind_executor(
                owner_->strand_,
                std::bind(
                    &connection::on_write,
                    owner_->shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2
                )
            )
        );
        return true;
    }
//#####################################################################################################################
}