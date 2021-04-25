#pragma once

#include "unsecure_server.hpp"
#include <attendee/attendee.hpp>

#include <gtest/gtest.h>
#include <iostream>

namespace attender::tests
{
    class HttpServerTests : public ::testing::Test
                          , public UnsecureServer
    {
    public:
        HttpServerTests()
            : UnsecureServer{{}}
        {}

    protected:
        void SetUp() override
        {
            server_.start();
            port_ = server_.get_local_endpoint().port();
        }

    protected:
        unsigned short port_;
    };

    TEST_F(HttpServerTests, ServerIsRunningAndBoundToAPort)
    {
        std::cout << "[          ] " << "Port: " << port_ << "\n";
        EXPECT_NE(port_, 0);
    }

    TEST_F(HttpServerTests, CanConnectToTheServer)
    {
        
    }
}