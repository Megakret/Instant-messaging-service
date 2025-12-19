#include <postpone_service.hpp>

#include <unistd.h>

PostponeService::PostponeService(UserStorage& users, os::Mutex& users_mu)
    : users_(users), users_mu_(users_mu) {}
PostponeErr PostponeService::DelaySend(std::string sender, std::string receiver,
                                       std::string msg) {
  return DelaySend(std::move(sender), std::move(receiver), std::move(msg),
                   std::chrono::system_clock::now());
}
PostponeErr PostponeService::DelaySend(std::string sender, std::string receiver,
                                       std::string msg,
                                       TimePoint time_to_send) {
  {
    std::lock_guard<os::Mutex> lk(users_mu_);
    if (users_.find(receiver) == users_.end()) {
      return PostponeErr::UserDoesntExist;
    }
  }
  std::lock_guard<os::Mutex> lk(queue_mu_);
  auto it = delayed_msgs_.find(receiver);
  if (it == delayed_msgs_.end()) {
    auto [new_it, exists] =
        delayed_msgs_.emplace(receiver, std::list<DelayedMessage>());
    it = new_it;
  }
  auto& messages = it->second;
  messages.emplace_back(std::move(sender), std::move(msg), time_to_send);
  return PostponeErr::Success;
}
void PostponeService::StartSendSchedule(std::chrono::seconds timeout) {
  int t_in_seconds = timeout.count();
  while (true) {
    sleep(t_in_seconds);
    std::lock_guard<os::Mutex> lk(queue_mu_);
    for (auto& [login, msg_list] : delayed_msgs_) {
      std::cout << "Send schedule\n";
      users_mu_.lock();
      auto user = users_.find(login)->second;
      users_mu_.unlock();
      if (!user.IsConnected()) {
        continue;
      }
      auto stream = user.StartStream();
      if (!stream) {
        std::cout << "Error in Send Schedule on sending to " << login << '\n';
        continue;
      }
      auto now = std::chrono::system_clock::now();
      auto list_it = msg_list.begin();
      for (auto it = msg_list.begin(); it != msg_list.end(); ++it) {
        const auto& msg = *it;
				std::cout << now << '\n';
				std::cout << msg.time_to_send << '\n';
        if (now <= msg.time_to_send) {
          continue;
        }
        auto status = stream->SendTo(msg.sender, msg.message, msg.time_to_send);
        if (status != handlers::SendToStatus::Success) {
          std::cout << std::format(
              "Failed to send message to user {} with error code: {}", login,
              static_cast<int>(status));
          continue;
        }
        ++it;
        msg_list.erase(std::prev(it));
        --it;
      }
    }
  }
}
