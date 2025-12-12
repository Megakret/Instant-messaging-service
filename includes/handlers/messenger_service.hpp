#pragma once

#include <map>

#include <protos/main.pb.h>
#include <handlers/user.hpp>

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
	std::map<std::string, User> users_;
};
} // namespace handler
