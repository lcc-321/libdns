// created by lcc 12/29/2021

#include "lib_dns/client.h"

#include <iostream>

#include <arpa/inet.h>

int main() {
  auto client = lib_dns::Client(1);

  std::string host = "upload.wikimedia.org";
  std::string path = "/wikipedia/commons/c/ca/En-us-exerting.ogg";
  client.query(host, 1, [&client,host,path](const std::vector<std::string> &dns_data) {
    if (dns_data.empty()) {
      exit(1);
    }
    const std::string& ip = dns_data[0];
    client.send_https_request(LIB_DNS_WITH_IPV6 ? AF_INET6 : AF_INET, ip, host, path, [path](const std::vector<std::vector<char>> &res) {
      const std::vector<char>& body = res[1];

      std::cout << "body.size: " << body.size() << std::endl;
      if (body.size() != 26234) {
        exit(2);
      }
    });
  });

  for (int i = 0; i < 2; i++) { client.receive(9); }
}
