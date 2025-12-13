#include <gtest/gtest.h>

#include <format>
#include <string.h>
#include <thread>

#include <config.hpp>
#include <transport/PipeTransport.hpp>

namespace transport {
constexpr std::string_view kHelloWorld("Hello world");
constexpr std::string_view kMsgTemplate("Hello world{}");
TEST(HelloWorld, HelloWorld) {
  ErrCreation err;
  PipeTransport transport(std::string(kAcceptingPipePath), Read, err);
  ASSERT_EQ(err.error_code, kSuccess);
  std::thread t([]() {
    ErrCreation err;
    PipeTransport transport(std::string(kAcceptingPipePath), Write, err);
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
TEST(WrongPermissions, ReadFromWrite) {
  ErrCreation err;
  PipeTransport transport(std::string(kAcceptingPipePath), Write, err);
  ASSERT_EQ(err.error_code, kSuccess);
  std::span<char> buf;
  auto [read_bytes, err_receive] = transport.Receive(buf);
}
const std::size_t stream_message_size = kMsgTemplate.size() - 1;
TEST(Stream, HelloWorld) {
  ErrCreation err;
  PipeTransport transport(std::string(kAcceptingPipePath), Read, err);
  ASSERT_EQ(err.error_code, kSuccess);
  std::thread t([]() {
    ErrCreation err;
    PipeTransport transport(std::string(kAcceptingPipePath), Write, err);
    ASSERT_EQ(err.error_code, kSuccess);
    auto [stream, err_stream] = transport.StartStream();
    ASSERT_EQ(err_stream.error_code, kSuccess);
    for (int i = 0; i < 5; ++i) {
      std::string msg = std::format(kMsgTemplate, i);
      std::span<const char> buf(msg);
      auto err_send = stream.Send(buf);
      ASSERT_EQ(err_send.error_code, kSuccess);
    }
  });
  auto [stream, err_stream] = transport.StartStream();
  ASSERT_EQ(err_stream.error_code, kSuccess);
  std::array<char, stream_message_size> buf;
  for (int i = 0; i < 5; ++i) {
    std::string msg = std::format(kMsgTemplate, i);
    auto [recv_bytes, err_recv] = stream.Receive(buf);
    ASSERT_EQ(recv_bytes, msg.length());
    ASSERT_EQ(err_recv.error_code, kSuccess);
    for (std::size_t i = 0; i < msg.length(); ++i) {
      EXPECT_EQ(buf[i], msg[i]);
    }
  }

  t.join();
}
}; // namespace transport
