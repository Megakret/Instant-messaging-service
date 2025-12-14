#include <handlers/user.hpp>

#include <handlers/handle.hpp>

namespace handlers {

User::User(const std::string &login, const transport::PipeTransport &transport)
    : login_(login), transport_(transport), is_connected_(true) {}
void User::OnConnect() { is_connected_ = true; }
void User::OnDisconnect() { is_connected_ = false; }
bool User::IsConnected() { return is_connected_; }
transport::PipeTransport &User::GetTransport() { return transport_; }
SendErr
User::SendTo(std::string sender_login, std::string message,
             std::chrono::time_point<std::chrono::system_clock> sending_time) {
  // TODO: add timeouts for opening user pipe
  messenger::MessageForUser msg;
  msg.set_sender_login(std::move(sender_login));
  msg.set_message(std::move(message));
  msg.set_time(std::chrono::duration_cast<std::chrono::milliseconds>(
                   sending_time.time_since_epoch())
                   .count());
  ResponseMetadata md{static_cast<int32_t>(msg.ByteSizeLong())};
  auto err = BindMetadataAndSend(msg, md, transport_);
  if (err.error_code != BindMdAndSendErr::Success) {
    if (err.error_code == BindMdAndSendErr::TransportErr) {
      return SendErr{SendErr::SerializationErr};
    } else {
      return SendErr{SendErr::SystemErr, err._errno};
    }
  }
  return SendErr{SendErr::Success};
}
}; // namespace handlers
