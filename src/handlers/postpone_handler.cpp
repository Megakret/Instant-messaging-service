#include <handlers/postpone_handlers.hpp>

namespace handlers {
PostponeMessageHandler::PostponeMessageHandler(PostponeService& service)
    : service_(service) {}

messenger::SendPostponedResponce
PostponeMessageHandler::operator()(messenger::SendPostponedRequest& msg) {
  messenger::SendPostponedResponce responce;
  auto seconds = std::chrono::seconds(msg.msg().time());
  auto time = std::chrono::time_point<std::chrono::system_clock>(seconds);
  auto err = service_.DelaySend(std::move(msg.msg().sender_login()),
                                std::move(msg.receiver()),
                                std::move(msg.msg().message()), time);
  if (err != PostponeErr::Success) {
    responce.set_status(messenger::SendPostponedResponce::ERROR);
    responce.set_verbose("receiver doesnt exist");
    return responce;
  }
  responce.set_status(messenger::SendPostponedResponce::OK);
  responce.set_verbose("receiver doesnt exist");
  return responce;
}
} // namespace handler
