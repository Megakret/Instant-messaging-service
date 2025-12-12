#pragma once

#include <map>

#include <protos/main.pb.h>

namespace handlers {
class MessegingService {
public:
  messenger::ConnectResponce
  CreateConnection(const messenger::ConnectMessage &msg);
  messenger::DisconnectResponce
  CloseConnection(const messenger::DisconnectMessage &msg);
  // SendTo not implemented yet
private:
  // Here i will store users
};
} // namespace handler
