#include <unistd.h>

#include <gtest/gtest.h>

#include <config.hpp>
#include <handlers/handle.hpp>
#include <main_handler_loop.hpp>
#include <protos/main.pb.h>
#include <thread.hpp>

const std::string_view kUserPipe = "/tmp/";
const std::chrono::seconds kTestTimeout(2);

template <int msg_type, typename ProtoRequest>
void SendToServer(const ProtoRequest& msg,
                  const transport::PipeTransport& server) {
  handlers::Metadata md{msg_type, static_cast<int64_t>(msg.ByteSizeLong())};
  auto err = handlers::BindMetadataAndSend(msg, md, server);
  EXPECT_EQ(err, handlers::BindMdAndSendErr::Success);
}
template <typename ProtoResponse>
ProtoResponse GetFromPipe(const transport::PipeTransport& pipe) {
  ProtoResponse response;
  auto stream = pipe.StartStream();
  EXPECT_TRUE(stream);
  std::string buf(sizeof(handlers::ResponseMetadata), '\0');
  auto read_bytes = stream->Receive(buf);
  EXPECT_TRUE(read_bytes);
  EXPECT_EQ(*read_bytes, buf.length());
  auto metadata =
      reinterpret_cast<const handlers::ResponseMetadata*>(buf.c_str());
  std::string msg_buf(metadata->length, '\0');
  auto read_bytes1 = stream->Receive(msg_buf);
  EXPECT_TRUE(read_bytes1);
  EXPECT_EQ(*read_bytes1, msg_buf.length());
  EXPECT_TRUE(response.ParseFromString(msg_buf));
  return response;
}

std::string ConnectUser(const std::string& user) {
  std::string pipe_path = "/tmp/" + user;
  transport::PipeErr err;
  transport::PipeTransport user_pipe(pipe_path,
                                     transport::Read | transport::Create, err);
  {
    transport::PipeErr err;
    transport::PipeTransport server_pipe(std::string(kAcceptingPipePath),
                                         transport::Write, err);
    messenger::ConnectMessage msg;
    msg.set_login(user);
    msg.set_pipe_path(pipe_path);
    SendToServer<kConnectionMsgID>(msg, server_pipe);
  }
  {
    auto msg(GetFromPipe<messenger::ConnectResponce>(user_pipe));
    EXPECT_EQ(msg.status(), messenger::ConnectResponce::OK);
    if (msg.status() != messenger::ConnectResponce::OK) {
      std::cout << "Error on connecting: " << msg.verbose() << '\n';
    }
    return msg.reading_pipe_path();
  }
}

