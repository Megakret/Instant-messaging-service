#pragma once

#include <chrono>
#include <mutex>
#include <string>

#include <protos/main.pb.h>

namespace handlers {
const int kSuccess = 0;
const int kSerializationErr = 1;
struct SendErr {
  int error_code;
  int _errno;
};
class User {
public:
  User(const std::string &login, const std::string &recv_pipe_name);
  User(const User &user) = delete;
  User(User &&) = default;
  void OnConnect();
  void OnDisconnect();
  bool IsConnected();
  std::string GetRecvPipeName();
  SendErr
  SendTo(std::string sender_login, std::string message,
         std::chrono::time_point<std::chrono::system_clock> sending_time);

private:
  std::string login_;
  std::string recv_pipe_name_;
  // have to use unique_ptr to move User into the map
  std::unique_ptr<std::mutex> write_mu_;
  bool is_connected_;
};
} // namespace handlers
