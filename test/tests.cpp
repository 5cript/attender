// #include "http/test_http_server.hpp"
// #include "http/test_header.hpp"
// #include "websocket/test_websocket_client.hpp"
// #include "websocket/test_websocket_secure_client.hpp"
#include "websocket/test_websocket_server.hpp"

#include <gtest/gtest.h>
#include <iostream>

int main(int argc, char **argv) 
{
  ::testing::InitGoogleTest(&argc, argv);
  auto result = RUN_ALL_TESTS();
#ifdef PAUSE_AT_TEST_END
  std::cin.get();
#endif
  return result;
}