void DisconnectUser(const std::string& user) {
  std::string pipe_path = "/tmp/" + user;
  {
    transport::PipeErr err;
    transport::PipeTransport pipe(std::string(kAcceptingPipePath),
                                  transport::Write, err);
    messenger::DisconnectMessage msg;
    msg.set_login(user);
    msg.set_pipe_path(pipe_path);
    SendToServer<kDisconnectMsgID>(msg, pipe);
  }
  {
    transport::PipeErr err;
    transport::PipeTransport pipe(pipe_path, transport::Read, err);
    auto msg(GetFromPipe<messenger::DisconnectResponce>(pipe));
    EXPECT_EQ(msg.status(), messenger::DisconnectResponce::OK);
    if (msg.status() != messenger::DisconnectResponce::OK) {
      std::cout << "Error on disconnecting: " << msg.verbose() << '\n';
    }
  }
}
TEST(ConnectingTests, SimpleConnect) {
  ConnectUser("kret");
  EXPECT_EQ(access((std::string(kReceiverDir) + "/kret").c_str(), F_OK), 0);
  // Now disconnect
  DisconnectUser("kret");
}
TEST(ConnectingTests, TwoUsers) {
  auto biba_path = ConnectUser("biba");
  auto boba_path = ConnectUser("boba");
  DisconnectUser("biba");
  DisconnectUser("boba");
}
TEST(SendMessageTest, PingPong) {
  auto biba_path = ConnectUser("biba");
  auto boba_path = ConnectUser("boba");
  os::Thread receiver;
  auto lambda = std::function<void()>([&biba_path]() {
    transport::PipeErr err;
    transport::PipeTransport biba_pipe(biba_path, transport::Read, err);
    transport::PipeTransport server_pipe(std::string(kAcceptingPipePath),
                                         transport::Write, err);
    transport::PipeTransport answer_pipe(std::string(kUserPipe) + "biba",
                                         transport::Read | transport::Create,
                                         err);
    {
      auto msg = GetFromPipe<messenger::MessageForUser>(biba_pipe);
      EXPECT_EQ(msg.sender_login(), "boba");
      EXPECT_EQ(msg.message(), "ping");
      std::cout << "Biba received ping\n";
    }
    {
      messenger::SendMessage msg;
      msg.set_sender_login("biba");
      msg.set_receiver_login("boba");
      msg.set_message("pong");
      msg.set_pipe_path(std::string(kUserPipe) + "biba");
      SendToServer<kSendMsgID>(msg, server_pipe);
      std::cout << "Biba send pong\n";
    }
    {
      auto msg = GetFromPipe<messenger::SendResponce>(answer_pipe);
      EXPECT_EQ(msg.status(), messenger::SendResponce::OK);
      if (msg.status() != messenger::SendResponce::OK) {
        std::cout << "Sending pong error: " << msg.verbose() << '\n';
      }
      std::cout << "Biba received answer from server\n";
    }
  });
  receiver.RunLambda(&lambda);
  transport::PipeErr err;
  transport::PipeTransport boba_pipe(boba_path, transport::Read, err);
  transport::PipeTransport server_pipe(std::string(kAcceptingPipePath),
                                       transport::Write, err);
  transport::PipeTransport answer_pipe(std::string(kUserPipe) + "boba",
                                       transport::Read | transport::Create,
                                       err);
  {
    messenger::SendMessage msg;
    msg.set_sender_login("boba");
    msg.set_receiver_login("biba");
    msg.set_message("ping");
    msg.set_pipe_path(std::string(kUserPipe) + "boba");
    SendToServer<kSendMsgID>(msg, server_pipe);
    std::cout << "Boba sent ping\n";
  }
  {
    auto msg = GetFromPipe<messenger::SendResponce>(answer_pipe);
    EXPECT_EQ(msg.status(), messenger::SendResponce::OK);
    if (msg.status() != messenger::SendResponce::OK) {
      std::cout << "Sending ping error: " << msg.verbose() << '\n';
    }
    std::cout << "Boba received answer from server\n";
  }
  {
    auto msg = GetFromPipe<messenger::MessageForUser>(boba_pipe);
    EXPECT_EQ(msg.sender_login(), "biba");
    EXPECT_EQ(msg.message(), "pong");
    std::cout << "Boba got pong\n";
  }
  receiver.Join();
  DisconnectUser("biba");
  DisconnectUser("boba");
}
TEST(DelayedMessage, Simple) {
  using namespace std::chrono_literals;
  const std::string_view kMessage = "Hello";
  auto biba_path = ConnectUser("biba");
  auto boba_path = ConnectUser("boba");
  auto prev_time = std::chrono::system_clock::now();
  const int32_t time_to_send = std::chrono::duration_cast<std::chrono::seconds>(
                                   (prev_time + 2s).time_since_epoch())
                                   .count();
  const std::string kBibaAnsPath = std::string(kUserPipe) + "biba";
  const std::string kBobaAnsPath = std::string(kUserPipe) + "boba";
  transport::PipeErr err;
  transport::PipeTransport server_pipe(std::string(kAcceptingPipePath),
                                       transport::Write, err);
  {
    transport::PipeTransport answer_pipe(
        kBibaAnsPath, transport::Read | transport::Create, err);
    messenger::SendPostponedRequest req;
    req.set_receiver("boba");
    req.set_pipe_path(kBibaAnsPath);
    auto msg = req.mutable_msg();
    msg->set_sender_login("biba");
    msg->set_message(kMessage);
    msg->set_time(time_to_send);
    SendToServer<kPostponeMsgID>(req, server_pipe);
    std::cout << "Sent the message\n";
    {
      auto msg = GetFromPipe<messenger::SendPostponedResponce>(answer_pipe);
      ASSERT_EQ(msg.status(), messenger::SendPostponedResponce::OK);
      std::cout << "Got positive answer from server\n";
    }
  }
  {

    transport::PipeTransport boba_pipe(boba_path, transport::Read, err);

    auto msg = GetFromPipe<messenger::MessageForUser>(boba_pipe);
    std::cout << "Boba got message\n";
    EXPECT_EQ(msg.message(), kMessage);
    EXPECT_EQ(msg.sender_login(), "biba");
    EXPECT_EQ(msg.time(), time_to_send);
    auto now = std::chrono::system_clock::now();
    EXPECT_GE(now - prev_time, 1s);
  }
  DisconnectUser("biba");
  DisconnectUser("boba");
}
TEST(DelayedMessage, NotConnected) {
  using namespace std::chrono_literals;
  const std::string_view kMessage = "Hello";
  auto biba_path = ConnectUser("biba");
  auto boba_path = ConnectUser("boba");
  auto prev_time = std::chrono::system_clock::now();
  DisconnectUser("boba");
  const int32_t time_to_send = std::chrono::duration_cast<std::chrono::seconds>(
                                   (prev_time + 2s).time_since_epoch())
                                   .count();
  const std::string kBibaAnsPath = std::string(kUserPipe) + "biba";
  const std::string kBobaAnsPath = std::string(kUserPipe) + "boba";
  transport::PipeErr err;
  transport::PipeTransport server_pipe(std::string(kAcceptingPipePath),
                                       transport::Write, err);
  {
    transport::PipeTransport answer_pipe(
        kBibaAnsPath, transport::Read | transport::Create, err);
    messenger::SendPostponedRequest req;
    req.set_receiver("boba");
    req.set_pipe_path(kBibaAnsPath);
    auto msg = req.mutable_msg();
    msg->set_sender_login("biba");
    msg->set_message(kMessage);
    msg->set_time(time_to_send);
    SendToServer<kPostponeMsgID>(req, server_pipe);
    std::cout << "Sent the message\n";
    {
      auto msg = GetFromPipe<messenger::SendPostponedResponce>(answer_pipe);
      ASSERT_EQ(msg.status(), messenger::SendPostponedResponce::OK);
      std::cout << "Got positive answer from server\n";
    }
  }
  sleep(2);
  boba_path = ConnectUser("boba");
  {

    transport::PipeTransport boba_pipe(boba_path, transport::Read, err);

    auto msg = GetFromPipe<messenger::MessageForUser>(boba_pipe);
    std::cout << "Boba got message\n";
    EXPECT_EQ(msg.message(), kMessage);
    EXPECT_EQ(msg.sender_login(), "biba");
    EXPECT_EQ(msg.time(), time_to_send);
    auto now = std::chrono::system_clock::now();
    EXPECT_GE(now - prev_time, 1s);
  }
  DisconnectUser("biba");
  DisconnectUser("boba");
}
TEST(DelayedTest, SelfSend) {
  using namespace std::chrono_literals;
  const std::string_view kMessage = "Hello";
  auto biba_path = ConnectUser("biba");
  auto prev_time = std::chrono::system_clock::now();
  const int32_t time_to_send = std::chrono::duration_cast<std::chrono::seconds>(
                                   (prev_time + 2s).time_since_epoch())
                                   .count();
  const std::string kBibaAnsPath = std::string(kUserPipe) + "biba";
  transport::PipeErr err;
  transport::PipeTransport server_pipe(std::string(kAcceptingPipePath),
                                       transport::Write, err);
  {
    transport::PipeTransport answer_pipe(
        kBibaAnsPath, transport::Read | transport::Create, err);
    messenger::SendPostponedRequest req;
    req.set_receiver("biba");
    req.set_pipe_path(kBibaAnsPath);
    auto msg = req.mutable_msg();
    msg->set_sender_login("biba");
    msg->set_message(kMessage);
    msg->set_time(time_to_send);
    SendToServer<kPostponeMsgID>(req, server_pipe);
    std::cout << "Sent the message\n";
    {
      auto msg = GetFromPipe<messenger::SendPostponedResponce>(answer_pipe);
      ASSERT_EQ(msg.status(), messenger::SendPostponedResponce::OK);
      std::cout << "Got positive answer from server\n";
    }
  }
  {
    transport::PipeTransport biba_pipe(biba_path, transport::Read, err);
    auto msg = GetFromPipe<messenger::MessageForUser>(biba_pipe);
    std::cout << "Boba got message\n";
    EXPECT_EQ(msg.message(), kMessage);
    EXPECT_EQ(msg.sender_login(), "biba");
    EXPECT_EQ(msg.time(), time_to_send);
    auto now = std::chrono::system_clock::now();
    EXPECT_GE(now - prev_time, 1s);
  }
  DisconnectUser("biba");
}
int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  os::Thread t1;
  auto lambda =
      std::function<void()>([]() { main_handler_loop(kTestTimeout); });
  t1.RunLambda(&lambda);
  t1.Detach();
  return RUN_ALL_TESTS();
}
