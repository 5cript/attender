#pragma once

#include <attender/io_context/managed_io_context.hpp>
#include <attender/io_context/thread_pooler.hpp>
#include <attender/http/http_server.hpp>

#include <gtest/gtest.h>

namespace attender::tests
{
    class UnsecureServer
    {
    public:
        UnsecureServer(std::function <void(boost::system::error_code)> const& errorHandler)
            : context_{}
            , server_{context_.get_io_service(), [errorHandler](auto*, auto const& ec, auto const&) {
                if (errorHandler)
                    errorHandler(ec);
                else
                    FAIL() << "Server error handler was called unexpectedly";
            }}
        {}

        ~UnsecureServer() = default;
        UnsecureServer(UnsecureServer const&) = default;
        UnsecureServer& operator=(UnsecureServer const&) = default;
        UnsecureServer(UnsecureServer&&) = default;
        UnsecureServer& operator=(UnsecureServer&&) = default;

    protected:
        managed_io_context <thread_pooler> context_;
        http_server server_;
    };
}