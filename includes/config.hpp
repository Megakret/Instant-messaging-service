#pragma once

#include <cstdint>
#include <string_view>

const std::string_view kPipesDir = "/tmp/chat";
const std::string_view kAcceptingPipePath = "/tmp/chat/main_pipe";
const std::string_view kReceiverDir = "/tmp/chat/receivers";
const int kConnectionMsgID = 0;
const int kDisconnectMsgID = 1;
const int kSendMsgID = 2;
const int kPostponeMsgID = 3;
struct Metadata {
  int64_t msg_type;
  int64_t length;
};
struct ResponseMetadata {
  int64_t length;
};
