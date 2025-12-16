#include <handlers/user.hpp>

#include <handlers/handle.hpp>

namespace handlers {

UserStream::UserStream(transport::PipeStream&& stream, std::string_view login)
    : pipe_stream_(std::move(stream)), login_(login) {}
SendToStatus UserStream::SendTo(
    std::string sender_login, std::string message,
    std::chrono::time_point<std::chrono::system_clock> sending_time) {

  messenger::MessageForUser msg;
  msg.set_sender_login(std::move(sender_login));
  msg.set_message(std::move(message));
  msg.set_time(std::chrono::duration_cast<std::chrono::milliseconds>(
                   sending_time.time_since_epoch())
                   .count());
  ResponseMetadata md{static_cast<int32_t>(msg.ByteSizeLong())};
  auto err = BindMetadataAndSend(msg, md, pipe_stream_);
  if (err != BindMdAndSendErr::Success) {
    if (err == BindMdAndSendErr::TransportErr) {
      return SendToStatus::SystemErr;
    } else {
      return SendToStatus::SerializationErr;
    }
  }
  return SendToStatus::Success;
}
User::User(const std::string& login, const transport::PipeTransport& transport)
    : login_(login), transport_(transport), is_connected_(true) {}
void User::OnConnect() { is_connected_ = true; }
void User::OnDisconnect() { is_connected_ = false; }
bool User::IsConnected() { return is_connected_; }
std::expected<UserStream, StartStreamErr> User::StartStream() {
  auto stream = transport_.StartStream();
  if (!stream) {
    return std::unexpected(StartStreamErr::SystemErr);
  }
  return UserStream(std::move(*stream), login_);
}
transport::PipeTransport& User::GetTransport() { return transport_; }
SendToStatus
User::SendTo(std::string sender_login, std::string message,
             std::chrono::time_point<std::chrono::system_clock> sending_time) {
  auto stream = StartStream();
  if (!stream) {
    return SendToStatus::SystemErr;
  }
  stream->SendTo(std::move(sender_login), std::move(message), sending_time);
  return SendToStatus::Success;
}
}; // namespace handlers
