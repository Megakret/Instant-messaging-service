#pragma once

#include <handlers/user.hpp>
#include <postpone_service.hpp>
#include <protos/main.pb.h>
#include <user_storage.hpp>
#include <os/mutex.hpp>

namespace handlers {
class MessegingService {
public:
  MessegingService(UserStorage& users, os::Mutex& users_mu, PostponeService& postponer_);
  messenger::ConnectResponce
  CreateConnection(const messenger::ConnectMessage& msg);
  messenger::DisconnectResponce
  CloseConnection(const messenger::DisconnectMessage& msg);
  messenger::SendResponce SendMessage(const messenger::SendMessage& msg);

private:
  // Here i will store users
  UserStorage& users_;
  os::Mutex& users_mu_;
  PostponeService& postponer_;
};
} // namespace handlers
