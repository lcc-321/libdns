// created by lcc 12/16/2021

#ifndef INCLUDE_LIB_DNS_CLIENT_H_
#define INCLUDE_LIB_DNS_CLIENT_H_

#include "lib_dns/config.h"

#include <openssl/ssl.h>

#include <string>
#include <functional>
#include <vector>
#include <map>

#ifdef __linux__
    #include <sys/epoll.h>
#else
    #include <sys/event.h>
#endif

namespace lib_dns {
  const std::string VERSION = std::to_string(LIB_DNS_VERSION_MAJOR) + "." + std::to_string(LIB_DNS_VERSION_MINOR);
  const std::string SERVER = LIB_DNS_WITH_IPV6 ? LIB_DNS_QUERY_SERVER6 : LIB_DNS_QUERY_SERVER4;

  /**
   * DNS record types: resource records (RRs)
   * see https://datatracker.ietf.org/doc/html/rfc1035#page-12
   * see https://datatracker.ietf.org/doc/html/rfc3596#section-2
   */
  const std::map<std::string, std::uint16_t> RRS = {
      { "A", 1 },
      { "NS", 2 },
      { "CNAME", 5 },
      { "PTR", 12 },
      { "MX", 15 },
      { "TXT", 16 },
      { "AAAA", 28 },
      { "SPF", 99 },
  };

  class Client {
   private:
    typedef std::function<void(std::vector<std::vector<char>>)> callback_t;

   public:
    explicit Client(std::int8_t log_verbosity_level);
    Client() : Client(0) {};

    /**
     * Query a DNS name Asynchronously
     * @param name domain name
     * @param type DNS record type, 1: A, 28: AAAA ...
     * @param f callback, will give you an answer list
     */
    void query(const std::string& name, std::uint16_t type, const std::function<void(std::vector<std::string>)>& f);

    /**
     * Send HTTPS Request
     * a utility tool function, if you need ...
     * @param af_type AF_INET or AF_INET6
     * @param ip IP address
     * @param host domain name
     * @param path Request Path, like /wiki/Domain_Name_System
     * @param f callback
     */
    void send_https_request(
        std::int32_t af_type,
        const std::string& ip,
        const std::string& host,
        const std::string& path,
        const callback_t& f);

    /**
     * Receive Server Response
     * you need to put this function to an "Event Loop"
     * @param timeout the timeout expires. see https://man7.org/linux/man-pages/man2/epoll_wait.2.html
     */
    void receive(std::int32_t timeout);

    void receive() { receive(0); };

   private:
    static constexpr std::size_t HTTP_BUFFER_SIZE = 8192;
    static constexpr int MAX_EVENTS = 1;  // epoll will return for 1 event

    /**
     * epoll file descriptor
     * see https://man7.org/linux/man-pages/man7/epoll.7.html
     */
    int event_fd;
#ifdef __linux__
    struct epoll_event events[MAX_EVENTS] { };
#else
    struct kevent events[MAX_EVENTS] { };
#endif

    /**
     * SSL configuration context
     * see https://www.openssl.org/docs/man3.0/man3/SSL_CTX_new.html
     */
    SSL_CTX *ssl_ctx;
    std::map<std::int32_t, SSL*> ssls;

    std::map<std::int32_t, callback_t> callbacks;

#ifdef __linux__
    void process_ssl_response(struct epoll_event event);
#else
    void process_ssl_response(struct kevent event);
#endif

    std::int8_t log_verbosity_level;
  };

  std::string url_encode(const std::string& str);
  std::string char_to_hex(char c);
}

#endif  // INCLUDE_LIB_DNS_CLIENT_H_
