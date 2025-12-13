#include <fstream>
#include <thread>
#include <unistd.h>

#include <gtest/gtest.h>

#include <config.hpp>
#include <handlers/handle.hpp>
#include <main_handler_loop.hpp>
#include <protos/main.pb.h>

const std::string_view kUserPipe = "/tmp/";
template <int msg_type, typename ProtoRequest>
void SendToServer(const ProtoRequest &msg,
                  const transport::PipeTransport &server) {
  handlers::Metadata md{msg_type, static_cast<int32_t>(msg.ByteSizeLong())};
  auto err = handlers::BindMetadataAndSend(msg, md, server);
  EXPECT_EQ(err.error_code, transport::kSuccess);
}
template <typename ProtoResponse>
ProtoResponse GetFromServer(const transport::PipeTransport &pipe) {
  ProtoResponse response;
	auto [stream, err_stream] = pipe.StartStream();
	EXPECT_EQ(err_stream.error_code, transport::kSuccess);
  std::string buf(sizeof(handlers::ResponseMetadata), '\0');
  auto [read_bytes, err] = stream.Receive(buf);
  EXPECT_EQ(err.error_code, transport::kSuccess);
  EXPECT_EQ(read_bytes, buf.length());
  auto metadata =
      reinterpret_cast<const handlers::ResponseMetadata *>(buf.c_str());
  std::string msg_buf(metadata->length, '\0');
  auto [read_bytes1, err1] = stream.Receive(msg_buf);
  EXPECT_EQ(err1.error_code, transport::kSuccess);
  EXPECT_EQ(read_bytes1, msg_buf.length());
  EXPECT_TRUE(response.ParseFromString(msg_buf));
  return response;
}

std::string ConnectUser(const std::string &user) {
  std::string pipe_path = "/tmp/" + user;
  mkfifo(pipe_path.c_str(), 0600);
  {
    transport::ErrCreation err;
    transport::PipeTransport pipe(std::string(kAcceptingPipePath),
                                  transport::Write, err);
    messenger::ConnectMessage msg;
    msg.set_login(user);
    msg.set_pipe_path(pipe_path);
    SendToServer<kConnectionMsgID>(msg, pipe);
  }
  {
    transport::ErrCreation err;
    transport::PipeTransport pipe(pipe_path, transport::Read, err);
    auto msg(GetFromServer<messenger::ConnectResponce>(pipe));
    EXPECT_EQ(msg.status(), messenger::ConnectResponce::OK);
    return msg.reading_pipe_path();
  }
}

void DisconnectUser(const std::string &user) {
  std::string pipe_path = "/tmp/" + user;
  {
    transport::ErrCreation err;
    transport::PipeTransport pipe(std::string(kAcceptingPipePath),
                                  transport::Write, err);
    messenger::DisconnectMessage msg;
    msg.set_login(user);
    msg.set_pipe_path(pipe_path);
    SendToServer<kDisconnectMsgID>(msg, pipe);
  }
  {
    transport::ErrCreation err;
    transport::PipeTransport pipe(pipe_path, transport::Read, err);
    auto msg(GetFromServer<messenger::DisconnectResponce>(pipe));
    EXPECT_EQ(msg.status(), messenger::DisconnectResponce::OK);
  }
}
TEST(ConnectingTests, SimpleConnect) {
  std::thread t1(main_handler_loop);
  ConnectUser("kret");
  EXPECT_EQ(access((std::string(kReceiverDir) + "/kret").c_str(), F_OK), 0);
  t1.detach();
  // Now disconnect
  DisconnectUser("kret");
  EXPECT_NE(access((std::string(kReceiverDir) + "/kret").c_str(), F_OK), 0);
}
//TEST(SendMessageTest, PingPong) {
//  std::thread t1(main_handler_loop);
//  t1.detach();
//  auto biba_path = ConnectUser("biba");
//  auto boba_path = ConnectUser("boba");
//  std::thread receiver([&biba_path]() {
//    {
//      std::ifstream biba_pipe(biba_path, std::ios::binary);
//      messenger::MessageForUser msg;
//      EXPECT_TRUE(msg.ParseFromIstream(&biba_pipe));
//      EXPECT_EQ(msg.sender_login(), "boba");
//      EXPECT_EQ(msg.message(), "ping");
//    }
//    {
//      std::ofstream server_pipe(kAcceptingPipePath.data(), std::ios::binary);
//      messenger::SendMessage msg;
//      msg.set_sender_login("biba");
//      msg.set_receiver_login("boba");
//      msg.set_message("pong");
//      msg.set_pipe_path("/tmp/biba");
//      EXPECT_TRUE(msg.SerializeToOstream(&server_pipe));
//    }
//    {
//      std::ifstream biba_pipe("/tmp/biba", std::ios::binary);
//      messenger::SendResponce msg;
//      EXPECT_TRUE(msg.ParseFromIstream(&biba_pipe));
//      EXPECT_EQ(msg.status(), messenger::SendResponce::OK);
//    }
//  });
//  {
//    std::ofstream server_pipe(kAcceptingPipePath.data(), std::ios::binary);
//    messenger::SendMessage msg;
//    msg.set_sender_login("boba");
//    msg.set_receiver_login("biba");
//    msg.set_message("ping");
//    msg.set_pipe_path("/tmp/boba");
//    EXPECT_TRUE(msg.SerializeToOstream(&server_pipe));
//  }
//  {
//    std::ifstream boba_pipe("/tmp/boba", std::ios::binary);
//    messenger::SendResponce msg;
//    EXPECT_TRUE(msg.ParseFromIstream(&boba_pipe));
//    EXPECT_EQ(msg.status(), messenger::SendResponce::OK);
//  }
//  {
//    std::ifstream boba_pipe(boba_path, std::ios::binary);
//    messenger::MessageForUser msg;
//    EXPECT_TRUE(msg.ParseFromIstream(&boba_pipe));
//    EXPECT_EQ(msg.sender_login(), "biba");
//    EXPECT_EQ(msg.message(), "pong");
//  }
//  receiver.join();
//}
