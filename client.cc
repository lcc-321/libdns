// created by lcc 12/16/2021

#include "lib_dns/client.h"

#include <iostream>
#include <regex>
#include <sstream>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#ifdef __linux__
  #include <sys/epoll.h>
#else
  #include <sys/event.h>
#endif

const std::vector<std::string> HEX_CODES = { "0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F" };
const std::map<std::int32_t, std::string> AF_MAP = { { AF_INET, "IPv4" }, { AF_INET6, "IPv6"} };

int connect_sock(int event_fd, int sock_fd, const sockaddr *sock_addr, const std::size_t sock_addr_len) {
  if (connect(sock_fd, sock_addr, sock_addr_len) == -1) {
    std::cerr << "socket connect error: " << sock_addr->sa_data << std::endl;
#ifdef __linux__
  epoll_ctl(event_fd, EPOLL_CTL_DEL, sock_fd, nullptr);
#endif
    close(event_fd);
    close(sock_fd);
    return -1;
  }
  return sock_fd;
}

int connect_ip(int event_fd, const std::int32_t af_type, const std::string& ip_addr, int port) {
  int sock_fd;
  if ((sock_fd = socket(af_type, SOCK_STREAM, IPPROTO_TCP)) == -1) {
    std::cerr << AF_MAP.at(af_type) << " socket create error" << std::endl;
    return -1;
  }

#ifdef __linux__
  struct epoll_event event{};
  event.events = EPOLLIN;
  event.data.fd = sock_fd;
  if (epoll_ctl(event_fd, EPOLL_CTL_ADD, sock_fd, &event) == -1) {
    std::cerr << "epoll ctl add error" << std::endl;
    close(sock_fd);
    close(event_fd);
    return -1;
  }
#else
  struct kevent event { };
  event.ident = sock_fd;
  EV_SET(&event, sock_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
  if (kevent(event_fd, &event, 1, nullptr, 0, nullptr) == -1) {
    std::cerr << "kevent add error" << std::endl;
    close(sock_fd);
    close(event_fd);
    return -1;
  }
#endif

  sockaddr_storage sock_addr_storage{};
  auto* sock_addr = reinterpret_cast<sockaddr*>(&sock_addr_storage);
  socklen_t sock_addr_len;
  if (af_type == AF_INET6) {
    auto* sin6 = reinterpret_cast<sockaddr_in6*>(sock_addr);
    sin6->sin6_family = af_type;
    sin6->sin6_port = htons(port);
    inet_pton(af_type, ip_addr.c_str(), &sin6->sin6_addr);
    sock_addr_len = sizeof(sockaddr_in6);
  } else {
    auto* sin = reinterpret_cast<sockaddr_in*>(sock_addr);
    sin->sin_family = af_type;
    sin->sin_port = htons(port);
    inet_pton(af_type, ip_addr.c_str(), &sin->sin_addr);
    sock_addr_len = sizeof(sockaddr_in);
  }

  return connect_sock(event_fd, sock_fd, sock_addr, sock_addr_len);
}

lib_dns::Client::Client(const std::int8_t log_verbosity_level) {
#ifdef __linux__
  event_fd = epoll_create1(0);
#else
  event_fd = kqueue();
#endif

  OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, nullptr);
  ssl_ctx = SSL_CTX_new(TLS_client_method());

  this->log_verbosity_level = log_verbosity_level;
}

