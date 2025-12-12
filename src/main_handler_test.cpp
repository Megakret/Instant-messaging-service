#include <fstream>
#include <thread>
#include <unistd.h>

#include <gtest/gtest.h>

#include <config.hpp>
#include <main_handler_loop.hpp>
#include <protos/main.pb.h>

const std::string_view kUserPipe = "/tmp/test_user";

TEST(ConnectingTests, SimpleConnect) {
  mkfifo("/tmp/test_user", 0600);
  std::thread t1(main_handler_loop);
  {
    std::fstream server_pipe(kAcceptingPipePath.data(),
                             std::ios::out | std::ios::binary);
    server_pipe << static_cast<int>(kConnectionMsgID);
    messenger::ConnectMessage msg;
    msg.set_login("kret");
    msg.set_pipe_path(kUserPipe);
    EXPECT_TRUE(msg.SerializeToOstream(&server_pipe));
  }
  {
    std::fstream client_pipe(kUserPipe.data(), std::ios::in | std::ios::binary);
    messenger::ConnectResponce msg;
    EXPECT_TRUE(msg.ParseFromIstream(&client_pipe));
    EXPECT_EQ(msg.status(), messenger::ConnectResponce::OK);
    std::cout << msg.verbose();
  }
  EXPECT_EQ(access((std::string(kReceiverDir) + "/kret").c_str(), F_OK), 0);
  t1.detach();
  // Now disconnect
  {
    std::fstream server_pipe(kAcceptingPipePath.data(),
                             std::ios::out | std::ios::binary);
    server_pipe << static_cast<int>(kDisconnectMsgID);
    messenger::DisconnectMessage msg;
    msg.set_login("kret");
    msg.set_pipe_path(kUserPipe);
    EXPECT_TRUE(msg.SerializeToOstream(&server_pipe));
  }
  {
    std::fstream client_pipe(kUserPipe.data(), std::ios::in | std::ios::binary);
    messenger::DisconnectResponce msg;
    EXPECT_TRUE(msg.ParseFromIstream(&client_pipe));
    EXPECT_EQ(msg.status(), messenger::DisconnectResponce::OK);
  }
  EXPECT_NE(access((std::string(kReceiverDir) + "/kret").c_str(), F_OK), 0);
}
