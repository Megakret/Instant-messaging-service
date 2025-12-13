#include <handlers/user.hpp>

#include <fstream>

namespace handlers {

User::User(const std::string &login, const std::string &recv_pipe_name)
    : login_(login), recv_pipe_name_(recv_pipe_name),
      write_mu_(std::make_unique<std::mutex>()), is_connected_(true) {}
void User::OnConnect() { is_connected_ = true; }
void User::OnDisconnect() { is_connected_ = false; }
bool User::IsConnected() { return is_connected_; }
std::string User::GetRecvPipeName() { return recv_pipe_name_; }
SendErr
User::SendTo(std::string sender_login, std::string message,
             std::chrono::time_point<std::chrono::system_clock> sending_time) {
  // TODO: add timeouts for opening user pipe
  std::fstream user_pipe(recv_pipe_name_, std::ios::out | std::ios::binary);
  messenger::MessageForUser msg;
  msg.set_sender_login(std::move(sender_login));
  msg.set_message(std::move(message));
  msg.set_time(std::chrono::duration_cast<std::chrono::milliseconds>(
                   sending_time.time_since_epoch())
                   .count());
  if (!(msg.SerializeToOstream(&user_pipe))) {
    return SendErr{kSerializationErr, errno};
  }
  user_pipe.flush();
  return SendErr{kSuccess};
}
}; // namespace handlers
