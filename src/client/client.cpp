#include <chrono>
#include <functional>
#include <memory>
#include <ncurses.h>
#undef OK
#include <fcntl.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <unistd.h>

#include <config.hpp>
#include <handlers/handle.hpp>
#include <os/mutex.hpp>
#include <os/thread.hpp>
#include <protos/main.pb.h>

const std::string_view kClientPipeDir = "/tmp/clients/";
const std::string_view kLogFilename = "client.log";
const std::size_t kBufferSize = 1024;
using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
const auto kCurrentZone = std::chrono::current_zone();
enum class SendToServerStatus {
  Success,
  TransportErr,
  SerializationErr,
  Error,
};
template <typename ProtoMessage>
SendToServerStatus SendMessage(const ProtoMessage& msg, int64_t request_id,
                               const transport::PipeTransport& server_pipe) {
  Metadata md{request_id, static_cast<int64_t>(msg.ByteSizeLong())};
  auto err = handlers::BindMetadataAndSend(msg, md, server_pipe);
  switch (err) {
  case handlers::BindMdAndSendErr::Success:
    return SendToServerStatus::Success;
  case handlers::BindMdAndSendErr::TransportErr:
    std::cerr << "Failed to send to server\n";
    return SendToServerStatus::TransportErr;
  case handlers::BindMdAndSendErr::SerializationErr:
    std::cerr << "Failed to send to server\n";
    return SendToServerStatus::SerializationErr;
  }
  return SendToServerStatus::Error;
}
enum class GetFromPipeErr {
  SystemError,
  Eof,
  ParseError,
};
void PrintErr(GetFromPipeErr err) {
  if (err == GetFromPipeErr::SystemError) {
    std::cerr << "Error when reading from pipe: SystemError\n";
    return;
  }
  if (err == GetFromPipeErr::Eof) {
    std::cerr << "Error when reading from pipe: EOF\n";
    return;
  }
  if (err == GetFromPipeErr::ParseError) {
    std::cerr << "Error when reading from pipe: ParseError\n";
    return;
  }
  std::cerr << "Error when reading from pipe: Unknown\n";
}
template <typename ProtoResponse>
std::expected<ProtoResponse, GetFromPipeErr>
GetFromStream(const transport::PipeStream& stream) {
  ProtoResponse response;
  std::string buf(sizeof(ResponseMetadata), '\0');
  auto read_bytes = stream.Receive(buf);
  auto metadata = reinterpret_cast<const ResponseMetadata*>(buf.c_str());
  std::string msg_buf(metadata->length, '\0');
  auto read_bytes_msg = stream.Receive(msg_buf);
  if (!read_bytes_msg) {
    auto err = read_bytes_msg.error();
    std::cerr << "Error on read from pipe: " << static_cast<int>(err) << '\n';
    return std::unexpected(GetFromPipeErr::SystemError);
  }
  if (read_bytes_msg == 0) {
    return std::unexpected(GetFromPipeErr::Eof);
  }
  if (!response.ParseFromString(msg_buf)) {
    return std::unexpected(GetFromPipeErr::ParseError);
  }
  return response;
}
template <typename ProtoResponse>
std::expected<ProtoResponse, GetFromPipeErr>
GetFromPipe(const transport::PipeTransport& pipe) {
  auto stream = pipe.StartStream();
  if (!stream) {
    return std::unexpected(GetFromPipeErr::SystemError);
  }
  return GetFromStream<ProtoResponse>(*stream);
}

struct Message {
  std::string sender;
  std::string content;
  std::chrono::system_clock::time_point timestamp;
};

class User {
private:
  os::Mutex user_mu_;
  std::string login_;
  transport::PipeTransport resp_pipe_;
  transport::PipeTransport server_pipe_;
  std::unique_ptr<transport::PipeTransport> recv_pipe_ = nullptr;
  std::function<void(Message)> callback_;

public:
  User(std::string login, std::function<void(Message)> callback)
      : login_(login), resp_pipe_(std::string(kClientPipeDir) + login,
                                  transport::Read | transport::Create),
        callback_(callback),
        server_pipe_(std::string(kAcceptingPipePath), transport::Write) {

    {
      messenger::ConnectMessage req;
      req.set_login(login);
      req.set_pipe_path(resp_pipe_.GetPath());
      auto status = SendMessage<messenger::ConnectMessage>(
          req, kConnectionMsgID, server_pipe_);
      if (status != SendToServerStatus::Success) {
        return;
      }
    }
    {
      auto msg(GetFromPipe<messenger::ConnectResponce>(resp_pipe_));
      if (!msg) {
        PrintErr(msg.error());
        return;
      }
      if (msg->status() == messenger::ConnectResponce::ERROR) {
        std::cerr << "Error from server on connecting\n";
        return;
      }
      transport::PipeErr err;
      recv_pipe_ = std::make_unique<transport::PipeTransport>(
          msg->reading_pipe_path(), transport::Read, err);
      if (err != transport::PipeErr::Success) {
        std::cerr << "Error on creating recv pipe: " << static_cast<int>(err)
                  << '\n';
        return;
      }
    }
  }
  ~User() {
    {
      messenger::DisconnectMessage req;
      req.set_login(login_);
      req.set_pipe_path(resp_pipe_.GetPath());
      auto status = SendMessage<messenger::DisconnectMessage>(
          req, kDisconnectMsgID, server_pipe_);
      if (status != SendToServerStatus::Success) {
        std::cerr << "Failed to disconnect\n";
        return;
      }
    }
    {
      auto msg(GetFromPipe<messenger::DisconnectResponce>(resp_pipe_));
      if (!msg) {
        PrintErr(msg.error());
        return;
      }
      if (msg->status() == messenger::DisconnectResponce::ERROR) {
        std::cerr << "Error from server  on disconnecting\n";
      }
    }
  }

