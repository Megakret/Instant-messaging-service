#pragma once

#include <functional>
#include <iostream>
#include <string.h>

#include <transport/PipeTransport.hpp>

namespace handlers {

struct Metadata {
  int64_t msg_type;
  int64_t length;
};
struct ResponseMetadata {
  int64_t length;
};
enum class BindMdAndSendErr { Success, TransportErr, SerializationErr };
template <typename ProtoMessage, typename MetadataT>
BindMdAndSendErr
BindMetadataAndSend(const ProtoMessage &msg, const MetadataT &metadata,
                    const transport::PipeTransport &user_transport) {
  std::vector<char> user_buf(sizeof(MetadataT) + msg.ByteSizeLong());
  std::span<char> user_buf_span(user_buf);
  std::span<char> metadata_buf = user_buf_span.subspan(0, sizeof(MetadataT));
  std::span<char> msg_buf =
      user_buf_span.subspan(sizeof(MetadataT), msg.ByteSizeLong());
  memcpy(metadata_buf.data(), &metadata, sizeof(MetadataT));
  if (!msg.SerializeToArray(msg_buf.data(), msg_buf.size())) {
    return BindMdAndSendErr::SerializationErr;
  };
  auto written_bytes = user_transport.Send(user_buf_span);
  if (!written_bytes) {
    return BindMdAndSendErr::TransportErr;
  }
  return BindMdAndSendErr{BindMdAndSendErr::Success};
}

template <typename ProtoResponse, typename ProtoRequest>
void HandleRequest(transport::PipeStream &server_stream, const Metadata &md,
                   std::function<ProtoResponse(ProtoRequest)> handler) {
  ProtoRequest request;
  std::string buf(md.length, '\0');
  auto read_bytes = server_stream.Receive(buf);
  if (!read_bytes || *read_bytes < buf.length()) {
    std::cout << "Error occured on receiving actual message in HandleRequest\n";
  }
  if (!request.ParseFromString(buf)) {
    std::cout << "Failed to accept connection due to parsing error\n";
    return;
  }
  transport::PipeErr err_creation;
  transport::PipeTransport user_transport(request.pipe_path(), transport::Write,
                                          err_creation);
  auto response = handler(request);
  ResponseMetadata resp_md{static_cast<int64_t>(response.ByteSizeLong())};
  auto err_send = BindMetadataAndSend(response, resp_md, user_transport);
  if (err_send != BindMdAndSendErr::Success) {
    if (err_send == BindMdAndSendErr::SerializationErr) {
      std::cout << "Serialization err in metadata bind\n";
      return;
    } else if (err_send == BindMdAndSendErr::TransportErr) {
      std::cout << "Error on sending a message in metadata bind\n";
      return;
    }
    std::cout << "Unknown error on handling request\n";
  }
}
enum class ErrReadMd {
  Success,
  PartialRead,
  SystemError,
  Eof,
};
std::expected<Metadata, ErrReadMd> ReadMetadata(transport::PipeStream &stream);
} // namespace handlers
