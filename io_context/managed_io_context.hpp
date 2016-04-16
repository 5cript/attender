#pragma once

#include "../net_core.hpp"
#include "async_model.hpp"

#include <memory>

namespace attender
{
    template <typename AsyncModel>
    class managed_io_context
    {
    public:
        constexpr managed_io_context()
            : service_{}
            , async_provider_{new AsyncModel{&service_}}
        {
            async_provider_->setup();
        }

        /**
         *  Returns the encapsulated io_service, for use.
         *  The managed_io_context does not get transferred to anything else ever,
         *  its just there to combine the io_service with some work / workers.
         *  As boost::asio supports thread pools, coroutines, etc.
         */
        constexpr asio::io_service* get_io_service() noexcept
        {
            return &service_;
        }

        managed_io_context(const managed_io_context&) = delete;
        managed_io_context& operator=(const managed_io_context&) = delete;
    private:
        asio::io_service service_;
        std::unique_ptr <AsyncModel> async_provider_;
    };
}
