#pragma once

#include <map>

#include <handlers/user.hpp>
#include <protos/main.pb.h>
#include <user_storage.hpp>

namespace handlers {
class MessegingService {
public:
  MessegingService(UserStorage& users, std::mutex& users_mu);
  messenger::ConnectResponce
  CreateConnection(const messenger::ConnectMessage& msg);
  messenger::DisconnectResponce
  CloseConnection(const messenger::DisconnectMessage& msg);
  messenger::SendResponce SendMessage(const messenger::SendMessage& msg);

private:
  // Here i will store users
  UserStorage& users_;
  std::mutex& users_mu_;
};
} // namespace handlers
