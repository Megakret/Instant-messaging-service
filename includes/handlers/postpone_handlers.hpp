#pragma once

#include <postpone_service.hpp>
#include <protos/main.pb.h>

namespace handlers {
class PostponeMessageHandler {
public:
	PostponeMessageHandler(PostponeService& service);
  messenger::SendPostponedResponce
  operator()(messenger::SendPostponedRequest&);

private:
  PostponeService& service_;
};
} // namespace handler
