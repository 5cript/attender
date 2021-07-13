#pragma once

#include "websocket_test_server.hpp"
#include "../barrier.hpp"

#include <attender/io_context/managed_io_context.hpp>
#include <attender/io_context/thread_pooler.hpp>
#include <gtest/gtest.h>

#include <thread>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace attender::testing
{
    template <typename WebsocketT>
    class WebsocketClientTestBase
    {
    public:
        template <typename... Ts>
        WebsocketClientTestBase(Ts&&... ts)
            : context_{}
            , server_{}
            , client_{context_.get_io_service(), std::forward<Ts>(ts)...}
        {}
        ~WebsocketClientTestBase()
        {
            server_.stop();
        }

        std::unordered_map <std::string, std::string> readBlock()
        {
            std::shared_ptr<Barrier> readSyncBarrier = std::make_shared<Barrier>();

            std::unordered_map <std::string, std::string> result;
            boost::system::error_code code;

            server_.read([this, &result, &code, readSyncBarrier](auto const& ec, auto const& props) 
            {
                readSyncBarrier->pass([&result, &code, &ec, &props](){
                    result = props;
                    code = ec;
                });
            });

            bool res = readSyncBarrier->halt(std::chrono::seconds{2});
            if (res != true)
                throw std::runtime_error("read wait timed out");

            if (code)
                throw std::system_error(code.value(), std::system_category());

            return result;
        }

        void readSome()
        {
            server_.read([](auto const&, auto const&) 
            {
            });
        }

        std::optional<std::string> waitForVariable(std::string const& key, std::chrono::milliseconds maxWaitTime = std::chrono::seconds{5})
        {
            std::string value;
            for (
                auto startTime = std::chrono::high_resolution_clock::now();
                (std::chrono::high_resolution_clock::now() - startTime) < maxWaitTime;
            )
            {
                const auto properties = readBlock();

                auto iter = properties.find(key);
                if (iter != std::end(properties))
                {
                    value = iter->second;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds{100});
            }
            if (value.empty())
                return std::nullopt;
            return {value};
        }

        std::optional<unsigned short> waitForPort(std::chrono::milliseconds maxWaitTime = std::chrono::seconds{5})
        {
            auto res = waitForVariable("port", maxWaitTime);
            if (!res)
                return std::nullopt;
            return {std::stoul(*res)};
        }

        std::string startServerAndWaitForPort(bool secure)
        {
            server_.start("echo_server", *context_.get_io_service(), secure);
            auto port = waitForPort();
            if (!port)
                [](){FAIL() << "Test server did not output its port.";}();
            return std::to_string(*port);
        }

    protected:
        managed_io_context <thread_pooler> context_;
        NodeWebSocketServer server_;
        WebsocketT client_;
    };

}