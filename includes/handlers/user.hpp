#pragma once

#include <mutex>
#include <string>

#include <protos/main.pb.h>

namespace handlers {
const int kSuccess = 0;
struct SendErr {
  int error_code;
  int _errno;
};
class User {
public:
  User(const std::string &login, const std::string& recv_pipe_name);
  User(const User &user) = delete;
	User(User&&) = default;
  void OnConnect();
  void OnDisconnect();
  bool IsConnected();
  std::string GetRecvPipeName();
  SendErr SendTo(const std::string &sender_login, const std::string &message,
                 int time);

private:
  std::string login_;
  std::string recv_pipe_name_;
	//have to use unique_ptr to move User into the map
	std::unique_ptr<std::mutex> write_mu_;
  bool is_connected_;
};
} // namespace handlers
