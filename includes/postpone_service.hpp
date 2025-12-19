#pragma once

#include <chrono>
#include <list>
#include <map>

#include <os/mutex.hpp>
#include <protos/main.pb.h>
#include <user_storage.hpp>

enum class PostponeErr { Success, UserDoesntExist };
class PostponeService {
private:
  using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

public:
  PostponeService(UserStorage& users, os::Mutex& users_mu_);
  PostponeErr DelaySend(std::string sender, std::string receiver,
                        std::string msg);
  PostponeErr DelaySend(std::string sender, std::string receiver,
                        std::string msg, TimePoint time_to_send);
  void StartSendSchedule(std::chrono::seconds timeout);

private:
  struct DelayedMessage {
    std::string sender;
    std::string message;
    TimePoint time_to_send;
  };
  UserStorage& users_;
  os::Mutex& users_mu_;
  os::Mutex queue_mu_;
  std::map<std::string, std::list<DelayedMessage>> delayed_msgs_;
};
