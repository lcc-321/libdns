// created by lcc 12/19/2021

#include "lib_dns/client.h"

int main() {
  std::cout << "version: " << lib_dns::VERSION << std::endl;
  std::cout << "build time: " << LIB_DNS_BUILD_TIME << std::endl;
  if ("0.2" != lib_dns::VERSION) {
    return 1;
  }

  bool stop = false;

  lib_dns::Client client(1);

  client.query("one.one.one.one", lib_dns::RRS.at("A"), [&stop](const std::vector<std::string>& data) {
    stop = true;

    bool found = false;
    for (const auto& row : data) {
      std::cout << "ip: " << row << std::endl;
      if ("1.1.1.1" == row) {
        found = true;
      }
    }

    if (found) {
      exit(0);
    }

    exit(1);
  });

  while (!stop) { client.receive(); }
}
