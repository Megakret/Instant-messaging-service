#include <handlers/messenger_service.hpp>

#include <chrono>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <config.hpp>

namespace handlers {

messenger::ConnectResponce
MessegingService::CreateConnection(const messenger::ConnectMessage &msg) {
  messenger::ConnectResponce responce;
  transport::ErrCreation err;
  std::string recv_pipe_path = std::string(kReceiverDir) + "/" + msg.login();
  auto it = users_.find(msg.login());
  if (it == users_.end()) {
    transport::PipeTransport transport(
        recv_pipe_path, transport::Create | transport::Write, err);
    if (err.error_code != transport::kSuccess) {
      responce.set_status(messenger::ConnectResponce::ERROR);
      responce.set_verbose("failed to create pipe");
      std::cout << "Failed to create pipe transport(CreateConnection) due to: "
                << strerror(err._errno) << '\n';
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
MessegingService::CloseConnection(const messenger::DisconnectMessage &msg) {
  messenger::DisconnectResponce responce;
  auto it = users_.find(msg.login());
  if (it == users_.end()) {
    responce.set_status(messenger::DisconnectResponce::ERROR);
    responce.set_verbose("user doesnt exits");
    return responce;
  }
  if (!it->second.IsConnected()) {
    responce.set_status(messenger::DisconnectResponce::ERROR);
    responce.set_verbose("user isn't connected");
    return responce;
  }
  it->second.OnDisconnect();
  return responce;
}
messenger::SendResponce
MessegingService::SendMessage(const messenger::SendMessage &msg) {
  messenger::SendResponce responce;
  auto it = users_.find(msg.receiver_login());
  if (it == users_.end()) {
    // TODO: normal errors
    responce.set_status(messenger::SendResponce::ERROR);
    responce.set_verbose("receiver doesnt exist");
    return responce;
  }
  User &receiver = it->second;
  if (receiver.IsConnected()) {
    const auto now = std::chrono::system_clock::now();
    auto err = receiver.SendTo(msg.sender_login(), msg.message(), now);
    if (err.error_code != SendErr::Success) {
      responce.set_status(messenger::SendResponce::ERROR);
      responce.set_verbose("error during message sending");
      return responce;
    }
    responce.set_status(messenger::SendResponce::OK);
    return responce;
  }
  // Error for now
  responce.set_status(messenger::SendResponce::ERROR);
  responce.set_verbose("receiver isnt connected");
  return responce;
}
} // namespace handlers
