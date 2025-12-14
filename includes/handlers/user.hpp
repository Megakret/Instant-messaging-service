#pragma once

#include <chrono>
#include <string>

#include <protos/main.pb.h>
#include <transport/PipeTransport.hpp>

namespace handlers {
struct SendErr {
	enum Codes{
		Success,
		SystemErr,
		SerializationErr,
	};
  Codes error_code;
  int _errno;
};
class User {
public:
  User(const std::string &login, const transport::PipeTransport &transport);
  User(const User &user) = delete;
  User(User &&) = default;
  void OnConnect();
  void OnDisconnect();
  bool IsConnected();
  transport::PipeTransport &GetTransport();
  SendErr
  SendTo(std::string sender_login, std::string message,
         std::chrono::time_point<std::chrono::system_clock> sending_time);

private:
  std::string login_;
  transport::PipeTransport transport_;
  bool is_connected_;
};
} // namespace handlers
