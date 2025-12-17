#pragma once

#include <chrono>
#include <string>

#include <protos/main.pb.h>
#include <transport/PipeTransport.hpp>

namespace handlers {
enum class SendToStatus { Success, SystemErr, SerializationErr };
enum class StartStreamErr { SystemErr };
//STREAM CANT EXIST WITHOUT USER OBJECT
class UserStream {
public:
  UserStream(transport::PipeStream&& stream, std::string_view login);
  UserStream(const UserStream&) = delete;
  UserStream(UserStream&&) = default;
  SendToStatus
  SendTo(std::string sender_login, std::string message,
         std::chrono::time_point<std::chrono::system_clock> sending_time);

private:
  transport::PipeStream pipe_stream_;
  std::string_view login_;
};
class User {
public:
  User(const std::string& login, const transport::PipeTransport& transport);
  void OnConnect();
  void OnDisconnect();
  bool IsConnected();
  transport::PipeTransport& GetTransport();
  std::expected<UserStream, StartStreamErr> StartStream();
  SendToStatus
  SendTo(std::string sender_login, std::string message,
         std::chrono::time_point<std::chrono::system_clock> sending_time);

private:
  std::string login_;
  transport::PipeTransport transport_;
  bool is_connected_;
};
} // namespace handlers
