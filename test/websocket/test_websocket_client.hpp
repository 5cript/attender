#pragma once

#include "websocket_client_test_base.hpp"

#include <attender/websocket/client/client.hpp>

using namespace std::chrono_literals;

namespace attender::testing
{
    class WebsocketClientTests 
        : public WebsocketClientTestBase<websocket::client>
        , public ::testing::Test
    {
    public:
        WebsocketClientTests()
            : WebsocketClientTestBase<websocket::client>{}
        {}
    };

    TEST_F(WebsocketClientTests, CanSynchronouslyConnectToServer)
    {
        auto port = startServerAndWaitForPort(false);
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
        auto port = startServerAndWaitForPort(false);
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

        auto port = startServerAndWaitForPort(false);
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

        auto port = startServerAndWaitForPort(false);
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

        auto port = startServerAndWaitForPort(false);
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

    TEST_F(WebsocketClientTests, CanInitiateReadAndClose)
    {
        auto port = startServerAndWaitForPort(false);
        {
            websocket::client client{context_.get_io_service()};
            client.connect_sync({.host = "localhost", .port = port});
            client.listen([](auto, std::string const&)
            {
            });
        }
        server_.stop();
    }

    TEST_F(WebsocketClientTests, ReadingEchoYieldsExpectedResult)
    {
        auto port = startServerAndWaitForPort(false);
        std::shared_ptr<Barrier> readSyncBarrier = std::make_shared<Barrier>();
        std::string recv;
        {
            websocket::client client{context_.get_io_service()};
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