#include <attender/websocket/server/session_base.hpp>

#include <attender/websocket/server/connection.hpp>

namespace attender::websocket
{
//#####################################################################################################################
    session_base::session_base(connection* owner)
        : owner_{owner}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    bool session_base::write_text(std::string_view text)
    {
        if (owner_->write_in_progress_.load())
        {
            return false;
        }
        owner_->ws_.text(true);
        return write_common(text.data(), text.size());
    }
//---------------------------------------------------------------------------------------------------------------------
    bool session_base::write_binary(char const* data, std::size_t amount)
    {
        if (owner_->write_in_progress_.load())
        {
            return false;
        }
        owner_->ws_.binary(true);
        return write_common(data, amount);
    }
//---------------------------------------------------------------------------------------------------------------------
    bool session_base::write_common(char const* begin, std::size_t amount)
    {
        owner_->write_in_progress_.store(true);
        /*auto amount = */boost::asio::buffer_copy(owner_->write_buffer_.prepare(amount), boost::asio::buffer(std::string_view{begin, amount}));
        // owner_->write_buffer_.commit(amount);
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