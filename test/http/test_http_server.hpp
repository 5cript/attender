#pragma once

#include <gtest/gtest.h>

namespace attender
{
    class HttpServerTests : public ::testing::Test
    {
    };
}

TEST_F(HttpServerTests, DummyTest)
{
    EXPECT_TRUE(true);
}