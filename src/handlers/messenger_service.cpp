#include <handlers/messenger_service.hpp>

#include <chrono>
#include <fstream>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <config.hpp>

namespace handlers {

messenger::ConnectResponce
MessegingService::CreateConnection(const messenger::ConnectMessage &msg) {
  messenger::ConnectResponce responce;
  std::string recv_pipe_path = std::string(kReceiverDir) + "/" + msg.login();
  int status = mkfifo(recv_pipe_path.c_str(), 0600);
  if (status == -1 && errno != EEXIST) {
    // TODO: add real error
    responce.set_status(messenger::ConnectResponce::ERROR);
    responce.set_verbose(strerror(errno));
    return responce;
  }
  auto it = users_.find(msg.login());
  if (it == users_.end()) {
    users_.insert(
        std::make_pair(msg.login(), User(msg.login(), recv_pipe_path)));
  } else {
    it->second.OnConnect();
  }
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
  int status = remove(it->second.GetRecvPipeName().c_str());
  if (status == -1) {
    std::cout << "Couldn't remove the fifo\n";
  }
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
    std::cout << "Sending from" << msg.sender_login() << "to "
              << msg.receiver_login() << '\n';
    if (err.error_code != kSuccess) {
      responce.set_status(messenger::SendResponce::ERROR);
      responce.set_verbose("error during message sending");
      return responce;
    }
    responce.set_status(messenger::SendResponce::OK);
    return responce;
  } else {
    // Error for now
    responce.set_status(messenger::SendResponce::ERROR);
    responce.set_verbose("receiver isnt connected");
    return responce;
  }
}
} // namespace handlers