  void SendToUser(const std::string& receiver, const std::string& message) {
    // In real implementation, send over network
    messenger::SendMessage req;
    req.set_sender_login(login_);
    req.set_receiver_login(receiver);
    req.set_message(message);
    req.set_pipe_path(resp_pipe_.GetPath());
    auto status = SendMessage(req, kSendMsgID, server_pipe_);
    if (status != SendToServerStatus::Success) {
      return;
    }
    {
      auto msg(GetFromPipe<messenger::SendResponce>(resp_pipe_));
      if (!msg) {
        PrintErr(msg.error());
        return;
      }
      if (msg->status() == messenger::SendResponce::ERROR) {
        std::cerr << "Error from server on sending message\n";
      }
    }
  }

  void SendDelayedToUser(const std::string& receiver,
                         const std::string& message, TimePoint time) {
    std::lock_guard<os::Mutex> lock(user_mu_);
    messenger::SendPostponedRequest req;
    req.set_pipe_path(resp_pipe_.GetPath());
    auto msg = req.mutable_msg();
    msg->set_sender_login(login_);
    msg->set_message(message);
    msg->set_time(
        static_cast<int32_t>(std::chrono::duration_cast<std::chrono::seconds>(
                                 time.time_since_epoch())
                                 .count()));
    req.set_receiver(receiver);
    auto status = SendMessage(req, kPostponeMsgID, server_pipe_);
    if (status != SendToServerStatus::Success) {
      return;
    }
    {
      auto msg(GetFromPipe<messenger::SendPostponedResponce>(resp_pipe_));
      if (msg->status() == messenger::SendPostponedResponce::ERROR) {
        std::cerr << "Error on sending postponed message\n";
      }
    }
  }

  void RunReceiverLoop() {
    if (recv_pipe_ == nullptr) {
      return;
    }
    while (true) {
      auto stream = recv_pipe_->StartStream();
      if (!stream) {
        std::cerr << "Failed to create receiving stream\n";
        return;
      }
      while (true) {
        auto msg(GetFromStream<messenger::MessageForUser>(*stream));
        if (!msg) {
          auto err = msg.error();
          if (err == GetFromPipeErr::SystemError) {
            std::cerr << "Error on receiving message from pipe: ";
            PrintErr(err);
            continue;
          }
          if (err == GetFromPipeErr::Eof) {
            break;
          }
        }
        callback_(Message{std::move(msg->sender_login()),
                          std::move(msg->message()),
                          TimePoint(std::chrono::seconds(msg->time()))});
      }
    }
  }
};

class ChatApp {
private:
  WINDOW* history_win_ = nullptr;
  WINDOW* input_win_ = nullptr;
  std::unique_ptr<User> user_ = nullptr;
  os::Mutex ui_mutex_;
  bool is_active_ = true;
  std::string time_to_string(TimePoint tp) {
    return std::format("{:%F %H-%M}", tp);
  }

  void print_message_to_history(const Message& msg) {
    std::lock_guard<os::Mutex> lock(ui_mutex_);
    std::string formatted = "@" + msg.sender + " [" +
                            time_to_string(msg.timestamp) + "] " + msg.content;
    wprintw(history_win_, "%s\n", formatted.c_str());
    wrefresh(history_win_);
  }

  std::string get_input_line(WINDOW* win) {
    std::array<char, kBufferSize> buffer{'\0'};
    echo();
    wgetnstr(win, buffer.data(), buffer.size() - 1);
    noecho();
    return std::string(buffer.data());
  }

  void clear_input_window() {
    werase(input_win_);
    box(input_win_, 0, 0);
    wrefresh(input_win_);
  }

