#include <handlers/user.hpp>

namespace handlers {

User::User(const std::string &login, const std::string &recv_pipe_name)
    : login_(login), recv_pipe_name_(recv_pipe_name),
      write_mu_(std::make_unique<std::mutex>()), is_connected_(true) {}
void User::OnConnect() { is_connected_ = true; }
void User::OnDisconnect() { is_connected_ = false; }
bool User::IsConnected() { return is_connected_; }
std::string User::GetRecvPipeName() { return recv_pipe_name_; }
SendErr SendTo(const std::string &sender_login, const std::string &message,
               int time) {
  // Not implemented
}
}; // namespace handlers
