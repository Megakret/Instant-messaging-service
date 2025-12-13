#pragma once

#include <map>

#include <handlers/user.hpp>
#include <protos/main.pb.h>

namespace handlers {
class MessegingService {
public:
  messenger::ConnectResponce
  CreateConnection(const messenger::ConnectMessage &msg);
  messenger::DisconnectResponce
  CloseConnection(const messenger::DisconnectMessage &msg);
  messenger::SendResponce SendMessage(const messenger::SendMessage &msg);

private:
  // Here i will store users
  std::map<std::string, User> users_;
};
} // namespace handlers
