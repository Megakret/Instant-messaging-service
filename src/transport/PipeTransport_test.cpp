#include <gtest/gtest.h>

#include <iostream>
#include <string.h>
#include <thread>

#include <transport/PipeTransport.hpp>

namespace transport {
const char *kTransportName = "/tmp/test";
const std::string_view kHelloWorld("Hello world");
TEST(HelloWorld, HelloWorld) {
  ErrCreation err;
  PipeTransport transport(kTransportName, Read, err);
  ASSERT_EQ(err.error_code, kSuccess);
  std::thread t([]() {
    ErrCreation err;
    PipeTransport transport(kTransportName, Write, err);
    ASSERT_EQ(err.error_code, kSuccess);
    std::span<const char> buf(kHelloWorld.data(), kHelloWorld.size());
    ErrSend err_send = transport.Send(buf);
    ASSERT_EQ(err_send.error_code, kSuccess);
  });
  std::array<char, 100> buffer;
  auto [read_bytes, err_receive] = transport.Receive(buffer);
  t.join();
  EXPECT_EQ(read_bytes, kHelloWorld.size());
  ASSERT_EQ(err_receive.error_code, kSuccess);
  for (std::size_t i = 0; i < kHelloWorld.size(); ++i) {
    EXPECT_EQ(buffer[i], kHelloWorld[i]);
  }
}
TEST(WrongPermissions, ReadFromWrite){
  ErrCreation err;
  PipeTransport transport(kTransportName, Write, err);
  ASSERT_EQ(err.error_code, kSuccess);
	std::span<char> buf;
	auto[read_bytes, err_receive] = transport.Receive(buf);
}
}; // namespace transport
