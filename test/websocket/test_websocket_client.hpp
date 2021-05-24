#pragma once

#include "websocket_test_server.hpp"
#include "../barrier.hpp"

#include <attender/io_context/managed_io_context.hpp>
#include <attender/io_context/thread_pooler.hpp>
#include <attender/websocket/websocket_client.hpp>
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <mutex>

using namespace std::chrono_literals;

namespace attender::testing
{
    class WebsocketClientTests : public ::testing::Test
    {
    public:
        WebsocketClientTests()
            : context_{}
            , server_{}
            , client_{context_.get_io_service()}
        {}
        ~WebsocketClientTests()
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

            bool res = readSyncBarrier->halt(1s);
            if (res != true)
                throw std::runtime_error("read wait timed out");

            if (code)
                throw std::system_error(code.value(), std::system_category());

            return result;
        }

        std::optional<std::string> waitForVariable(std::string const& key, std::chrono::milliseconds maxWaitTime = 5s)
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
                std::this_thread::sleep_for(100ms);
            }
            if (value.empty())
                return std::nullopt;
            return {value};
        }

        std::optional<unsigned short> waitForPort(std::chrono::milliseconds maxWaitTime = 5s)
        {
            auto res = waitForVariable("port", maxWaitTime);
            if (!res)
                return std::nullopt;
            return {std::stoul(*res)};
        }

        std::string startServerAndWaitForPort()
        {
            server_.start("echo_server", *context_.get_io_service());
            auto port = waitForPort();
            if (!port)
                [](){FAIL() << "Test server did not output its port.";}();
            return std::to_string(*port);
        }

    protected:
        managed_io_context <thread_pooler> context_;
        NodeWebSocketServer server_;
        websocket::client client_;
    };

    TEST_F(WebsocketClientTests, CanSynchronouslyConnectToServer)
    {
        auto port = startServerAndWaitForPort();
        auto result = client_.connect_sync({.host = "localhost", .port = port});
        EXPECT_EQ(result.value(), boost::system::errc::success);
        server_.stop();
    }

    TEST_F(WebsocketClientTests, CannotSynchronouslyConnectToNonRunningServer)
    {
        auto result = client_.connect_sync({.host = "localhost", .port = "45458"});
        EXPECT_NE(result.value(), boost::system::errc::success);
    }

    TEST_F(WebsocketClientTests, CannotSynchronouslyConnectToInvalidInterface)
    {
        auto result = client_.connect_sync({.host = "__cheese@cake", .port = "45458"});
        EXPECT_NE(result.value(), boost::system::errc::success);
    }

    TEST_F(WebsocketClientTests, CannotSynchronouslyConnectToInvalidPort)
    {
        auto result = client_.connect_sync({.host = "localhost", .port = "0:0"});
        EXPECT_NE(result.value(), boost::system::errc::success);
    }

    TEST_F(WebsocketClientTests, CanConnectToServer)
    {
        Barrier connectCallBarrier;
        auto port = startServerAndWaitForPort();
        boost::system::error_code errorCode;
        client_.connect({.host = "localhost", .port = port}, [&connectCallBarrier, &errorCode](auto const& ec){
            connectCallBarrier.pass([&errorCode, ec](){
                errorCode = ec;
            });
        });
        connectCallBarrier.halt(10s);
        EXPECT_EQ(errorCode, boost::system::errc::success);
    }

    TEST_F(WebsocketClientTests, CannotConnectToNonRunningServer)
    {
        Barrier connectCallBarrier;
        boost::system::error_code errorCode;
        client_.connect({.host = "localhost", .port = "45454"}, [&connectCallBarrier, &errorCode](auto const& ec){
            connectCallBarrier.pass([&errorCode, ec](){
                errorCode = ec;
            });
        });
        connectCallBarrier.halt(10s);
        EXPECT_NE(errorCode, boost::system::errc::success);
    }
    
    TEST_F(WebsocketClientTests, CanWriteStringToServerSynchronously)
    {
        constexpr char const* testString = "testString";

        auto port = startServerAndWaitForPort();
        client_.connect_sync({.host = "localhost", .port = port});
        client_.write_sync(testString);
        auto res = waitForVariable("recv");
        if (!res)
            FAIL() << "server never received string";
        EXPECT_EQ(*res, testString);
        server_.stop();
    }
    
    TEST_F(WebsocketClientTests, CanWriteStringToServer)
    {
        constexpr char const* testString = "testString";

        auto port = startServerAndWaitForPort();
        client_.connect_sync({.host = "localhost", .port = port});
        client_.write(testString, [](auto const& ec, std::size_t amountWritten)
        {
            EXPECT_EQ(amountWritten, std::strlen(testString));
            EXPECT_EQ(ec, boost::system::errc::success);
        });
        auto res = waitForVariable("recv");
        if (!res)
            FAIL() << "server never received string";
        EXPECT_EQ(*res, testString);
        server_.stop();
    }

    TEST_F(WebsocketClientTests, CanDestructTheClientDuringWrite)
    {
        constexpr char const* testString = "testString";

        auto port = startServerAndWaitForPort();
        {
            websocket::client client{context_.get_io_service()};
            client.connect_sync({.host = "localhost", .port = port});
            client.write(testString, [](auto const& ec, std::size_t amountWritten)
            {
                EXPECT_EQ(amountWritten, std::strlen(testString));
                EXPECT_EQ(ec, boost::system::errc::success);
            });
        }
        server_.stop();
    }
}