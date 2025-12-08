#include<fstream>
#include<thread>

#include<gtest/gtest.h>

#include<main_handler_loop.hpp>
#include<config.hpp>
#include<protos/main.pb.h>

const std::string_view kUserPipe = "/tmp/test_user_pipe";

TEST(ConnectingTests, SimpleConnect){
	std::thread t1(main_handler_loop);	
	{
	std::fstream server_pipe(kAcceptingPipePath.data(), std::ios::out | std::ios::binary);
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
		std::string verbose = msg.verbose();
		EXPECT_EQ(verbose, "Hello, world");
	}
	t1.detach();
}
