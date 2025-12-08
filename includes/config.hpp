#pragma once

#include <string_view>

const std::string_view kAcceptingPipePath = "/tmp/test_pipe";
const int kConnectionMsgID = 0;
const int kDisconnectMsgID = 1;
const int kSendMsgID = 2;
const int kPlanSendMsgID = 3;