void lib_dns::Client::send_https_request(
    const std::int32_t af_type,
    const std::string& ip,
    const std::string& host,
    const std::string& path,
    const callback_t& f)
{
  int sock_fd;
  if ((sock_fd = connect_ip(event_fd, af_type, ip, 443)) == -1) {
    std::cerr << "connect error: " << event_fd << ", " << af_type << ", " << ip << std::endl;
    return;
  }

  SSL *ssl = SSL_new(ssl_ctx);
  do {
    if (!ssl) {
      std::cerr << "Error creating SSL." << std::endl;
      break;
    }
    if (SSL_set_fd(ssl, sock_fd) == 0) {
      std::cerr << "Error to set fd." << std::endl;
      break;
    }
    if (const int err = SSL_connect(ssl); err <= 0) {
      std::cerr << "Error creating SSL connection. err = " << err << std::endl;
      break;
    }
    if (log_verbosity_level > 0) {
      std::cout << "SSL connection using: " << sock_fd << ", " << SSL_get_cipher(ssl) << std::endl;
    }

    std::stringstream req;
    req << "GET " << path << " HTTP/1.1\r\n";
    req << "Host: " << host << "\r\n";
    req << "User-Agent: lib_dns/" << VERSION << "\r\n";
    req << "Accept: */*\r\n";
    req << "\r\n";

    const auto data = req.str();
    if (log_verbosity_level > 0) {
      std::cout << "HTTPS request: " << sock_fd << ", " << ip << ": " << data << std::endl;
    }
    if (SSL_write(ssl, data.c_str(), data.length()) < 0) {  // NOLINT(cppcoreguidelines-narrowing-conversions)
      break;
    }

    callbacks[sock_fd] = f;
    ssls[sock_fd] = ssl;
    return;
  } while (false);  // NOLINT

#ifdef __linux__
  epoll_ctl(event_fd, EPOLL_CTL_DEL, sock_fd, nullptr);
#endif
  close(event_fd);
  close(sock_fd);
  SSL_free(ssl);
}

void lib_dns::Client::query(const std::string& name, std::uint16_t type, const std::function<void(std::vector<std::string>)>& f) {
  constexpr std::int32_t af = LIB_DNS_WITH_IPV6 ? AF_INET6 : AF_INET;
  const std::string query = "/resolve?name=" + name + "&type=" + std::to_string(type);
  send_https_request(af, SERVER, "dns.google", query, [type, f](std::vector<std::vector<char>> res) {
    std::vector<std::string> result;
    if (auto data = json_parse(std::string(res[1].begin(), res[1].end()));
      data.at_path("$.Answer").type() == simdjson::ondemand::json_type::array
      && data.at_path("$.Status").type() == simdjson::ondemand::json_type::number
      && data.at_path("$.Status").get_int64() == 0) {
      for (auto answer : data.at_path("$.Answer").get_array()) {
        if (answer["type"].get_int64() == type) {
          std::string answer_data; answer["data"].get_string(answer_data);
          result.emplace_back(answer_data);
        }
      }
    }
    f(result);
  });
}

void lib_dns::Client::receive(const std::int64_t timeout_ms) {
#ifdef __linux__
  if (epoll_wait(event_fd, events, MAX_EVENTS, timeout_ms) > 0) {
    process_ssl_response(events[0]);
  }
#else
  timespec timeout { };
  timeout.tv_sec = timeout_ms / 1000;
  timeout.tv_nsec = timeout_ms % 1000 * 1000000;
  if (const int num_events = kevent(event_fd, nullptr, 0, events, MAX_EVENTS, &timeout); num_events > 0) {
    process_ssl_response(events[0]);
  }
#endif
}

