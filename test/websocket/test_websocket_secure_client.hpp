#pragma once

#include "websocket_client_test_base.hpp"
#include "client_ssl_context.hpp"

#include <attender/websocket/websocket_secure_client.hpp>

#include <memory>
#include <iostream>

using namespace std::chrono_literals;

namespace attender::testing
{
    class WebsocketSecureClientTests 
        : public WebsocketClientTestBase<websocket::secure_client>
        , public ::testing::Test
    {
    public:
        WebsocketSecureClientTests()
            : WebsocketClientTestBase{std::unique_ptr<attender::ssl_context_interface>(new ssl_client_context())}
        {}
    };

    TEST_F(WebsocketSecureClientTests, CanSynchronouslyConnectToServer)
    {
        auto port = startServerAndWaitForPort(true);
        auto result = client_.connect_sync({.host = "localhost", .port = port});
        EXPECT_EQ(result.value(), boost::system::errc::success);
        server_.stop();
    }

    TEST_F(WebsocketSecureClientTests, CannotSynchronouslyConnectToNonRunningServer)
    {
        auto result = client_.connect_sync({.host = "localhost", .port = "45458"});
        EXPECT_NE(result.value(), boost::system::errc::success);
    }

    TEST_F(WebsocketSecureClientTests, CannotSynchronouslyConnectToInvalidInterface)
    {
        auto result = client_.connect_sync({.host = "__cheese@cake", .port = "45458"});
        EXPECT_NE(result.value(), boost::system::errc::success);
    }

    TEST_F(WebsocketSecureClientTests, CannotSynchronouslyConnectToInvalidPort)
    {
        auto result = client_.connect_sync({.host = "localhost", .port = "0:0"});
        EXPECT_NE(result.value(), boost::system::errc::success);
    }

    TEST_F(WebsocketSecureClientTests, CanConnectToServer)
    {
        Barrier connectCallBarrier;
        auto port = startServerAndWaitForPort(true);
        boost::system::error_code errorCode;
        client_.connect({.host = "localhost", .port = port}, [&connectCallBarrier, &errorCode](auto const& ec, auto const){
            connectCallBarrier.pass([&errorCode, ec](){
                errorCode = ec;
            });
        });
        connectCallBarrier.halt(10s);
        EXPECT_EQ(errorCode, boost::system::errc::success) << boost::system::system_error{errorCode.value(), boost::asio::error::get_ssl_category()}.what();
    }
    
    TEST_F(WebsocketSecureClientTests, CanWriteStringToServerSynchronously)
    {
        constexpr char const* testString = "testString";

        auto port = startServerAndWaitForPort(true);
        client_.connect_sync({.host = "localhost", .port = port});
        client_.write_sync(testString);
        auto res = waitForVariable("recv");
        if (!res)
            FAIL() << "server never received string";
        EXPECT_EQ(*res, testString);
        server_.stop();
    }
    
    TEST_F(WebsocketSecureClientTests, CanWriteStringToServer)
    {
        constexpr char const* testString = "testString";

        auto port = startServerAndWaitForPort(true);
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

    TEST_F(WebsocketSecureClientTests, CanDestructTheClientDuringWrite)
    {
        constexpr char const* testString = "testString";

        auto port = startServerAndWaitForPort(true);
        {
            decltype(client_) client{context_.get_io_service(), std::unique_ptr<attender::ssl_context_interface>(new ssl_client_context())};
            client.connect_sync({.host = "localhost", .port = port});
            client.write(testString, [](auto const& ec, std::size_t amountWritten)
            {
                EXPECT_EQ(amountWritten, std::strlen(testString));
                EXPECT_EQ(ec, boost::system::errc::success);
            });
        }
        server_.stop();
    }

    TEST_F(WebsocketSecureClientTests, CanInitiateReadAndClose)
    {
        auto port = startServerAndWaitForPort(true);
        {
            decltype(client_) client{context_.get_io_service(), std::unique_ptr<attender::ssl_context_interface>(new ssl_client_context())};
            client.connect_sync({.host = "localhost", .port = port});
            client.listen([](auto, std::string const&)
            {
            });
        }
        server_.stop();
    }

    TEST_F(WebsocketSecureClientTests, ReadingEchoYieldsExpectedResult)
    {
        auto port = startServerAndWaitForPort(true);
        std::shared_ptr<Barrier> readSyncBarrier = std::make_shared<Barrier>();
        std::string recv;
        {
            decltype(client_) client{context_.get_io_service(), std::unique_ptr<attender::ssl_context_interface>(new ssl_client_context())};
            client.connect_sync({.host = "localhost", .port = port});
            client.listen([&recv, readSyncBarrier](auto, std::string const& data)
            {
                readSyncBarrier->pass([&recv, &data](){
                    recv = data;
                });
            });
            client.write("hello", [](auto, auto){});
        }
        readSyncBarrier->halt(1s);
        EXPECT_EQ(recv, "hello");
        server_.stop();
    }
}