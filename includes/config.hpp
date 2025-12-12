#pragma once

#include <string_view>

const std::string_view kPipesDir = "/tmp/chat";
const std::string_view kAcceptingPipePath = "/tmp/chat/main_pipe";
const std::string_view kReceiverDir = "/tmp/chat/receivers";
const int kConnectionMsgID = 0;
const int kDisconnectMsgID = 1;
const int kSendMsgID = 2;
const int kPlanSendMsgID = 3;