#ifdef __linux__
void lib_dns::Client::process_ssl_response(struct epoll_event event) {
  int sock_fd = event.data.fd;
#else
void lib_dns::Client::process_ssl_response(struct kevent event) {
  int sock_fd = static_cast<int>(event.ident);
#endif
  SSL *ssl = ssls[sock_fd];

  std::vector<char> data;
  std::string head;
  char buffer[HTTP_BUFFER_SIZE];
  int response_size;
  std::uint64_t content_length = 0;
  bool chunked = false;
  std::regex length_regex("\r\nContent-Length: (\\d+)", std::regex_constants::icase);
  std::regex chunked_regex("\r\nTransfer-Encoding: chunked", std::regex_constants::icase);
  long blank_line_pos = -1;
  do {
    if ((response_size = SSL_read(ssl, buffer, HTTP_BUFFER_SIZE)) <= 0) {
      std::cerr << "ssl read error: " << response_size << ", fd: " << sock_fd << std::endl;
      break;
    }
    if (log_verbosity_level > 0) {
      std::cout << "ssl read size: " << response_size
                << ", content-length: " << content_length
                << ", chunked: " << chunked
                << ", blank line position: " << blank_line_pos
                << ", data size: " << data.size()
                << ", head size: " << head.size()
                << std::endl;
    }
    for (int i = 0; i < response_size; i++) { data.push_back(buffer[i]); }

    if (blank_line_pos == -1) {
      for (int i = 0; i < data.size() - 3; i++) {
        if (data[i] == '\r' && data[i+1] == '\n' && data[i+2] == '\r' && data[i+3] == '\n') {
          blank_line_pos = i;
          break;
        }
      }
    }

    if (blank_line_pos != -1) {
      if (head.empty()) {
        head = std::string(data.begin(), data.begin() + blank_line_pos);
      }

      if (content_length == 0 && !chunked) {
        std::smatch length_match;
        if (std::regex_search(head, length_match, length_regex)) {
          content_length = std::stoull(length_match[1]);
        } else if (std::regex_search(head, chunked_regex)) {
          chunked = true;
        }
      }

      if (content_length > 0) {
        if (data.size() - (blank_line_pos + 4) >= content_length) {
          break;
        }
      } else if (chunked) {
        if (data[data.size() - 1] == '\n' && data[data.size() - 2] == '\r'
            && data[data.size() - 3] == '\n' && data[data.size() - 4] == '\r'
            && data[data.size() - 5] == '0') {
          break;
        }
      }
    }
  } while(true);

  if (log_verbosity_level > 0) {
    std::cout << "ssl socket(" << sock_fd << ") response: " << head << '\n' << std::endl;
  }

  std::vector<char> body;
  if (chunked) {
    long chunk_start_pos = blank_line_pos + 4, chunk_data_start_pos = 0, chunk_size;
    do {
      for (long i = chunk_start_pos; i < data.size() - 1; i++) {
        if (data[i] == '\r' && data[i+1] == '\n') {
          chunk_data_start_pos = i + 2;
          break;
        }
      }
      chunk_size = std::stol(std::string(data.begin() + chunk_start_pos, data.begin() + chunk_data_start_pos - 1), nullptr, 16);
      if (chunk_size == 0) {
        break;
      }
      body.insert(body.end(), data.begin() + chunk_data_start_pos, data.begin() + chunk_data_start_pos + chunk_size);
      chunk_start_pos = chunk_data_start_pos + chunk_size + 2;
    } while(chunk_size > 0 && chunk_start_pos < data.size() - 1);
  } else if (content_length > 0) {
    body.insert(body.begin(), data.begin() + blank_line_pos + 4, data.end());
  }

  if (log_verbosity_level > 0) {
    std::cout << std::string(body.begin(), body.end()) << std::endl;
  }

  callbacks[sock_fd]({ std::vector<char>(head.begin(), head.end()), body });
  callbacks.erase(sock_fd);

#ifdef __linux__
  epoll_ctl(event_fd, EPOLL_CTL_DEL, sock_fd, &event);
#endif

  close(sock_fd);
  SSL_free(ssl);
}

simdjson::ondemand::parser lib_dns::Client::json_parser { };
char lib_dns::Client::json_buff[json_buff_size] { };
simdjson::simdjson_result<simdjson::ondemand::document> lib_dns::Client::json_parse(const std::string& json) {
  strcpy(json_buff, json.c_str());
  return json_parser.iterate(json_buff, json.length(), json_buff_size);
}

std::string lib_dns::url_encode(const std::string& str) {
  std::string encode;

  const char *s = str.c_str();
  for (int i = 0; s[i]; i++) {
    if (const char ci = s[i];
        (ci >= 'a' && ci <= 'z') ||
        (ci >= 'A' && ci <= 'Z') ||
        (ci >= '0' && ci <= '9') ) { // allowed
      encode.push_back(ci);
    } else if (ci == ' ') {
      encode.push_back('+');
    } else {
      encode.append("%").append(char_to_hex(ci));
    }
  }

  return encode;
}

std::string lib_dns::char_to_hex(char c) {
  const std::uint8_t n = c;
  std::string res;
  res.append(HEX_CODES[n / 16]);
  res.append(HEX_CODES[n % 16]);
  return res;
}
