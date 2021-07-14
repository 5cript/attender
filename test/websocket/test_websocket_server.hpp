#pragma once

#include <attender/websocket/server/server.hpp>
#include <attender/io_context/managed_io_context.hpp>
#include <attender/io_context/thread_pooler.hpp>
#include <attender/websocket/client/client.hpp>
#include <attender/websocket/server/flexible_session.hpp>

#include <gtest/gtest.h>

#include <thread>
#include <chrono>
#include <future>
#include <iostream>

using namespace std::chrono_literals;

namespace attender::testing
{
    class WebsocketServerTests 
        : public ::testing::Test
    {
    public:
        WebsocketServerTests()
            : context_{}
        {}

    protected:
        managed_io_context <thread_pooler> context_;
    };

    
    TEST_F(WebsocketServerTests, CanStartServerWithoutError)
    {
        bool errorCalled = false;
        websocket::server server(context_.get_io_service(), [&errorCalled](auto){
            errorCalled = true;
        });
        server.start([](auto) {});
        EXPECT_FALSE(errorCalled);
    }

    TEST_F(WebsocketServerTests, CanConnectToServerAndTheAcceptCallbackIsCalled)
    {
        std::promise<bool> serverWasCalled;
        websocket::server server(context_.get_io_service(), [](...){});
        server.start([&serverWasCalled](auto) {
            serverWasCalled.set_value(true);
        });
        websocket::client client{context_.get_io_service()};
        auto port = std::to_string(server.local_endpoint().port());
        EXPECT_FALSE(client.connect_sync({.host = "localhost", .port = port}));
        EXPECT_EQ(std::future_status::ready, serverWasCalled.get_future().wait_for(std::chrono::seconds(1)));
    }

    TEST_F(WebsocketServerTests, CanReadFromClient)
    {
        websocket::server server(context_.get_io_service(), [](...){});
        std::promise<std::string> read;
        server.start([&read](std::shared_ptr<websocket::connection> sharedConnection) {
            auto& session = sharedConnection->create_session<websocket::flexible_session>();
            session.on_text_cb = [&read](auto view){
                read.set_value(std::string{view});
            };
        });
        websocket::client client{context_.get_io_service()};
        auto port = std::to_string(server.local_endpoint().port());
        EXPECT_FALSE(client.connect_sync({.host = "localhost", .port = port}));
        client.write_sync("asdf_test");
        auto future = read.get_future();
        ASSERT_EQ(std::future_status::ready, future.wait_for(std::chrono::seconds(1)));
        EXPECT_EQ(future.get(), "asdf_test");
    }

    TEST_F(WebsocketServerTests, CanWriteToClient)
    {
        std::shared_ptr<websocket::connection> connection;
        websocket::server server(context_.get_io_service(), [](...){});
        server.start([&connection](std::shared_ptr<websocket::connection> sharedConnection) {
            // keep it alive for the rest of this test.
            connection = sharedConnection;
            auto& session = sharedConnection->create_session<websocket::flexible_session>();
            session.write_text("hello");
        });
        websocket::client client{context_.get_io_service()};  
        std::promise<std::string> read;
        auto port = std::to_string(server.local_endpoint().port());
        EXPECT_FALSE(client.connect_sync({.host = "localhost", .port = port})); 
        client.listen([&](auto ec, std::string const& data)
        {
            if (ec)
                read.set_value(ec.message());
            else
                read.set_value(data);
        });     
        auto future = read.get_future();
        ASSERT_EQ(std::future_status::ready, future.wait_for(std::chrono::seconds(1)));
        EXPECT_EQ(future.get(), "hello");
    }
}