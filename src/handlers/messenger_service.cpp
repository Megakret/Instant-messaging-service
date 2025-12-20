#include <handlers/messenger_service.hpp>

#include <chrono>
#include <format>
#include <sys/stat.h>
#include <sys/types.h>

#include <config.hpp>

namespace handlers {
const std::string_view kSpecialChars = "_-";
std::string ValidateLogin(std::string_view login) {
  if (login == "") {
    return "Login cant be empty";
  }
  for (char c : login) {
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || kSpecialChars.contains(c))) {
      return std::format("Invalid character: {}", c);
    }
  }
  return "";
}
MessegingService::MessegingService(UserStorage& users, os::Mutex& users_mu,
                                   PostponeService& postponer)
    : users_(users), users_mu_(users_mu), postponer_(postponer) {}
messenger::ConnectResponce
MessegingService::CreateConnection(const messenger::ConnectMessage& msg) {
  messenger::ConnectResponce responce;
  auto validation_err = ValidateLogin(msg.login());
  if (validation_err != "") {
    responce.set_status(messenger::ConnectResponce::INVALID_ARGUMENT);
    responce.set_verbose(validation_err);
    return responce;
  }
  transport::PipeErr err;
  std::string recv_pipe_path = std::string(kReceiverDir) + "/" + msg.login();
  std::lock_guard<os::Mutex> lk(users_mu_);
  auto it = users_.find(msg.login());
  std::cout << "Connecting: " << msg.login() << '\n';
  if (it == users_.end()) {
    transport::PipeTransport transport(
        recv_pipe_path, transport::Create | transport::Write, err);
    if (err != transport::PipeErr::Success) {
      responce.set_status(messenger::ConnectResponce::ERROR);
      responce.set_verbose("failed to create pipe");
      return responce;
    }
    auto [new_it, res] = users_.insert(
        std::make_pair(msg.login(), User(msg.login(), transport)));
    new_it->second.OnConnect();
  } else {
    it->second.OnConnect();
  }
  responce.set_reading_pipe_path(recv_pipe_path);
  return responce;
}

messenger::DisconnectResponce
MessegingService::CloseConnection(const messenger::DisconnectMessage& msg) {
  messenger::DisconnectResponce responce;
  auto validation_err = ValidateLogin(msg.login());
  if (validation_err != "") {
    responce.set_status(messenger::DisconnectResponce::INVALID_ARGUMENT);
    responce.set_verbose(validation_err);
    return responce;
  }
  std::lock_guard<os::Mutex> lk(users_mu_);
  auto it = users_.find(msg.login());
  std::cout << "Disconnecting: " << msg.login() << '\n';
  if (it == users_.end()) {
    responce.set_status(messenger::DisconnectResponce::ERROR);
    responce.set_verbose("user doesnt exits");
    return responce;
  }
  if (!(it->second.IsConnected())) {
    responce.set_status(messenger::DisconnectResponce::ERROR);
    responce.set_verbose("user isn't connected");
    return responce;
  }
  it->second.OnDisconnect();
  return responce;
}
messenger::SendResponce
MessegingService::SendMessage(const messenger::SendMessage& msg) {
  messenger::SendResponce responce;
  responce.set_status(messenger::SendResponce::OK);
  {
    std::lock_guard<os::Mutex> lk(users_mu_);
    auto it = users_.find(msg.receiver_login());
    if (it == users_.end()) {
      responce.set_status(messenger::SendResponce::ERROR);
      responce.set_verbose("receiver doesnt exist");
      return responce;
    }
    User& receiver = it->second;
    if (receiver.IsConnected()) {
      const auto now = std::chrono::system_clock::now();
      auto err = receiver.SendTo(msg.sender_login(), msg.message(), now);
      if (err != SendToStatus::Success) {
        responce.set_status(messenger::SendResponce::ERROR);
        responce.set_verbose("error during message sending");
        std::cout << "SendMessage error code: " << static_cast<int>(err)
                  << '\n';
        return responce;
      }
      responce.set_status(messenger::SendResponce::OK);
      return responce;
    }
  }
  auto err = postponer_.DelaySend(msg.sender_login(), msg.receiver_login(),
                                  msg.message());
  if (err != PostponeErr::Success) {
    std::cout << "Failed to postpone message to disconnected user: "
              << static_cast<int>(err) << '\n';
    responce.set_status(messenger::SendResponce::ERROR);
    responce.set_verbose(
        "receiver is disconnected. Failed to postpone message");
    return responce;
  }
  return responce;
}
} // namespace handlers
