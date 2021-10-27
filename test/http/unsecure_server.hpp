#pragma once

#include <attender/io_context/managed_io_context.hpp>
#include <attender/io_context/thread_pooler.hpp>
#include <attender/http/http_server.hpp>

#include <attendee/attendee.hpp>

#include <gtest/gtest.h>
#include <condition_variable>
#include <mutex>
#include <atomic>

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
            , port_{0}
            , client_{}
            , condVar_{}
            , condMutex_{}
        {}

        void setupAndStart(std::function <void(http_server&)> const& func = [](auto&){})
        {
            func(server_);
            server_.start();
            port_ = server_.get_local_endpoint().port();
        }

        std::string url(std::string const& subUrl)
        {
            return "http://localhost:" + std::to_string(port_) + subUrl;
        }

        template <typename BackendControl>
        void setupEmpty(BackendControl& server)
        {
            server.get("/empty", [](auto, auto res)
            {
                res->status(204).end();
            });
        }

        template <typename BackendControl>
        void setupHeaderTester(BackendControl& server, std::function <void(request_handler*)> const& tester = [](auto){})
        {
            server.get("/read_header", [tester](auto req, auto res)
            {
                tester(req);
                res->end();
            });
        }

        ~UnsecureServer() = default;
        UnsecureServer(UnsecureServer const&) = default;
        UnsecureServer& operator=(UnsecureServer const&) = default;
        UnsecureServer(UnsecureServer&&) = default;
        UnsecureServer& operator=(UnsecureServer&&) = default;

    protected:
        managed_io_context <thread_pooler> context_;
        http_server server_;
        unsigned short port_;
        attendee::request client_;
        std::condition_variable condVar_;
        std::mutex condMutex_;
    };
}