#pragma once

#include "protocol.hpp"

#include <functional>
#include <string>
#include <vector>

namespace websocket {

namespace internal {

/**
 * Parse url to get hostname, path and port number.
 *
 * @return
 * true: when parse successed
 * false: parse error
 *
 * @example
 *  ws://host:1000 -> "host", "", 1000
 *  ws://host/foo  -> "host", "foo", 80
 *  ws://host/foo/ -> "host", "foo", 80
 */
bool parse_url(const std::string& url, std::string* host, std::string* path,
               int* port);

/**
 * Call recv until recieve "\r\n". Returned value contains "\r\n".
 */
std::string read_line(int sockfd);

/**
 * Check handshake.
 */
bool check_header(int sockfd);

/**
 * Send handshake to the server and establish connection.
 */
int establish_connection(const int sockfd, const std::string& host,
                         const std::string& path, const int port);

/**
 * Get sockfd from hostname and port.
 *
 * TODO: localhost is not available.
 * @return
 *  sockfd: result of ::socket
 *  -1: invalid sockfd (failed to connect)
 *  -2: failed at getaddrinfo
 */
int connect_from_hostname(const std::string& hostname, int port);

/**
 * Send message via socket.
 *
 * TODO: use template
 */
int send(int sockfd, uint8_t* data, const std::string& message);

/**
 * Receive data of specific size with timeout.
 */
int recv_timeout(int sockfd, uint8_t* data, uint64_t size, int timeout = 0);

/**
 * Receive message via socket.
 */
std::string recv(int sockfd, std::vector<uint8_t>& data, int timeout = 0);

/**
 * Close connection.
 *
 * TODO Use protocol.
 */
void close(int sockfd);

}  // namespace internal

/**
 * Class to handle WebSocket connections.
 *
 * TODO: set callbacks (on_message, on_error, on_close, on_open)
 * TODO: async
 * TODO: mutex
 * TODO: logging
 */
class WebSocket {

 public:
  static const std::size_t DEFAULT_SEND_BUF_SIZE = 1024;
  static const std::size_t DEFAULT_RECV_BUF_SIZE = 1024;

  WebSocket()
      : send_buf_(DEFAULT_SEND_BUF_SIZE), recv_buf_(DEFAULT_RECV_BUF_SIZE) {}
  ~WebSocket() { close(); }

  static WebSocket* create_connection(const std::string& url) {
    WebSocket* ws = new WebSocket();

    std::string host, path;
    int port;
    internal::parse_url(url, &host, &path, &port);

    if ((ws->sockfd_ = internal::connect_from_hostname(host, port)) < 0) {
      return nullptr;
    }
    if (!internal::establish_connection(ws->sockfd_, host, path, port)) {
      return nullptr;
    }
    return ws;
  }

  std::size_t send_buf_size() const { return send_buf_.size(); }
  std::size_t recv_buf_size() const { return recv_buf_.size(); }

  int send(const std::string& message) {
    if (send_buf_.size() <
        message.size() + internal::Protocol::MAX_FRAMING_HEADER_LEN) {
      send_buf_.resize(message.size() +
                       internal::Protocol::MAX_FRAMING_HEADER_LEN);
    }
    return internal::send(sockfd_, send_buf_.data(), message);
  }

  std::string recv(int timeout = 0) {
    return internal::recv(sockfd_, recv_buf_, timeout);
  }

  void close() { internal::close(sockfd_); }

  int sockfd() const { return sockfd_; }

 private:
  int sockfd_;
  std::vector<uint8_t> send_buf_;
  std::vector<uint8_t> recv_buf_;
};

/**
 * Alias to WebSocket::create_connection
 */
inline WebSocket* create_connection(const std::string& url) {
  return WebSocket::create_connection(url);
}

}  // namespace websocket
