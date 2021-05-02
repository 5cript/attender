#pragma once

#include "websocket_test_server.hpp"

#include <attender/io_context/managed_io_context.hpp>
#include <attender/io_context/thread_pooler.hpp>
#include <attender/websocket/websocket_client.hpp>
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

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

    protected:
        managed_io_context <thread_pooler> context_;
        NodeWebSocketServer server_;
        websocket::client client_;
    };

    TEST_F(WebsocketClientTests, WebsocketClientCanConnectToServer)
    {
        server_.start("echo_server");
        auto result = client_.connect_sync({.host = "localhost", .port = "45458"});
        EXPECT_EQ(result.value(), boost::system::errc::success);
    }

    TEST_F(WebsocketClientTests, WebsocketClientCannotConnectToNonRunningServer)
    {
        client_.set_timeout({.handshake_timeout = 500ms});
        auto result = client_.connect_sync({.host = "localhost", .port = "45458"});
        EXPECT_EQ(result.value(), boost::system::errc::success);
    }
}