#include <gtest/gtest.h>

#include <format>
#include <string.h>

#include <config.hpp>
#include <os/thread.hpp>
#include <transport/PipeTransport.hpp>

namespace transport {
constexpr std::string_view kHelloWorld("Hello world");
constexpr std::string_view kMsgTemplate("Hello world{}");
TEST(HelloWorld, HelloWorld) {
  transport::PipeErr err;
  PipeTransport transport(std::string(kAcceptingPipePath), Read, err);
  ASSERT_EQ(err, transport::PipeErr::Success);
  os::Thread t1;
  auto lambda = std::function<void()>([]() {
    transport::PipeErr err;
    PipeTransport transport(std::string(kAcceptingPipePath), Write, err);
    ASSERT_EQ(err, transport::PipeErr::Success);
    std::span<const char> buf(kHelloWorld.data(), kHelloWorld.size());
    auto err_send = transport.Send(buf);
    ASSERT_TRUE(err_send);
  });
  t1.RunLambda(&lambda);
  std::array<char, 100> buffer;
  auto read_bytes = transport.Receive(buffer);
  t1.Join();
  ASSERT_TRUE(read_bytes);
  EXPECT_EQ(*read_bytes, kHelloWorld.size());
  for (std::size_t i = 0; i < kHelloWorld.size(); ++i) {
    EXPECT_EQ(buffer[i], kHelloWorld[i]);
  }
}
TEST(WrongPermissions, ReadFromWrite) {
  transport::PipeErr err;
  PipeTransport transport(std::string(kAcceptingPipePath), Write, err);
  ASSERT_EQ(err, transport::PipeErr::Success);
  std::span<char> buf;
  auto read_bytes = transport.Receive(buf);
  EXPECT_FALSE(read_bytes);
}
const std::size_t stream_message_size = kMsgTemplate.size() - 1;
TEST(Stream, HelloWorld) {
  transport::PipeErr err;
  PipeTransport transport(std::string(kAcceptingPipePath), Read, err);
  ASSERT_EQ(err, transport::PipeErr::Success);
  os::Thread t;
  auto lambda = std::function<void()>([]() {
    transport::PipeErr err;
    PipeTransport transport(std::string(kAcceptingPipePath), Write, err);
    ASSERT_EQ(err, transport::PipeErr::Success);
    auto stream = transport.StartStream();
    ASSERT_TRUE(stream);
    for (int i = 0; i < 5; ++i) {
      std::string msg = std::format(kMsgTemplate, i);
      std::span<const char> buf(msg);
      auto err_send = stream->Send(buf);
      ASSERT_TRUE(err_send);
    }
  });
  t.RunLambda(&lambda);
  auto stream = transport.StartStream();
  ASSERT_TRUE(stream);
  std::array<char, stream_message_size> buf;
  for (int i = 0; i < 5; ++i) {
    std::string msg = std::format(kMsgTemplate, i);
    auto recv_bytes = stream->Receive(buf);
    ASSERT_TRUE(recv_bytes);
    ASSERT_EQ(recv_bytes, msg.length());
    for (std::size_t i = 0; i < msg.length(); ++i) {
      EXPECT_EQ(buf[i], msg[i]);
    }
  }

  t.Join();
}
}; // namespace transport