  void run_input_loop() {
    wprintw(input_win_, "Enter your login: ");
    std::string login = get_input_line(input_win_);
    clear_input_window();
    if (!is_active_) {
      return;
    }
    user_ = std::make_unique<User>(login, [this](const Message& msg) {
      this->print_message_to_history(msg);
    });
    // Start receiver thread
    os::Thread receiver_thread;
    auto recv_lambda =
        std::function<void()>([this]() { this->user_->RunReceiverLoop(); });
    receiver_thread.RunLambda(&recv_lambda);
    receiver_thread.Detach();
    while (is_active_) {
      ui_mutex_.lock();
      mvwprintw(
          input_win_, 1, 2,
          "Select the message you want to send (1: instant, 2: delayed): ");
      wrefresh(input_win_);
      ui_mutex_.unlock();

      std::string choice = get_input_line(input_win_);
      ui_mutex_.lock();
      clear_input_window();
      ui_mutex_.unlock();
      // Instant message
      if (choice == "1") {
        if (!is_active_) {
          return;
        }
        ui_mutex_.lock();
        mvwprintw(input_win_, 1, 2, "Input the receiver login: ");
        wrefresh(input_win_);
        ui_mutex_.unlock();
        std::string receiver = get_input_line(input_win_);
        if (!is_active_) {
          return;
        }

        ui_mutex_.lock();
        clear_input_window();
        mvwprintw(input_win_, 1, 2, "Input the message: ");
        wrefresh(input_win_);
        ui_mutex_.unlock();
        std::string message = get_input_line(input_win_);
        if (!is_active_) {
          return;
        }

        user_->SendToUser(receiver, message);
        ui_mutex_.lock();
        clear_input_window();
        ui_mutex_.unlock();
        // Delayed message
      } else if (choice == "2") {
        ui_mutex_.lock();
        mvwprintw(input_win_, 1, 2, "Input the receiver login: ");
        wrefresh(input_win_);
        ui_mutex_.unlock();
        std::string receiver = get_input_line(input_win_);
        if (!is_active_) {
          return;
        }

        ui_mutex_.lock();
        clear_input_window();
        mvwprintw(input_win_, 1, 2, "Input the message: ");
        wrefresh(input_win_);
        ui_mutex_.unlock();
        std::string message = get_input_line(input_win_);
        if (!is_active_) {
          return;
        }

        ui_mutex_.lock();
        clear_input_window();
        mvwprintw(input_win_, 1, 2, "Input date (YYYY-MM-DD hh-mm): ");
        wrefresh(input_win_);
        ui_mutex_.unlock();

        std::string time_str = get_input_line(input_win_);
        if (!is_active_) {
          return;
        }

        std::chrono::local_seconds time_to_send;
        std::istringstream istream{time_str};
        istream >> std::chrono::parse("%F %R", time_to_send);
        // TODO: failed to parse
        if (istream.fail()) {
          std::lock_guard<os::Mutex> lk(ui_mutex_);
          mvwprintw(input_win_, 1, 2,
                    "Invalid time format. Press the key to continue...");
          wrefresh(input_win_);
          getch();
          clear_input_window();
        }
        user_->SendDelayedToUser(receiver, message,
                                 kCurrentZone->to_sys(time_to_send));
        ui_mutex_.lock();
        clear_input_window();
        ui_mutex_.unlock();
      } else {
        if (!is_active_) {
          return;
        }
        ui_mutex_.lock();
        mvwprintw(input_win_, 1, 2,
                  "Invalid choice. Press any key to continue...");
        wrefresh(input_win_);
        getch();
        clear_input_window();
        ui_mutex_.unlock();
      }
    }
  }

public:
  void run() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int height, width;
    getmaxyx(stdscr, height, width);

    // Create windows
    history_win_ = newwin(height - 5, width, 0, 0);
    input_win_ = newwin(5, width, height - 5, 0);

    scrollok(history_win_, TRUE);
    box(input_win_, 0, 0);
    wrefresh(history_win_);

    // Run input loop in main thread
    run_input_loop();

    endwin();
  }
  void Shutdown() { is_active_ = false; }
};
std::unique_ptr<ChatApp> kChatApp;
void GracefulShutdown(int signum) {
  kChatApp->Shutdown();
  std::cerr << "Shutdown started\n";
}
int main() {
  struct sigaction sigact;
  sigact.sa_handler = &GracefulShutdown;
  sigact.sa_flags &= ~SA_RESTART;
  int status = sigaction(SIGINT, &sigact, NULL);
  if (status == -1) {
    std::cerr << "Error when establishing signal handler\n";
    return -1;
  }
  int fd = open(kLogFilename.data(), O_WRONLY | O_CREAT, 0600);
  if (fd == -1) {
    std::cerr << "Failed to open log file: " << strerror(errno) << '\n';
    return -1;
  }
  int res = dup2(fd, STDERR_FILENO);
  if (res == -1) {
    std::cerr << "Failed to dup2: " << strerror(errno) << '\n';
    return -1;
  }
  kChatApp = std::make_unique<ChatApp>();
  kChatApp->run();
  return 0;
}
