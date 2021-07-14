#pragma once

#include <attender/websocket/server/server.hpp>
#include <attender/io_context/managed_io_context.hpp>
#include <attender/io_context/thread_pooler.hpp>

#include <gtest/gtest.h>

#include <thread>
#include <chrono>

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

    
    TEST_F(WebsocketServerTests, CanStartServer)
    {
        bool errorCalled = false;
        websocket::server server(context_.get_io_service(), [&errorCalled](auto){
            errorCalled = true;
        });

        server.start([](auto) {});
    }
}